#include "uhci.h"
#include <kernel/include/io.h>
#include <kernel/mem/kernel_memory/kernel_memory.h>
#include <kernel/mem/meminclude.h>
#include <kernel/shell/acsh.h>
#include <drivers/usb/usb_keyboard.h>
#include <drivers/usb/usb_mouse.h>
#include <kernel/interrupts/handlers/irq.h>
#include <outputs/types.h>
#include <kernel/include/defs.h>
static uhci_controller_t uhci_ctrl;
static uhci_device_t devices[256];
static uint8_t device_count = 0;
static void uhci_write16(uint16_t port, uint16_t val) {
    outw(port, val);
}
static uint16_t uhci_read16(uint16_t port) {
    return inw(port);
}
static void uhci_write32(uint16_t port, uint32_t val) {
    outl(port, val);
}
static void uhci_wait_ms(int ms) {
    for (int i = 0; i < ms * 1000; i++) {
        __asm__ volatile("nop");
    }
}
static int uhci_wait_for_bit(uint16_t port, uint16_t bit, int set, int timeout_ms) {
    for (int i = 0; i < timeout_ms * 1000; i++) {
        if ((uhci_read16(port) & bit) == (set ? bit : 0)) return 1;
        __asm__ volatile("nop");
    }
    return 0;
}
static uhci_td_t* uhci_alloc_td() {
    return (uhci_td_t*)kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, sizeof(uhci_td_t), 1);
}
static void uhci_free_td(uhci_td_t* td) {
    kernel_memory_free((kernel_memory_t*)fs_kernel_memory, (u64*)td);
}
static int uhci_control_transfer(uint8_t slot_id, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength, void* data) {
    uhci_device_t* dev = &devices[slot_id];
    uhci_td_t* setup_td = uhci_alloc_td();
    if (!setup_td) return -1;
    setup_td->link = 1;
    setup_td->status = 0x80;
    setup_td->token = (8 << 21) | (0x2D << 16) | (slot_id << 8) | 0xE1;
    uint64_t setup_buffer_virt = (uint64_t)kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, 8, 1);
    if (!setup_buffer_virt) {
        uhci_free_td(setup_td);
        return -1;
    }
    setup_td->buffer_virt = setup_buffer_virt;
    setup_td->buffer = (uint32_t)(setup_buffer_virt - hhdm_request.response->offset);
    *(uint32_t*)setup_buffer_virt = (bmRequestType) | (bRequest << 8) | (wValue << 16);
    *(uint32_t*)(setup_buffer_virt + 4) = (wIndex) | (wLength << 16);
    uhci_td_t* data_td = NULL;
    if (wLength > 0) {
        data_td = uhci_alloc_td();
        if (!data_td) {
            kernel_memory_free((kernel_memory_t*)fs_kernel_memory, (u64*)setup_td->buffer_virt);
            uhci_free_td(setup_td);
            return -1;
        }
        data_td->link = 1;
        data_td->status = 0x80;
        data_td->token = (wLength << 21) | ((bmRequestType & 0x80) ? 0x69 : 0xE9) | (slot_id << 8) | (dev->ep_desc[0].bEndpointAddress & 0xF);
        data_td->buffer_virt = (uint64_t)data;
        data_td->buffer = (uint32_t)((uint64_t)data - hhdm_request.response->offset);
        setup_td->link = (uint32_t)(uint64_t)data_td;
    }
    uhci_td_t* status_td = uhci_alloc_td();
    if (!status_td) {
        if (data_td) uhci_free_td(data_td);
        kernel_memory_free((kernel_memory_t*)fs_kernel_memory, (u64*)setup_td->buffer_virt);
        uhci_free_td(setup_td);
        return -1;
    }
    status_td->link = 1;
    status_td->status = 0x80;
    status_td->token = (0 << 21) | ((bmRequestType & 0x80) ? 0xE9 : 0x69) | (slot_id << 8) | 0xE1;
    status_td->buffer = 0;
    status_td->buffer_virt = 0;
    if (data_td) {
        data_td->link = (uint32_t)((uint64_t)status_td | 1);
    } else {
        setup_td->link = (uint32_t)((uint64_t)status_td | 1);
    }
    uhci_ctrl.frame_list[0] = (uint32_t)((uint64_t)setup_td | 1);
    uhci_ctrl.td->status = 0x80;
    uhci_wait_ms(10);
    kernel_memory_free((kernel_memory_t*)fs_kernel_memory, (u64*)setup_td->buffer_virt);
    uhci_free_td(status_td);
    if (data_td) uhci_free_td(data_td);
    uhci_free_td(setup_td);
    return 0;
}
static int uhci_get_descriptor(uint8_t slot_id, uint8_t desc_type, uint8_t desc_index, uint16_t lang_id, void* buffer, uint16_t length) {
    return uhci_control_transfer(slot_id, 0x80, 6, (desc_type << 8) | desc_index, lang_id, length, buffer);
}
static int uhci_set_configuration(uint8_t slot_id, uint8_t config_value) {
    return uhci_control_transfer(slot_id, 0x00, 9, config_value, 0, 0, NULL);
}
static int uhci_set_address(uint8_t slot_id, uint8_t address) {
    return uhci_control_transfer(slot_id, 0x00, 5, address, 0, 0, NULL);
}
static void uhci_parse_hid_report(uint8_t slot_id, uint8_t* report_desc, uint16_t len) {
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
static void uhci_enumerate_device(uint8_t slot_id, uint8_t port_id) {
    uhci_device_t* dev = &devices[slot_id];
    dev->slot_id = slot_id;
    dev->port_id = port_id;
    dev->speed = 1;
    uhci_get_descriptor(slot_id, 1, 0, 0, &dev->device_desc, sizeof(usb_device_descriptor_t));
    dev->max_packet_size = dev->device_desc.bMaxPacketSize0;
    uhci_set_address(slot_id, slot_id);
    uhci_get_descriptor(slot_id, 2, 0, 0, &dev->config_desc, sizeof(usb_config_descriptor_t));
    uint8_t config_buf[256];
    uhci_get_descriptor(slot_id, 2, 0, 0, config_buf, dev->config_desc.wTotalLength);
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
            uhci_get_descriptor(slot_id, 0x22, 0, 0, dev->report_desc, hid->wReportDescriptorLength);
            dev->report_desc_len = hid->wReportDescriptorLength;
            uhci_parse_hid_report(slot_id, dev->report_desc, dev->report_desc_len);
            break;
        }
    }
    uhci_set_configuration(slot_id, 1);
    if (dev->type == 1 || dev->type == 2) {
        dev->td = uhci_alloc_td();
        if (dev->td) {
            dev->td->link = 1;
            dev->td->status = 0x80;
            uint16_t packet_size = dev->ep_desc[0].wMaxPacketSize;
            dev->td->token = (packet_size << 21) | 0x69 | (slot_id << 8) | (dev->ep_desc[0].bEndpointAddress & 0xF);
            dev->td->buffer_virt = (uint64_t)kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, packet_size, 1);
            dev->td->buffer = (uint32_t)(dev->td->buffer_virt - hhdm_request.response->offset);
            uhci_ctrl.frame_list[0] = (uint32_t)((uint64_t)dev->td | 1);
        }
    }
    device_count++;
}
void uhci_enumerate_devices() {
    for (uint8_t port = 1; port <= 2; port++) {
        uint16_t portsc = uhci_ctrl.io_base + UHCI_PORTSC1 + (port - 1) * 2;
        uint16_t status = uhci_read16(portsc);
        if (status & (1 << 0)) {
            uhci_write16(portsc, status | (1 << 9));
            uhci_wait_ms(100);
            status = uhci_read16(portsc);
            if ((status & (1 << 0)) == 0) {
                uhci_enumerate_device(device_count, port);
            }
        }
    }
}
uhci_device_t* uhci_get_device(uint8_t slot_id) {
    return &devices[slot_id];
}
int uhci_init(uint64_t base_addr) {
    if (!fs_kernel_memory || base_addr == 0) return -1;
    uhci_ctrl.io_base = (uint16_t)base_addr;
    uint16_t test = uhci_read16(uhci_ctrl.io_base + UHCI_USBCMD);
    if (test == 0xFFFF) return -1;
    uhci_ctrl.td = uhci_alloc_td();
    if (!uhci_ctrl.td) return -1;
    uhci_ctrl.frame_list = (uint32_t*)kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, 1024 * 4, 1);
    if (!uhci_ctrl.frame_list) {
        uhci_free_td(uhci_ctrl.td);
        return -1;
    }
    for (int i = 0; i < 1024; i++) {
        uhci_ctrl.frame_list[i] = 1;
    }
    uhci_write32(uhci_ctrl.io_base + UHCI_FLBASEADD, (uint32_t)uhci_ctrl.frame_list);
    uhci_ctrl.td->status = 0;
    uhci_ctrl.initialized = 1;
    return 0;
}
void uhci_start() {
    uhci_write16(uhci_ctrl.io_base + UHCI_USBCMD, 1);
}
void uhci_stop() {
    uhci_write16(uhci_ctrl.io_base + UHCI_USBCMD, 0);
}
int uhci_reset() {
    uhci_write16(uhci_ctrl.io_base + UHCI_USBCMD, 1 << 1);
    uhci_wait_for_bit(uhci_ctrl.io_base + UHCI_USBSTS, 1 << 1, 0, 100);
    return 0;
}
void uhci_handle_interrupt() {
    if (!uhci_ctrl.initialized) return;
    uint16_t status = uhci_read16(uhci_ctrl.io_base + UHCI_USBSTS);
    if (status & (1 << 0)) {
        uhci_write16(uhci_ctrl.io_base + UHCI_USBSTS, status);
        for (int i = 0; i < device_count; i++) {
            uhci_device_t* dev = &devices[i];
            if (dev->td && (dev->td->status & 0x80) == 0) {
                uint8_t* data = (uint8_t*)dev->td->buffer_virt;
                if (dev->type == 1) {
                    usb_keyboard_handle_report((usb_kb_report_t*)data);
                } else if (dev->type == 2) {
                    usb_mouse_interrupt_handler(data, 4);
                }
                dev->td->status = 0x80;
                break;
            }
        }
    }
}