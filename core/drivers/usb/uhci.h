#ifndef UHCI_H
#define UHCI_H
#include <outputs/types.h>
#include "ehci.h"
#define UHCI_CLASS 0x0C
#define UHCI_SUBCLASS 0x03
#define UHCI_PROG_IF 0x00
#define UHCI_USBCMD 0x00
#define UHCI_USBSTS 0x02
#define UHCI_USBINTR 0x04
#define UHCI_FRNUM 0x06
#define UHCI_FLBASEADD 0x08
#define UHCI_SOFMOD 0x0C
#define UHCI_PORTSC1 0x10
#define UHCI_PORTSC2 0x12
typedef struct {
    uint32_t link;
    uint32_t status;
    uint32_t token;
    uint32_t buffer;
    uint64_t buffer_virt;
} __attribute__((packed)) uhci_td_t;
typedef struct {
    uint64_t base_addr;
    uint16_t io_base;
    uhci_td_t* td;
    uint32_t* frame_list;
    int initialized;
} uhci_controller_t;
typedef struct {
    uint8_t type;
    uint8_t slot_id;
    uint8_t port_id;
    uint8_t speed;
    uint8_t device_class;
    uint8_t device_subclass;
    uint8_t protocol;
    uint8_t max_packet_size;
    usb_device_descriptor_t device_desc;
    usb_config_descriptor_t config_desc;
    usb_interface_descriptor_t interface_desc;
    usb_endpoint_descriptor_t ep_desc[16];
    usb_hid_descriptor_t hid_desc;
    uint8_t report_desc[256];
    uint16_t report_desc_len;
    uhci_td_t* td;
} uhci_device_t;
int uhci_init(uint64_t base_addr);
void uhci_start();
void uhci_stop();
int uhci_reset();
void uhci_enumerate_devices();
uhci_device_t* uhci_get_device(uint8_t slot_id);
void uhci_handle_interrupt();
#endif