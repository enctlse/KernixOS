#include "xhci.h"
#include <kernel/include/io.h>
#include <kernel/mem/kernel_memory/kernel_memory.h>
#include <kernel/mem/meminclude.h>
#include <kernel/shell/acsh.h>
#include <drivers/usb/usb_keyboard.h>
#include <drivers/usb/usb_mouse.h>
#include <kernel/interrupts/handlers/irq.h>
#include <outputs/types.h>
static xhci_controller_t xhci_ctrl;
static xhci_device_t devices[256];
static uint8_t device_count = 0;
static int xhci_initialized = 0;
static void xhci_write32(volatile uint32_t* reg, uint32_t val) {
    *reg = val;
}
static uint32_t xhci_read32(volatile uint32_t* reg) {
    return *reg;
}
static void xhci_write64(volatile uint64_t* reg, uint64_t val) {
    *reg = val;
}
static uint64_t xhci_read64(volatile uint64_t* reg) {
    return *reg;
}
static void xhci_wait_ms(int ms) {
    for (int i = 0; i < ms * 1000; i++) {
        __asm__ volatile("nop");
    }
}
static int xhci_wait_for_bit(volatile uint32_t* reg, uint32_t bit, int set, int timeout_ms) {
    for (int i = 0; i < timeout_ms * 1000; i++) {
        if ((xhci_read32(reg) & bit) == (set ? bit : 0)) return 1;
        __asm__ volatile("nop");
    }
    return 0;
}
static void xhci_ring_doorbell(uint8_t slot, uint8_t ep) {
    xhci_ctrl.db_regs[slot] = ep;
}
static xhci_trb_t* xhci_get_next_trb(xhci_trb_t* ring, uint32_t* index, uint32_t size) {
    xhci_trb_t* trb = &ring[*index];
    *index = (*index + 1) % size;
    return trb;
}
static void xhci_submit_command(xhci_trb_t* trb) {
    xhci_trb_t* cmd_trb = xhci_get_next_trb(xhci_ctrl.cmd_ring, &xhci_ctrl.cmd_ring_index, 256);
    *cmd_trb = *trb;
    cmd_trb->control |= 1;
    xhci_ring_doorbell(0, 0);
}
static int xhci_alloc_device_context(uint8_t slot_id) {
    if (slot_id >= xhci_ctrl.max_slots) return -1;
    xhci_ctrl.contexts[slot_id] = (xhci_device_context_t*)kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, sizeof(xhci_device_context_t), 1);
    if (!xhci_ctrl.contexts[slot_id]) return -1;
    xhci_ctrl.dcbaap[slot_id + 1] = (uint64_t)xhci_ctrl.contexts[slot_id];
    return 0;
}
static void xhci_free_device_context(uint8_t slot_id) {
    if (slot_id < xhci_ctrl.max_slots && xhci_ctrl.contexts[slot_id]) {
        kernel_memory_free((kernel_memory_t*)fs_kernel_memory, (u64*)xhci_ctrl.contexts[slot_id]);
        xhci_ctrl.contexts[slot_id] = NULL;
        xhci_ctrl.dcbaap[slot_id + 1] = 0;
    }
}
static int xhci_control_transfer(uint8_t slot_id, uint8_t ep, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength, void* data) {
    xhci_trb_t setup_trb = {0};
    setup_trb.parameter = (uint64_t)&setup_trb;
    setup_trb.status = (bmRequestType << 24) | (bRequest << 16) | wValue;
    setup_trb.command = (wIndex << 16) | wLength;
    setup_trb.control = XHCI_TRB_TYPE_SETUP_STAGE;
    xhci_submit_command(&setup_trb);
    if (wLength > 0) {
        xhci_trb_t data_trb = {0};
        data_trb.parameter = (uint64_t)data;
        data_trb.status = wLength;
        data_trb.control = XHCI_TRB_TYPE_DATA_STAGE | (bmRequestType & 0x80 ? 0 : (1 << 16));
        xhci_submit_command(&data_trb);
    }
    xhci_trb_t status_trb = {0};
    status_trb.control = XHCI_TRB_TYPE_STATUS_STAGE | ((bmRequestType & 0x80) ? (1 << 16) : 0);
    xhci_submit_command(&status_trb);
    return 0;
}
static int xhci_get_descriptor(uint8_t slot_id, uint8_t desc_type, uint8_t desc_index, uint16_t lang_id, void* buffer, uint16_t length) {
    return xhci_control_transfer(slot_id, 0, 0x80, 6, (desc_type << 8) | desc_index, lang_id, length, buffer);
}
static int xhci_set_configuration(uint8_t slot_id, uint8_t config_value) {
    return xhci_control_transfer(slot_id, 0, 0x00, 9, config_value, 0, 0, NULL);
}
static int xhci_set_interface(uint8_t slot_id, uint8_t interface, uint8_t alt_setting) {
    return xhci_control_transfer(slot_id, 0, 0x01, 11, alt_setting, interface, 0, NULL);
}
static void xhci_parse_hid_report(uint8_t slot_id, uint8_t* report_desc, uint16_t len) {
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
static void xhci_enumerate_device(uint8_t slot_id, uint8_t port_id) {
    xhci_device_t* dev = &devices[slot_id];
    dev->slot_id = slot_id;
    dev->port_id = port_id;
    dev->speed = (xhci_read32(&xhci_ctrl.op_regs->usbsts) >> 20) & 0xF;
    xhci_get_descriptor(slot_id, 1, 0, 0, &dev->device_desc, sizeof(usb_device_descriptor_t));
    xhci_get_descriptor(slot_id, 2, 0, 0, &dev->config_desc, sizeof(usb_config_descriptor_t));
    uint8_t config_buf[256];
    xhci_get_descriptor(slot_id, 2, 0, 0, config_buf, dev->config_desc.wTotalLength);
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
            xhci_get_descriptor(slot_id, 0x22, 0, 0, dev->report_desc, hid->wReportDescriptorLength);
            dev->report_desc_len = hid->wReportDescriptorLength;
            xhci_parse_hid_report(slot_id, dev->report_desc, dev->report_desc_len);
            break;
        }
    }
    xhci_set_configuration(slot_id, 1);
    if (dev->type == 1) {
        print("xHCI: USB Keyboard detected on slot ", cyan);
        char buf[16];
        buf[0] = '\0';
        str_append_uint(buf, slot_id);
        print(buf, cyan);
        print("\n", cyan);
    } else if (dev->type == 2) {
        print("xHCI: USB Mouse detected on slot ", cyan);
        char buf[16];
        buf[0] = '\0';
        str_append_uint(buf, slot_id);
        print(buf, cyan);
        print("\n", cyan);
    }
    if (dev->type == 1 || dev->type == 2) {
        uint8_t ep_index = 1;
        uint16_t max_packet = dev->ep_desc[0].wMaxPacketSize;
        xhci_configure_endpoint(slot_id, ep_index, 3, max_packet, 8);
        uint64_t buffer = (uint64_t)kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, 64, 1);
        if (buffer) {
            xhci_setup_interrupt_transfer(slot_id, ep_index, buffer, 64);
        }
    }
    device_count++;
}
void xhci_enumerate_devices() {
    for (uint8_t port = 1; port <= xhci_ctrl.max_ports; port++) {
        volatile uint32_t* portsc = (volatile uint32_t*)((uint64_t)xhci_ctrl.op_regs + XHCI_PORTSC(port));
        uint32_t status = xhci_read32(portsc);
        if (status & (1 << 0)) {
            uint8_t slot_id;
            if (xhci_enable_slot(&slot_id) == 0) {
                xhci_address_device(slot_id, port);
                xhci_enumerate_device(slot_id, port);
                xhci_write32(portsc, status | (1 << 9));
            }
        }
    }
}
xhci_device_t* xhci_get_device(uint8_t slot_id) {
    return &devices[slot_id];
}
int xhci_init(uint64_t base_addr) {
    xhci_ctrl.cap_regs = (volatile xhci_cap_regs_t*)base_addr;
    uint32_t caplength = xhci_read32(&xhci_ctrl.cap_regs->caplength);
    xhci_ctrl.op_regs = (volatile xhci_op_regs_t*)(base_addr + caplength);
    uint32_t rtsoff = xhci_read32(&xhci_ctrl.cap_regs->rtsoff);
    xhci_ctrl.rt_regs = (volatile xhci_runtime_regs_t*)(base_addr + rtsoff);
    uint32_t dboff = xhci_read32(&xhci_ctrl.cap_regs->dboff);
    xhci_ctrl.db_regs = (volatile uint32_t*)(base_addr + dboff);
    xhci_ctrl.max_slots = (xhci_read32(&xhci_ctrl.cap_regs->hcsparams1) >> 0) & 0xFF;
    xhci_ctrl.max_ports = (xhci_read32(&xhci_ctrl.cap_regs->hcsparams1) >> 24) & 0xFF;
    xhci_ctrl.context_size = (xhci_read32(&xhci_ctrl.cap_regs->hccparams1) & (1 << 2)) ? 64 : 32;
    xhci_ctrl.page_size = xhci_read32(&xhci_ctrl.op_regs->pagesize) & 0xFFFF;
    xhci_ctrl.dcbaap = (uint64_t*)kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, (xhci_ctrl.max_slots + 1) * 8, 1);
    if (!xhci_ctrl.dcbaap) return -1;
    xhci_ctrl.contexts = (xhci_device_context_t**)kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, xhci_ctrl.max_slots * 8, 1);
    if (!xhci_ctrl.contexts) return -1;
    xhci_ctrl.cmd_ring = (xhci_trb_t*)kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, 256 * sizeof(xhci_trb_t), 1);
    if (!xhci_ctrl.cmd_ring) return -1;
    xhci_ctrl.event_ring = (xhci_trb_t*)kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, 256 * sizeof(xhci_trb_t), 1);
    if (!xhci_ctrl.event_ring) return -1;
    xhci_ctrl.erst = (xhci_erst_entry_t*)kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, sizeof(xhci_erst_entry_t), 1);
    if (!xhci_ctrl.erst) return -1;
    xhci_ctrl.dcbaap[0] = (uint64_t)xhci_ctrl.dcbaap;
    xhci_write64(&xhci_ctrl.op_regs->dcbaap, (uint64_t)xhci_ctrl.dcbaap);
    xhci_ctrl.cmd_ring_index = 0;
    xhci_ctrl.event_ring_index = 0;
    xhci_ctrl.erst->base = (uint64_t)xhci_ctrl.event_ring;
    xhci_ctrl.erst->size = 256;
    xhci_write32(&xhci_ctrl.rt_regs->erstsz[0], 1);
    xhci_write64(&xhci_ctrl.rt_regs->erstba[0], (uint64_t)xhci_ctrl.erst);
    xhci_write64(&xhci_ctrl.rt_regs->erdp[0], (uint64_t)xhci_ctrl.event_ring);
    xhci_write32(&xhci_ctrl.op_regs->config, xhci_ctrl.max_slots);
    xhci_initialized = 1;
    return 0;
}
void xhci_start() {
    uint32_t cmd = xhci_read32(&xhci_ctrl.op_regs->usbcmd);
    cmd |= (1 << 0);
    xhci_write32(&xhci_ctrl.op_regs->usbcmd, cmd);
    xhci_wait_for_bit(&xhci_ctrl.op_regs->usbsts, (1 << 0), 0, 100);
}
void xhci_stop() {
    uint32_t cmd = xhci_read32(&xhci_ctrl.op_regs->usbcmd);
    cmd &= ~(1 << 0);
    xhci_write32(&xhci_ctrl.op_regs->usbcmd, cmd);
    xhci_wait_for_bit(&xhci_ctrl.op_regs->usbsts, (1 << 0), 1, 100);
}
int xhci_reset() {
    uint32_t cmd = xhci_read32(&xhci_ctrl.op_regs->usbcmd);
    cmd |= (1 << 1);
    xhci_write32(&xhci_ctrl.op_regs->usbcmd, cmd);
    xhci_wait_for_bit(&xhci_ctrl.op_regs->usbsts, (1 << 11), 0, 100);
    return 0;
}
int xhci_get_max_slots() {
    return xhci_ctrl.max_slots;
}
int xhci_get_max_ports() {
    return xhci_ctrl.max_ports;
}
int xhci_enable_slot(uint8_t* slot_id) {
    xhci_trb_t trb = {0};
    trb.control = XHCI_TRB_TYPE_ENABLE_SLOT;
    xhci_submit_command(&trb);
    return 0;
}
int xhci_disable_slot(uint8_t slot_id) {
    xhci_trb_t trb = {0};
    trb.parameter = slot_id;
    trb.control = XHCI_TRB_TYPE_DISABLE_SLOT;
    xhci_submit_command(&trb);
    return 0;
}
int xhci_address_device(uint8_t slot_id, uint8_t port_id) {
    xhci_alloc_device_context(slot_id);
    xhci_device_context_t* ctx = xhci_ctrl.contexts[slot_id];
    ctx->slot.route_string = 0;
    ctx->slot.speed = devices[slot_id].speed;
    ctx->slot.slot_state = XHCI_SLOT_STATE_DEFAULT;
    ctx->ep[0].ep_state = XHCI_ENDPOINT_STATE_RUNNING;
    ctx->ep[0].ep_type = 4;
    ctx->ep[0].dequeue_ptr = (uint64_t)xhci_ctrl.cmd_ring;
    xhci_trb_t trb = {0};
    trb.parameter = slot_id;
    trb.control = XHCI_TRB_TYPE_ADDRESS_DEVICE;
    xhci_submit_command(&trb);
    return 0;
}
int xhci_configure_endpoint(uint8_t slot_id, uint8_t ep_index, uint8_t ep_type, uint16_t max_packet_size, uint8_t interval) {
    xhci_device_context_t* ctx = xhci_ctrl.contexts[slot_id];
    ctx->ep[ep_index].ep_state = XHCI_ENDPOINT_STATE_RUNNING;
    ctx->ep[ep_index].ep_type = ep_type;
    ctx->ep[ep_index].dequeue_ptr = (uint64_t)xhci_ctrl.event_ring;
    xhci_trb_t trb = {0};
    trb.parameter = slot_id;
    trb.control = XHCI_TRB_TYPE_CONFIGURE_ENDPOINT;
    xhci_submit_command(&trb);
    return 0;
}
int xhci_setup_interrupt_transfer(uint8_t slot_id, uint8_t ep_index, uint64_t buffer, uint32_t length) {
    xhci_trb_t trb = {0};
    trb.parameter = buffer;
    trb.status = length;
    trb.control = XHCI_TRB_TYPE_NORMAL;
    xhci_submit_command(&trb);
    return 0;
}
void xhci_handle_event() {
    if (!xhci_initialized || !xhci_ctrl.event_ring) return;
    xhci_trb_t* event = &xhci_ctrl.event_ring[xhci_ctrl.event_ring_index];
    if (event->control & 1) {
        uint8_t type = (event->control >> 10) & 0x3F;
        if (type == XHCI_TRB_TYPE_TRANSFER_EVENT) {
            uint8_t slot_id = (event->control >> 24) & 0xFF;
            uint8_t ep_id = (event->control >> 16) & 0x1F;
            xhci_device_t* dev = &devices[slot_id];
            print("xHCI: Transfer event on slot ", cyan);
            char buf[16];
            buf[0] = '\0';
            str_append_uint(buf, slot_id);
            print(buf, cyan);
            print(" ep ", cyan);
            buf[0] = '\0';
            str_append_uint(buf, ep_id);
            print(buf, cyan);
            print("\n", cyan);
            if (dev->type == 1) {
                usb_keyboard_handle_report((usb_kb_report_t*)event->parameter);
            } else if (dev->type == 2) {
                usb_mouse_interrupt_handler((uint8_t*)event->parameter, event->status & 0xFFFF);
            }
        }
        xhci_ctrl.event_ring_index = (xhci_ctrl.event_ring_index + 1) % 256;
        xhci_write64(&xhci_ctrl.rt_regs->erdp[0], (uint64_t)&xhci_ctrl.event_ring[xhci_ctrl.event_ring_index]);
    }
}