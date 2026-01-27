#ifndef EHCI_H
#define EHCI_H
#include <outputs/types.h>
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdUSB;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
} __attribute__((packed)) usb_device_descriptor_t;
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wTotalLength;
    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t iConfiguration;
    uint8_t bmAttributes;
    uint8_t bMaxPower;
} __attribute__((packed)) usb_config_descriptor_t;
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} __attribute__((packed)) usb_interface_descriptor_t;
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
} __attribute__((packed)) usb_endpoint_descriptor_t;
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdHID;
    uint8_t bCountryCode;
    uint8_t bNumDescriptors;
    uint8_t bReportDescriptorType;
    uint16_t wReportDescriptorLength;
} __attribute__((packed)) usb_hid_descriptor_t;
#define EHCI_CLASS 0x0C
#define EHCI_SUBCLASS 0x03
#define EHCI_PROG_IF 0x20
#define EHCI_USBCMD 0x00
#define EHCI_USBSTS 0x04
#define EHCI_USBINTR 0x08
#define EHCI_FRINDEX 0x0C
#define EHCI_CTRLDSSEGMENT 0x10
#define EHCI_PERIODICLISTBASE 0x14
#define EHCI_ASYNCLISTADDR 0x18
#define EHCI_CONFIGFLAG 0x40
#define EHCI_PORTSC(n) (0x44 + ((n) * 4))
typedef struct {
    uint32_t next;
    uint32_t alt_next;
    uint32_t token;
    uint32_t buffer[5];
} __attribute__((packed)) ehci_qtd_t;
typedef struct {
    uint32_t horizontal_link;
    uint32_t characteristics;
    uint32_t capabilities;
    uint32_t current_qtd;
    uint32_t next_qtd;
    uint32_t alt_next_qtd;
    uint32_t token;
    uint32_t buffer[5];
    uint32_t ext_buffer[5];
} __attribute__((packed)) ehci_qh_t;
typedef struct {
    uint32_t usbcmd;
    uint32_t usbsts;
    uint32_t usbintr;
    uint32_t frindex;
    uint32_t ctrldssegment;
    uint32_t periodiclistbase;
    uint32_t asynclistaddr;
    uint32_t reserved[9];
    uint32_t configflag;
    uint32_t ports[15];
} __attribute__((packed)) ehci_regs_t;
typedef struct {
    ehci_regs_t* regs;
    ehci_qh_t* async_qh;
    ehci_qh_t* periodic_qh[1024];
    uint32_t* frame_list;
    int max_ports;
    int initialized;
} ehci_controller_t;
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
    ehci_qh_t* qh;
} ehci_device_t;
int ehci_init(uint64_t base_addr);
void ehci_start();
void ehci_stop();
int ehci_reset();
void ehci_enumerate_devices();
ehci_device_t* ehci_get_device(uint8_t slot_id);
void ehci_handle_interrupt();
#endif