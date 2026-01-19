#ifndef XHCI_H
#define XHCI_H
#include <outputs/types.h>
#include <drivers/usb/ehci.h>
#define XHCI_CLASS 0x0C
#define XHCI_SUBCLASS 0x03
#define XHCI_PROG_IF 0x30
#define XHCI_CAPLENGTH 0x00
#define XHCI_HCIVERSION 0x02
#define XHCI_HCSPARAMS1 0x04
#define XHCI_HCSPARAMS2 0x08
#define XHCI_HCSPARAMS3 0x0C
#define XHCI_HCCPARAMS1 0x10
#define XHCI_DBOFF 0x14
#define XHCI_RTSOFF 0x18
#define XHCI_HCCPARAMS2 0x1C
#define XHCI_USBCMD 0x00
#define XHCI_USBSTS 0x04
#define XHCI_PAGESIZE 0x08
#define XHCI_DNCTRL 0x14
#define XHCI_CRCR 0x18
#define XHCI_DCBAAP 0x30
#define XHCI_CONFIG 0x38
#define XHCI_PORTSC(n) (0x400 + ((n) * 0x10))
#define XHCI_PORTPMSC(n) (0x404 + ((n) * 0x10))
#define XHCI_PORTLI(n) (0x408 + ((n) * 0x10))
#define XHCI_PORTHLPMC(n) (0x40C + ((n) * 0x10))
#define XHCI_MFINDEX 0x00
#define XHCI_IMAN(n) (0x20 + ((n) * 0x20))
#define XHCI_IMOD(n) (0x24 + ((n) * 0x20))
#define XHCI_ERSTSZ(n) (0x28 + ((n) * 0x20))
#define XHCI_ERSTBA(n) (0x30 + ((n) * 0x20))
#define XHCI_ERDP(n) (0x38 + ((n) * 0x20))
#define XHCI_TRB_TYPE_NORMAL 1
#define XHCI_TRB_TYPE_SETUP_STAGE 2
#define XHCI_TRB_TYPE_DATA_STAGE 3
#define XHCI_TRB_TYPE_STATUS_STAGE 4
#define XHCI_TRB_TYPE_LINK 6
#define XHCI_TRB_TYPE_ENABLE_SLOT 9
#define XHCI_TRB_TYPE_DISABLE_SLOT 10
#define XHCI_TRB_TYPE_ADDRESS_DEVICE 11
#define XHCI_TRB_TYPE_CONFIGURE_ENDPOINT 12
#define XHCI_TRB_TYPE_EVALUATE_CONTEXT 13
#define XHCI_TRB_TYPE_RESET_ENDPOINT 14
#define XHCI_TRB_TYPE_STOP_ENDPOINT 15
#define XHCI_TRB_TYPE_SET_TR_DEQUEUE 16
#define XHCI_TRB_TYPE_RESET_DEVICE 17
#define XHCI_TRB_TYPE_FORCE_EVENT 18
#define XHCI_TRB_TYPE_NEGOTIATE_BW 19
#define XHCI_TRB_TYPE_SET_LATENCY_TOL 20
#define XHCI_TRB_TYPE_GET_PORT_BW 21
#define XHCI_TRB_TYPE_FORCE_HEADER 22
#define XHCI_TRB_TYPE_NO_OP 23
#define XHCI_TRB_TYPE_TRANSFER_EVENT 32
#define XHCI_TRB_TYPE_COMMAND_COMPLETION 33
#define XHCI_TRB_TYPE_PORT_STATUS_CHANGE 34
#define XHCI_TRB_TYPE_BANDWIDTH_REQUEST 35
#define XHCI_TRB_TYPE_DOORBELL 36
#define XHCI_TRB_TYPE_HOST_CONTROLLER 37
#define XHCI_TRB_TYPE_DEVICE_NOTIFICATION 38
#define XHCI_TRB_TYPE_MFINDEX_WRAP 39
#define XHCI_SLOT_STATE_DISABLED 0
#define XHCI_SLOT_STATE_ENABLED 1
#define XHCI_SLOT_STATE_DEFAULT 2
#define XHCI_SLOT_STATE_ADDRESSED 3
#define XHCI_ENDPOINT_STATE_DISABLED 0
#define XHCI_ENDPOINT_STATE_RUNNING 1
#define XHCI_ENDPOINT_STATE_HALTED 2
#define XHCI_ENDPOINT_STATE_STOPPED 3
#define XHCI_ENDPOINT_STATE_ERROR 4
#define XHCI_USB_SPEED_LOW 1
#define XHCI_USB_SPEED_FULL 2
#define XHCI_USB_SPEED_HIGH 3
#define XHCI_USB_SPEED_SUPER 4
#define XHCI_USB_SPEED_SUPER_PLUS 5
typedef struct {
    uint32_t parameter;
    uint32_t status;
    uint32_t command;
    uint32_t control;
} __attribute__((packed)) xhci_trb_t;
typedef struct {
    uint32_t route_string;
    uint32_t speed;
    uint32_t tt_info;
    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t reserved3;
    uint32_t reserved4;
    uint32_t slot_state;
} __attribute__((packed)) xhci_slot_context_t;
typedef struct {
    uint32_t ep_state;
    uint32_t ep_type;
    uint32_t dequeue_ptr;
    uint32_t dequeue_length;
    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t reserved3;
    uint32_t reserved4;
} __attribute__((packed)) xhci_endpoint_context_t;
typedef struct {
    xhci_slot_context_t slot;
    xhci_endpoint_context_t ep[31];
} __attribute__((packed)) xhci_device_context_t;
typedef struct {
    uint64_t base;
    uint32_t size;
    uint32_t reserved;
} __attribute__((packed)) xhci_erst_entry_t;
typedef struct {
    uint32_t mfindex;
    uint32_t iman[1024];
    uint32_t imod[1024];
    uint32_t erstsz[1024];
    uint64_t erstba[1024];
    uint64_t erdp[1024];
} __attribute__((packed)) xhci_runtime_regs_t;
typedef struct {
    uint32_t caplength;
    uint32_t hciversion;
    uint32_t hcsparams1;
    uint32_t hcsparams2;
    uint32_t hcsparams3;
    uint32_t hccparams1;
    uint32_t dboff;
    uint32_t rtsoff;
    uint32_t hccparams2;
} __attribute__((packed)) xhci_cap_regs_t;
typedef struct {
    uint32_t usbcmd;
    uint32_t usbsts;
    uint32_t pagesize;
    uint32_t reserved1[5];
    uint32_t dnctrl;
    uint64_t crcr;
    uint32_t reserved2[4];
    uint64_t dcbaap;
    uint32_t config;
} __attribute__((packed)) xhci_op_regs_t;
typedef struct {
    uint64_t* dcbaap;
    xhci_device_context_t** contexts;
    xhci_trb_t* cmd_ring;
    uint32_t cmd_ring_index;
    xhci_trb_t* event_ring;
    uint32_t event_ring_index;
    xhci_erst_entry_t* erst;
    uint32_t max_slots;
    uint32_t max_ports;
    uint32_t context_size;
    uint32_t page_size;
    volatile xhci_cap_regs_t* cap_regs;
    volatile xhci_op_regs_t* op_regs;
    volatile xhci_runtime_regs_t* rt_regs;
    volatile uint32_t* db_regs;
} xhci_controller_t;
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
} xhci_device_t;
int xhci_init(uint64_t base_addr);
void xhci_start();
void xhci_stop();
int xhci_reset();
int xhci_get_max_slots();
int xhci_get_max_ports();
int xhci_enable_slot(uint8_t* slot_id);
int xhci_disable_slot(uint8_t slot_id);
int xhci_address_device(uint8_t slot_id, uint8_t port_id);
int xhci_configure_endpoint(uint8_t slot_id, uint8_t ep_index, uint8_t ep_type, uint16_t max_packet_size, uint8_t interval);
int xhci_setup_interrupt_transfer(uint8_t slot_id, uint8_t ep_index, uint64_t buffer, uint32_t length);
void xhci_handle_event();
void xhci_enumerate_devices();
xhci_device_t* xhci_get_device(uint8_t slot_id);
#endif