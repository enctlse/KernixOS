#include "ehci.h"
#include <kernel/include/io.h>
#include <kernel/mem/kernel_memory/kernel_memory.h>
#include <kernel/mem/meminclude.h>
#include <kernel/shell/acsh.h>
#include <drivers/usb/usb_keyboard.h>
#include <drivers/usb/usb_mouse.h>
#include <kernel/interrupts/handlers/irq.h>
#include <outputs/types.h>
#include <kernel/include/defs.h>
extern volatile struct limine_hhdm_request hhdm_request;
extern int usb_kb_initialized;
extern int usb_mouse_initialized;
static ehci_controller_t ehci_ctrl;
static ehci_device_t devices[256];
static uint8_t device_count = 0;
static void ehci_write32(volatile uint32_t* reg, uint32_t val) {
    *reg = val;
}
static uint32_t ehci_read32(volatile uint32_t* reg) {
    return *reg;
}
static void ehci_wait_ms(int ms) {
    for (int i = 0; i < ms * 1000; i++) {
        __asm__ volatile("nop");
    }
}
static int ehci_wait_for_bit(volatile uint32_t* reg, uint32_t bit, int set, int timeout_ms) {
    for (int i = 0; i < timeout_ms * 1000; i++) {
        if ((ehci_read32(reg) & bit) == (set ? bit : 0)) return 1;
        __asm__ volatile("nop");
    }
    return 0;
}
static ehci_qtd_t* ehci_alloc_qtd() {
    return (ehci_qtd_t*)kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, sizeof(ehci_qtd_t), 1);
}
static ehci_qh_t* ehci_alloc_qh() {
    return (ehci_qh_t*)kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, sizeof(ehci_qh_t), 1);
}
static void ehci_free_qtd(ehci_qtd_t* qtd) {
    kernel_memory_free((kernel_memory_t*)fs_kernel_memory, (u64*)qtd);
}
static void ehci_free_qh(ehci_qh_t* qh) {
    kernel_memory_free((kernel_memory_t*)fs_kernel_memory, (u64*)qh);
}
static int ehci_control_transfer(uint8_t slot_id, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength, void* data) {
    ehci_device_t* dev = &devices[slot_id];
    ehci_qh_t* qh = ehci_alloc_qh();
    if (!qh) return -1;
    qh->horizontal_link = 1;
    qh->characteristics = (dev->max_packet_size << 16) | (dev->speed << 12) | (1 << 15) | (slot_id << 8) | 0x40;
    qh->capabilities = (3 << 28) | (1 << 30);
    qh->current_qtd = 0;
    ehci_qtd_t* setup_qtd = ehci_alloc_qtd();
    if (!setup_qtd) {
        ehci_free_qh(qh);
        return -1;
    }
    setup_qtd->next = 1;
    setup_qtd->alt_next = 1;
    setup_qtd->token = (8 << 16) | (0x2D << 8) | 0x80;
    setup_qtd->buffer[0] = (bmRequestType) | (bRequest << 8) | (wValue << 16);
    setup_qtd->buffer[1] = (wIndex) | (wLength << 16);
    qh->next_qtd = (uint32_t)setup_qtd;
    ehci_qtd_t* data_qtd = NULL;
    if (wLength > 0) {
        data_qtd = ehci_alloc_qtd();
        if (!data_qtd) {
            ehci_free_qtd(setup_qtd);
            ehci_free_qh(qh);
            return -1;
        }
        data_qtd->next = 1;
        data_qtd->alt_next = 1;
        data_qtd->token = (wLength << 16) | ((bmRequestType & 0x80) ? 0x69 : 0xE9);
        data_qtd->buffer[0] = (uint32_t)data;
        setup_qtd->next = (uint32_t)data_qtd;
    }
    ehci_qtd_t* status_qtd = ehci_alloc_qtd();
    if (!status_qtd) {
        if (data_qtd) ehci_free_qtd(data_qtd);
        ehci_free_qtd(setup_qtd);
        ehci_free_qh(qh);
        return -1;
    }
    status_qtd->next = 1;
    status_qtd->alt_next = 1;
    status_qtd->token = (0 << 16) | ((bmRequestType & 0x80) ? 0xE9 : 0x69);
    if (data_qtd) {
        data_qtd->next = (uint32_t)status_qtd;
    } else {
        setup_qtd->next = (uint32_t)status_qtd;
    }
    ehci_write32(&ehci_ctrl.regs->asynclistaddr, (uint32_t)qh | 2);
    ehci_write32(&ehci_ctrl.regs->usbcmd, ehci_read32(&ehci_ctrl.regs->usbcmd) | (1 << 5));
    ehci_wait_ms(10);
    ehci_write32(&ehci_ctrl.regs->usbcmd, ehci_read32(&ehci_ctrl.regs->usbcmd) & ~(1 << 5));
    ehci_free_qtd(status_qtd);
    if (data_qtd) ehci_free_qtd(data_qtd);
    ehci_free_qtd(setup_qtd);
    ehci_free_qh(qh);
    return 0;
}
static int ehci_get_descriptor(uint8_t slot_id, uint8_t desc_type, uint8_t desc_index, uint16_t lang_id, void* buffer, uint16_t length) {
    return ehci_control_transfer(slot_id, 0x80, 6, (desc_type << 8) | desc_index, lang_id, length, buffer);
}
static int ehci_set_configuration(uint8_t slot_id, uint8_t config_value) {
    return ehci_control_transfer(slot_id, 0x00, 9, config_value, 0, 0, NULL);
}
static int ehci_set_address(uint8_t slot_id, uint8_t address) {
    return ehci_control_transfer(slot_id, 0x00, 5, address, 0, 0, NULL);
}
static void ehci_parse_hid_report(uint8_t slot_id, uint8_t* report_desc, uint16_t len) {
    for (uint16_t i = 0; i < len; ) {
        uint8_t bSize = report_desc[i] & 0x03;
        uint8_t bType = (report_desc[i] >> 2) & 0x03;
        uint8_t bTag = (report_desc[i] >> 4) & 0x0F;
        uint32_t data = 0;
        for (uint8_t j = 0; j < bSize; j++) {
            data |= report_desc[i + 1 + j] << (j * 8);
        }
        i += 1 + bSize;
        if (bType == 1) {
            if (bTag == 9) {
                if (data == 0x04) {
                    devices[slot_id].type = 1;
                } else if (data == 0x02) {
                    devices[slot_id].type = 2;
                }
            }
        }
    }
}
static void ehci_enumerate_device(uint8_t slot_id, uint8_t port_id) {
    ehci_device_t* dev = &devices[slot_id];
    dev->slot_id = slot_id;
    dev->port_id = port_id;
    dev->speed = (ehci_read32(&ehci_ctrl.regs->ports[port_id - 1]) >> 26) & 0x3;
    if (dev->speed == 0) dev->speed = 2;
    ehci_get_descriptor(slot_id, 1, 0, 0, &dev->device_desc, sizeof(usb_device_descriptor_t));
    dev->max_packet_size = dev->device_desc.bMaxPacketSize0;
    ehci_set_address(slot_id, slot_id);
    ehci_get_descriptor(slot_id, 2, 0, 0, &dev->config_desc, sizeof(usb_config_descriptor_t));
    uint8_t config_buf[256];
    ehci_get_descriptor(slot_id, 2, 0, 0, config_buf, dev->config_desc.wTotalLength);
    uint16_t offset = sizeof(usb_config_descriptor_t);
    for (uint8_t i = 0; i < dev->config_desc.bNumInterfaces; i++) {
        usb_interface_descriptor_t* iface = (usb_interface_descriptor_t*)&config_buf[offset];
        offset += iface->bLength;
        if (iface->bInterfaceClass == 0x03) {
            dev->device_class = iface->bInterfaceClass;
            dev->device_subclass = iface->bInterfaceSubClass;
            dev->protocol = iface->bInterfaceProtocol;
            dev->interface_desc = *iface;
            for (uint8_t j = 0; j < iface->bNumEndpoints; j++) {
                usb_endpoint_descriptor_t* ep = (usb_endpoint_descriptor_t*)&config_buf[offset];
                offset += ep->bLength;
                dev->ep_desc[j] = *ep;
            }
            usb_hid_descriptor_t* hid = (usb_hid_descriptor_t*)&config_buf[offset];
            offset += hid->bLength;
            dev->hid_desc = *hid;
            ehci_get_descriptor(slot_id, 0x22, 0, 0, dev->report_desc, hid->wReportDescriptorLength);
            dev->report_desc_len = hid->wReportDescriptorLength;
            ehci_parse_hid_report(slot_id, dev->report_desc, dev->report_desc_len);
            break;
        }
    }
    ehci_set_configuration(slot_id, 1);
    if (dev->type == 1) {
        print("EHCI: USB Keyboard detected on slot ", cyan);
        char buf[16];
        buf[0] = '\0';
        str_append_uint(buf, slot_id);
        print(buf, cyan);
        print("\n", cyan);
        usb_keyboard_init();
        usb_kb_initialized = 1;
    } else if (dev->type == 2) {
        print("EHCI: USB Mouse detected on slot ", cyan);
        char buf[16];
        buf[0] = '\0';
        str_append_uint(buf, slot_id);
        print(buf, cyan);
        print("\n", cyan);
        usb_mouse_init();
        usb_mouse_initialized = 1;
    }
    if (dev->type == 1 || dev->type == 2) {
        dev->qh = ehci_alloc_qh();
        if (dev->qh) {
            dev->qh->horizontal_link = 1;
            dev->qh->characteristics = (dev->max_packet_size << 16) | (dev->speed << 12) | (1 << 15) | (slot_id << 8) | (dev->ep_desc[0].bEndpointAddress & 0xF);
            dev->qh->capabilities = (3 << 28) | (1 << 30);
            ehci_qtd_t* int_qtd = ehci_alloc_qtd();
            if (int_qtd) {
                int_qtd->next = 1;
                int_qtd->alt_next = 1;
                int_qtd->token = (4 << 16) | 0x80;
                int_qtd->buffer[0] = (uint32_t)kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, 4, 1);
                dev->qh->next_qtd = (uint32_t)int_qtd;
            }
            int frame = 0;
            ehci_ctrl.periodic_qh[frame] = dev->qh;
            ehci_ctrl.frame_list[frame] = ((uint32_t)dev->qh) | 2;
        }
    }
    device_count++;
}
void ehci_enumerate_devices() {
    for (uint8_t port = 1; port <= ehci_ctrl.max_ports; port++) {
        volatile uint32_t* portsc = &ehci_ctrl.regs->ports[port - 1];
        uint32_t status = ehci_read32(portsc);
        if (status & (1 << 0)) {
            print("EHCI: Device connected on port ", cyan);
            char buf[4];
            buf[0] = '\0';
            str_append_uint(buf, port);
            print(buf, cyan);
            print("\n", cyan);
            ehci_write32(portsc, status | (1 << 9));
            ehci_wait_ms(100);
            status = ehci_read32(portsc);
            if ((status & (1 << 0)) == 0) {
                ehci_enumerate_device(device_count, port);
            }
        }
    }
}
ehci_device_t* ehci_get_device(uint8_t slot_id) {
    return &devices[slot_id];
}
int ehci_init(uint64_t base_addr) {
    ehci_ctrl.regs = (ehci_regs_t*)base_addr;
    ehci_ctrl.max_ports = 15;
    ehci_ctrl.frame_list = (uint32_t*)kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, 1024 * 4, 1);
    if (!ehci_ctrl.frame_list) return -1;
    for (int i = 0; i < 1024; i++) {
        ehci_ctrl.frame_list[i] = 1;
    }
    ehci_ctrl.async_qh = ehci_alloc_qh();
    if (!ehci_ctrl.async_qh) return -1;
    ehci_ctrl.async_qh->horizontal_link = (uint32_t)ehci_ctrl.async_qh | 1;
    ehci_ctrl.async_qh->characteristics = 0x40000000;
    ehci_ctrl.async_qh->current_qtd = 0;
    ehci_ctrl.async_qh->next_qtd = 1;
    ehci_write32(&ehci_ctrl.regs->asynclistaddr, (uint32_t)ehci_ctrl.async_qh | 2);
    ehci_write32(&ehci_ctrl.regs->periodiclistbase, (uint32_t)ehci_ctrl.frame_list);
    ehci_write32(&ehci_ctrl.regs->usbintr, 0);
    ehci_ctrl.initialized = 1;
    return 0;
}
void ehci_start() {
    ehci_write32(&ehci_ctrl.regs->usbcmd, ehci_read32(&ehci_ctrl.regs->usbcmd) | (1 << 0) | (1 << 4) | (1 << 5));
    ehci_write32(&ehci_ctrl.regs->configflag, 1);
}
void ehci_stop() {
    ehci_write32(&ehci_ctrl.regs->usbcmd, ehci_read32(&ehci_ctrl.regs->usbcmd) & ~(1 << 0));
}
int ehci_reset() {
    ehci_write32(&ehci_ctrl.regs->usbcmd, ehci_read32(&ehci_ctrl.regs->usbcmd) | (1 << 1));
    ehci_wait_for_bit(&ehci_ctrl.regs->usbsts, (1 << 11), 0, 100);
    return 0;
}
void ehci_handle_interrupt() {
    if (!ehci_ctrl.initialized) return;
    uint32_t status = ehci_read32(&ehci_ctrl.regs->usbsts);
    if (status & (1 << 0)) {
        ehci_write32(&ehci_ctrl.regs->usbsts, status);
        for (int i = 0; i < device_count; i++) {
            ehci_device_t* dev = &devices[i];
            if (dev->qh && dev->qh->current_qtd) {
                ehci_qtd_t* qtd = (ehci_qtd_t*)dev->qh->current_qtd;
                if (qtd->token & (1 << 0)) {
                    uint8_t* data = (uint8_t*)qtd->buffer[0];
                    print("EHCI: Interrupt data received for device ", cyan);
                    char buf[16];
                    buf[0] = '\0';
                    str_append_uint(buf, i);
                    print(buf, cyan);
                    print("\n", cyan);
                    if (dev->type == 1) {
                        usb_keyboard_handle_report((usb_kb_report_t*)data);
                    } else if (dev->type == 2) {
                        usb_mouse_interrupt_handler(data, 4);
                    }
                    dev->qh->current_qtd = 0;
                    ehci_qtd_t* new_qtd = ehci_alloc_qtd();
                    if (new_qtd) {
                        new_qtd->next = 1;
                        new_qtd->alt_next = 1;
                        new_qtd->token = (4 << 16) | 0x80;
                        new_qtd->buffer[0] = (uint32_t)kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, 4, 1);
                        dev->qh->next_qtd = (uint32_t)new_qtd;
                    }
                }
            }
        }
    }
}