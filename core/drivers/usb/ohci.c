#include "ohci.h"
#include <kernel/include/io.h>
#include <kernel/mem/kernel_memory/kernel_memory.h>
#include <kernel/mem/meminclude.h>
#include <kernel/shell/acsh.h>
#include <drivers/usb/usb_keyboard.h>
#include <drivers/usb/usb_mouse.h>
#include <kernel/interrupts/handlers/irq.h>
#include <outputs/types.h>
extern volatile struct limine_hhdm_request hhdm_request;
static ohci_controller_t ohci_ctrl;
static ohci_device_t devices[256];
static uint8_t device_count = 0;
static void ohci_write32(volatile uint32_t* reg, uint32_t val) {
    *reg = val;
}
static uint32_t ohci_read32(volatile uint32_t* reg) {
    return *reg;
}
static void ohci_wait_ms(int ms) {
    for (int i = 0; i < ms * 1000; i++) {
        __asm__ volatile("nop");
    }
}
static int ohci_wait_for_bit(volatile uint32_t* reg, uint32_t bit, int set, int timeout_ms) {
    for (int i = 0; i < timeout_ms * 1000; i++) {
        if ((ohci_read32(reg) & bit) == (set ? bit : 0)) return 1;
        __asm__ volatile("nop");
    }
    return 0;
}
static ohci_ed_t* ohci_alloc_ed() {
    return (ohci_ed_t*)kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, sizeof(ohci_ed_t), 1);
}
static ohci_td_t* ohci_alloc_td() {
    return (ohci_td_t*)kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, sizeof(ohci_td_t), 1);
}
static void ohci_free_ed(ohci_ed_t* ed) {
    kernel_memory_free((kernel_memory_t*)fs_kernel_memory, (u64*)ed);
}
static void ohci_free_td(ohci_td_t* td) {
    kernel_memory_free((kernel_memory_t*)fs_kernel_memory, (u64*)td);
}
static int ohci_control_transfer(uint8_t slot_id, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength, void* data) {
    ohci_device_t* dev = &devices[slot_id];
    ohci_ed_t* ed = ohci_alloc_ed();
    if (!ed) return -1;
    ed->flags = (dev->max_packet_size << 16) | (1 << 15) | (slot_id << 8) | 0x40;
    ed->tailp = 0;
    ed->headp = 0;
    ed->nexted = 0;
    ohci_td_t* setup_td = ohci_alloc_td();
    if (!setup_td) {
        ohci_free_ed(ed);
        return -1;
    }
    setup_td->flags = (8 << 21) | (0x2D << 16) | 0xE000;
    uint64_t setup_cbp_virt = (uint64_t)kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, 8, 1);
    if (!setup_cbp_virt) {
        ohci_free_td(setup_td);
        ohci_free_ed(ed);
        return -1;
    }
    setup_td->cbp = (uint32_t)(setup_cbp_virt - hhdm_request.response->offset);
    *(uint32_t*)setup_cbp_virt = (bmRequestType) | (bRequest << 8) | (wValue << 16);
    *(uint32_t*)(setup_cbp_virt + 4) = (wIndex) | (wLength << 16);
    setup_td->nexttd = 0;
    setup_td->be = setup_td->cbp + 7;
    ed->headp = (uint32_t)((uint64_t)setup_td - hhdm_request.response->offset);
    ohci_td_t* data_td = NULL;
    if (wLength > 0) {
        data_td = ohci_alloc_td();
        if (!data_td) {
            kernel_memory_free((kernel_memory_t*)fs_kernel_memory, (u64*)setup_td->cbp);
            ohci_free_td(setup_td);
            ohci_free_ed(ed);
            return -1;
        }
        data_td->flags = (wLength << 21) | ((bmRequestType & 0x80) ? 0x69 : 0xE9);
        data_td->cbp = (uint32_t)data;
        data_td->nexttd = 0;
        data_td->be = (uint32_t)data + wLength - 1;
        setup_td->nexttd = (uint32_t)data_td;
    }
    ohci_td_t* status_td = ohci_alloc_td();
    if (!status_td) {
        if (data_td) ohci_free_td(data_td);
        kernel_memory_free((kernel_memory_t*)fs_kernel_memory, (u64*)setup_td->cbp);
        ohci_free_td(setup_td);
        ohci_free_ed(ed);
        return -1;
    }
    status_td->flags = (0 << 21) | ((bmRequestType & 0x80) ? 0xE9 : 0x69);
    status_td->cbp = 0;
    status_td->nexttd = 0;
    status_td->be = 0;
    if (data_td) {
        data_td->nexttd = (uint32_t)status_td;
    } else {
        setup_td->nexttd = (uint32_t)status_td;
    }
    ed->tailp = (uint32_t)status_td;
    ohci_write32(&ohci_ctrl.regs->controlheaded, (uint32_t)ed);
    ohci_write32(&ohci_ctrl.regs->cmdstatus, 1 << 0);
    ohci_wait_ms(10);
    kernel_memory_free((kernel_memory_t*)fs_kernel_memory, (u64*)setup_td->cbp);
    ohci_free_td(status_td);
    if (data_td) ohci_free_td(data_td);
    ohci_free_td(setup_td);
    ohci_free_ed(ed);
    return 0;
}
static int ohci_get_descriptor(uint8_t slot_id, uint8_t desc_type, uint8_t desc_index, uint16_t lang_id, void* buffer, uint16_t length) {
    return ohci_control_transfer(slot_id, 0x80, 6, (desc_type << 8) | desc_index, lang_id, length, buffer);
}
static int ohci_set_configuration(uint8_t slot_id, uint8_t config_value) {
    return ohci_control_transfer(slot_id, 0x00, 9, config_value, 0, 0, NULL);
}
static int ohci_set_address(uint8_t slot_id, uint8_t address) {
    return ohci_control_transfer(slot_id, 0x00, 5, address, 0, 0, NULL);
}
static void ohci_parse_hid_report(uint8_t slot_id, uint8_t* report_desc, uint16_t len) {
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
static void ohci_enumerate_device(uint8_t slot_id, uint8_t port_id) {
    ohci_device_t* dev = &devices[slot_id];
    dev->slot_id = slot_id;
    dev->port_id = port_id;
    dev->speed = 1;
    ohci_get_descriptor(slot_id, 1, 0, 0, &dev->device_desc, sizeof(usb_device_descriptor_t));
    dev->max_packet_size = dev->device_desc.bMaxPacketSize0;
    ohci_set_address(slot_id, slot_id);
    ohci_get_descriptor(slot_id, 2, 0, 0, &dev->config_desc, sizeof(usb_config_descriptor_t));
    uint8_t config_buf[256];
    ohci_get_descriptor(slot_id, 2, 0, 0, config_buf, dev->config_desc.wTotalLength);
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
            ohci_get_descriptor(slot_id, 0x22, 0, 0, dev->report_desc, hid->wReportDescriptorLength);
            dev->report_desc_len = hid->wReportDescriptorLength;
            ohci_parse_hid_report(slot_id, dev->report_desc, dev->report_desc_len);
            break;
        }
    }
    ohci_set_configuration(slot_id, 1);
    if (dev->type == 1 || dev->type == 2) {
        dev->ed = ohci_alloc_ed();
        if (dev->ed) {
            dev->ed->flags = (dev->max_packet_size << 16) | (1 << 15) | (slot_id << 8) | (dev->ep_desc[0].bEndpointAddress & 0xF);
            dev->ed->tailp = 0;
            dev->ed->headp = 0;
            dev->ed->nexted = 0;
            ohci_td_t* td = ohci_alloc_td();
            if (td) {
                td->flags = (4 << 21) | 0xE1;
                td->cbp = (uint32_t)kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, 4, 1);
                td->nexttd = 0;
                td->be = td->cbp + 3;
                dev->ed->headp = (uint32_t)td;
                dev->ed->tailp = (uint32_t)td;
            }
            ohci_write32(&ohci_ctrl.regs->controlheaded, (uint32_t)dev->ed);
        }
    }
    device_count++;
}
void ohci_enumerate_devices() {
    for (uint8_t port = 1; port <= 2; port++) {
        volatile uint32_t* portsc = &ohci_ctrl.regs->rhportstatus[port - 1];
        uint32_t status = ohci_read32(portsc);
        if (status & (1 << 0)) {
            ohci_write32(portsc, status | (1 << 9));
            ohci_wait_ms(100);
            status = ohci_read32(portsc);
            if ((status & (1 << 0)) == 0) {
                ohci_enumerate_device(device_count, port);
            }
        }
    }
}
ohci_device_t* ohci_get_device(uint8_t slot_id) {
    return &devices[slot_id];
}
int ohci_init(uint64_t base_addr) {
    ohci_ctrl.regs = (ohci_regs_t*)base_addr;
    ohci_ctrl.hcca = (ohci_hcca_t*)kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, sizeof(ohci_hcca_t), 1);
    if (!ohci_ctrl.hcca) return -1;
    ohci_write32(&ohci_ctrl.regs->hcca, (uint32_t)ohci_ctrl.hcca);
    ohci_write32(&ohci_ctrl.regs->control, 0);
    ohci_write32(&ohci_ctrl.regs->cmdstatus, 1 << 0);
    ohci_wait_for_bit(&ohci_ctrl.regs->cmdstatus, 1 << 0, 0, 100);
    ohci_ctrl.initialized = 1;
    return 0;
}
void ohci_start() {
    ohci_write32(&ohci_ctrl.regs->control, (1 << 0) | (1 << 2) | (1 << 3));
    ohci_write32(&ohci_ctrl.regs->rhstatus, 1 << 15);
}
void ohci_stop() {
    ohci_write32(&ohci_ctrl.regs->control, 0);
}
int ohci_reset() {
    ohci_write32(&ohci_ctrl.regs->cmdstatus, 1 << 1);
    ohci_wait_for_bit(&ohci_ctrl.regs->cmdstatus, 1 << 1, 0, 100);
    return 0;
}
void ohci_handle_interrupt() {
    if (!ohci_ctrl.initialized) return;
    uint32_t status = ohci_read32(&ohci_ctrl.regs->intstatus);
    if (status & (1 << 0)) {
        ohci_write32(&ohci_ctrl.regs->intstatus, status);
        uint32_t done_head = ohci_read32(&ohci_ctrl.regs->donehead);
        while (done_head & ~1) {
            ohci_td_t* td = (ohci_td_t*)(done_head & ~0xF);
            if (td->cbp) {
                uint8_t* data = (uint8_t*)td->cbp;
                for (int i = 0; i < device_count; i++) {
                    ohci_device_t* dev = &devices[i];
                    if (dev->ed && dev->ed->headp == (uint32_t)td) {
                        if (dev->type == 1) {
                            usb_keyboard_handle_report((usb_kb_report_t*)data);
                        } else if (dev->type == 2) {
                            usb_mouse_interrupt_handler(data, 4);
                        }
                        break;
                    }
                }
            }
            done_head = td->nexttd;
        }
        ohci_write32(&ohci_ctrl.regs->donehead, 0);
    }
}