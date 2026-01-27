#ifndef OHCI_H
#define OHCI_H
#include <outputs/types.h>
#include "ehci.h"
#define OHCI_CLASS 0x0C
#define OHCI_SUBCLASS 0x03
#define OHCI_PROG_IF 0x10
#define OHCI_HcRevision 0x00
#define OHCI_HcControl 0x04
#define OHCI_HcCommandStatus 0x08
#define OHCI_HcInterruptStatus 0x0C
#define OHCI_HcInterruptEnable 0x10
#define OHCI_HcInterruptDisable 0x14
#define OHCI_HcHCCA 0x18
#define OHCI_HcPeriodCurrentED 0x1C
#define OHCI_HcControlHeadED 0x20
#define OHCI_HcControlCurrentED 0x24
#define OHCI_HcBulkHeadED 0x28
#define OHCI_HcBulkCurrentED 0x2C
#define OHCI_HcDoneHead 0x30
#define OHCI_HcFmInterval 0x34
#define OHCI_HcFmRemaining 0x38
#define OHCI_HcFmNumber 0x3C
#define OHCI_HcPeriodicStart 0x40
#define OHCI_HcLSThreshold 0x44
#define OHCI_HcRhDescriptorA 0x48
#define OHCI_HcRhDescriptorB 0x4C
#define OHCI_HcRhStatus 0x50
#define OHCI_HcRhPortStatus 0x54
typedef struct {
    uint32_t flags;
    uint32_t tailp;
    uint32_t headp;
    uint32_t nexted;
} __attribute__((packed)) ohci_ed_t;
typedef struct {
    uint32_t flags;
    uint32_t cbp;
    uint32_t nexttd;
    uint32_t be;
} __attribute__((packed)) ohci_td_t;
typedef struct {
    uint32_t hcca[256];
} __attribute__((packed)) ohci_hcca_t;
typedef struct {
    uint32_t revision;
    uint32_t control;
    uint32_t cmdstatus;
    uint32_t intstatus;
    uint32_t intenable;
    uint32_t intdisable;
    uint32_t hcca;
    uint32_t periodcurrented;
    uint32_t controlheaded;
    uint32_t controlcurrented;
    uint32_t bulkheaded;
    uint32_t bulkcurrented;
    uint32_t donehead;
    uint32_t fminterval;
    uint32_t fmremaining;
    uint32_t fmnumber;
    uint32_t periodicstart;
    uint32_t lsthreshold;
    uint32_t rhdescriptora;
    uint32_t rhdescriptorb;
    uint32_t rhstatus;
    uint32_t rhportstatus[15];
} __attribute__((packed)) ohci_regs_t;
typedef struct {
    ohci_regs_t* regs;
    ohci_hcca_t* hcca;
    ohci_ed_t* interrupt_ed;
    ohci_td_t* interrupt_td;
    int initialized;
} ohci_controller_t;
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
    ohci_ed_t* ed;
} ohci_device_t;
int ohci_init(uint64_t base_addr);
void ohci_start();
void ohci_stop();
int ohci_reset();
void ohci_enumerate_devices();
ohci_device_t* ohci_get_device(uint8_t slot_id);
void ohci_handle_interrupt();
#endif