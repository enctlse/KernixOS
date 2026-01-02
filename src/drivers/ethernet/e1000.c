#include "e1000.h"
#include <kernel/pci/device.h>
#include <kernel/pci/config.h>
#include <kernel/include/ports.h>
#include <memory/main.h>
#include <string/string.h>
#define E1000_CTRL     0x0000
#define E1000_STATUS   0x0008
#define E1000_EERD     0x0014
#define E1000_TDBAL    0x3800
#define E1000_TDBAH    0x3804
#define E1000_TDLEN    0x3808
#define E1000_TDH      0x3810
#define E1000_TDT      0x3818
#define E1000_RDBAL    0x2800
#define E1000_RDBAH    0x2804
#define E1000_RDLEN    0x2808
#define E1000_RDH      0x2810
#define E1000_RDT      0x2818
#define E1000_TCTL     0x0400
#define E1000_RCTL     0x0100
#define E1000_CTRL_RST    (1 << 26)
#define E1000_CTRL_ASDE   (1 << 5)
#define E1000_CTRL_SLU    (1 << 6)
#define E1000_TCTL_EN     (1 << 1)
#define E1000_TCTL_PSP    (1 << 3)
#define E1000_RCTL_EN     (1 << 1)
#define E1000_RCTL_BAM    (1 << 15)
typedef struct {
    u64 addr;
    u16 length;
    u8 cso;
    u8 cmd;
    u8 status;
    u8 css;
    u16 special;
} __attribute__((packed)) tx_desc_t;
typedef struct {
    u64 addr;
    u16 length;
    u16 checksum;
    u8 status;
    u8 errors;
    u16 special;
} __attribute__((packed)) rx_desc_t;
#define TX_RING_SIZE 16
#define RX_RING_SIZE 16
static struct {
    pci_device_t *pci_dev;
    volatile u32 *mmio_base;
    tx_desc_t tx_ring[TX_RING_SIZE] __attribute__((aligned(16)));
    rx_desc_t rx_ring[RX_RING_SIZE] __attribute__((aligned(16)));
    u8 tx_buffers[TX_RING_SIZE][2048];
    u8 rx_buffers[RX_RING_SIZE][2048];
    u32 tx_head;
    u32 tx_tail;
    u32 rx_head;
    u32 rx_tail;
} e1000;
static inline u32 read_reg(u32 offset) {
    return e1000.mmio_base[offset / 4];
}
static inline void write_reg(u32 offset, u32 value) {
    e1000.mmio_base[offset / 4] = value;
}
void e1000_init(void) {
    e1000.pci_dev = pci_device_find_by_vendor(E1000_VENDOR_ID, E1000_DEVICE_ID);
    if (!e1000.pci_dev) {
        return;
    }
    pci_config_write_word(e1000.pci_dev->bus, e1000.pci_dev->device, e1000.pci_dev->function, PCI_COMMAND, 0x04);
    u32 bar0 = pci_config_read(e1000.pci_dev->bus, e1000.pci_dev->device, e1000.pci_dev->function, PCI_BAR0);
    bar0 &= ~0xF;
    e1000.mmio_base = (volatile u32*)bar0;
    write_reg(E1000_CTRL, read_reg(E1000_CTRL) | E1000_CTRL_RST);
    while (read_reg(E1000_CTRL) & E1000_CTRL_RST);
    memset(e1000.tx_ring, 0, sizeof(e1000.tx_ring));
    for (int i = 0; i < TX_RING_SIZE; i++) {
        e1000.tx_ring[i].addr = (u64)&e1000.tx_buffers[i];
        e1000.tx_ring[i].status = 1;
    }
    write_reg(E1000_TDBAL, (u32)&e1000.tx_ring);
    write_reg(E1000_TDBAH, 0);
    write_reg(E1000_TDLEN, sizeof(e1000.tx_ring));
    write_reg(E1000_TDH, 0);
    write_reg(E1000_TDT, 0);
    memset(e1000.rx_ring, 0, sizeof(e1000.rx_ring));
    for (int i = 0; i < RX_RING_SIZE; i++) {
        e1000.rx_ring[i].addr = (u64)&e1000.rx_buffers[i];
        e1000.rx_ring[i].status = 0;
    }
    write_reg(E1000_RDBAL, (u32)&e1000.rx_ring);
    write_reg(E1000_RDBAH, 0);
    write_reg(E1000_RDLEN, sizeof(e1000.rx_ring));
    write_reg(E1000_RDH, 0);
    write_reg(E1000_RDT, RX_RING_SIZE - 1);
    write_reg(E1000_TCTL, E1000_TCTL_EN | E1000_TCTL_PSP);
    write_reg(E1000_RCTL, E1000_RCTL_EN | E1000_RCTL_BAM);
    write_reg(E1000_CTRL, read_reg(E1000_CTRL) | E1000_CTRL_SLU);
}
void e1000_send_packet(const void *data, u32 len) {
    if (len > 1518) return;
    u32 tail = e1000.tx_tail;
    if (!(e1000.tx_ring[tail].status & 1)) return;
    memcpy(e1000.tx_buffers[tail], data, len);
    e1000.tx_ring[tail].length = len;
    e1000.tx_ring[tail].cmd = 0x09;
    e1000.tx_ring[tail].status = 0;
    e1000.tx_tail = (tail + 1) % TX_RING_SIZE;
    write_reg(E1000_TDT, e1000.tx_tail);
}
void e1000_receive_packet(void *buffer, u32 *len) {
    u32 head = e1000.rx_head;
    if (!(e1000.rx_ring[head].status & 1)) return;
    *len = e1000.rx_ring[head].length;
    memcpy(buffer, e1000.rx_buffers[head], *len);
    e1000.rx_ring[head].status = 0;
    e1000.rx_head = (head + 1) % RX_RING_SIZE;
    write_reg(E1000_RDT, (e1000.rx_head + RX_RING_SIZE - 1) % RX_RING_SIZE);
}