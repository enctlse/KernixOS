#include "e1000.h"
#include <kernel/include/io.h>
#include <kernel/mem/kernel_memory/kernel_memory.h>
#include <string/string.h>
#include <kernel/communication/serial.h>

extern void* fs_kernel_memory;

#define RX_DESC_COUNT 32
#define TX_DESC_COUNT 32
#define RX_BUFFER_SIZE 2048
#define TX_BUFFER_SIZE 2048

e1000_device_t e1000_dev;

static uint32_t e1000_read_reg(uint32_t reg) {
    return inl(e1000_dev.io_base + reg);
}

static void e1000_write_reg(uint32_t reg, uint32_t value) {
    outl(e1000_dev.io_base + reg, value);
}

static uint32_t e1000_read_eeprom(uint8_t addr) {
    uint32_t data = 0;
    uint32_t tmp = 0;
    e1000_write_reg(E1000_REG_EERD, (addr << 8) | 1);
    int timeout = 10000;
    while (!((tmp = e1000_read_reg(E1000_REG_EERD)) & (1 << 4)) && timeout-- > 0);
    if (timeout <= 0) return 0;
    data = (tmp >> 16) & 0xFFFF;
    return data;
}

static void e1000_detect_eeprom() {
    e1000_write_reg(E1000_REG_EERD, 1);
    for (int i = 0; i < 1000; i++) {
        if (e1000_read_reg(E1000_REG_EERD) & 0x10) {
            return;
        }
    }
    // No EEPROM
}

static void e1000_read_mac() {
    if (e1000_read_reg(E1000_REG_EERD) & 0x10) {
        uint32_t mac_low = e1000_read_eeprom(0);
        uint32_t mac_high = e1000_read_eeprom(1);
        e1000_dev.mac[0] = mac_low & 0xFF;
        e1000_dev.mac[1] = (mac_low >> 8) & 0xFF;
        e1000_dev.mac[2] = (mac_low >> 16) & 0xFF;
        e1000_dev.mac[3] = (mac_low >> 24) & 0xFF;
        e1000_dev.mac[4] = mac_high & 0xFF;
        e1000_dev.mac[5] = (mac_high >> 8) & 0xFF;
    } else {
        // Read from RAL/RAH
        uint32_t ral = e1000_read_reg(E1000_REG_RAL);
        uint32_t rah = e1000_read_reg(E1000_REG_RAH);
        e1000_dev.mac[0] = ral & 0xFF;
        e1000_dev.mac[1] = (ral >> 8) & 0xFF;
        e1000_dev.mac[2] = (ral >> 16) & 0xFF;
        e1000_dev.mac[3] = (ral >> 24) & 0xFF;
        e1000_dev.mac[4] = rah & 0xFF;
        e1000_dev.mac[5] = (rah >> 8) & 0xFF;
    }
}

void e1000_init(uint32_t io_base) {
    e1000_dev.io_base = io_base;
    e1000_dev.rx_cur = 0;
    e1000_dev.tx_cur = 0;

    // Allocate descriptors and buffers
    e1000_dev.rx_descs = (e1000_rx_desc_t*)kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, sizeof(e1000_rx_desc_t) * RX_DESC_COUNT, 1);
    e1000_dev.tx_descs = (e1000_tx_desc_t*)kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, sizeof(e1000_tx_desc_t) * TX_DESC_COUNT, 1);
    e1000_dev.rx_buffers = (uint8_t*)kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, RX_BUFFER_SIZE * RX_DESC_COUNT, 1);
    e1000_dev.tx_buffers = (uint8_t*)kernel_memory_alloc((kernel_memory_t*)fs_kernel_memory, TX_BUFFER_SIZE * TX_DESC_COUNT, 1);

    if (!e1000_dev.rx_descs || !e1000_dev.tx_descs || !e1000_dev.rx_buffers || !e1000_dev.tx_buffers) {
        serial_puts("E1000: Failed to allocate memory\n");
        return;
    }

    // Reset device
    e1000_write_reg(E1000_REG_CTRL, e1000_read_reg(E1000_REG_CTRL) | 0x4000000);
    // Wait for reset with timeout
    int timeout = 10000;
    while ((e1000_read_reg(E1000_REG_CTRL) & 0x4000000) && timeout-- > 0);
    if (timeout <= 0) {
        serial_puts("E1000: Reset timeout\n");
        return;
    }

    // Read MAC
    e1000_read_mac();

    // Setup receive
    for (int i = 0; i < RX_DESC_COUNT; i++) {
        e1000_dev.rx_descs[i].addr = (uint64_t)&e1000_dev.rx_buffers[i * RX_BUFFER_SIZE];
        e1000_dev.rx_descs[i].status = 0;
    }
    e1000_write_reg(E1000_REG_RDBAL, (uint32_t)((uint64_t)e1000_dev.rx_descs & 0xFFFFFFFF));
    e1000_write_reg(E1000_REG_RDBAH, (uint32_t)((uint64_t)e1000_dev.rx_descs >> 32));
    e1000_write_reg(E1000_REG_RDRLEN, RX_DESC_COUNT * sizeof(e1000_rx_desc_t));
    e1000_write_reg(E1000_REG_RDH, 0);
    e1000_write_reg(E1000_REG_RDT, RX_DESC_COUNT - 1);
    e1000_write_reg(E1000_REG_RCTL, 0x400801C2); // Enable receive

    // Setup transmit
    for (int i = 0; i < TX_DESC_COUNT; i++) {
        e1000_dev.tx_descs[i].addr = (uint64_t)&e1000_dev.tx_buffers[i * TX_BUFFER_SIZE];
        e1000_dev.tx_descs[i].cmd = 0;
        e1000_dev.tx_descs[i].status = 1; // Descriptor done
    }
    e1000_write_reg(E1000_REG_TDBAL, (uint32_t)((uint64_t)e1000_dev.tx_descs & 0xFFFFFFFF));
    e1000_write_reg(E1000_REG_TDBAH, (uint32_t)((uint64_t)e1000_dev.tx_descs >> 32));
    e1000_write_reg(E1000_REG_TDLEN, TX_DESC_COUNT * sizeof(e1000_tx_desc_t));
    e1000_write_reg(E1000_REG_TDH, 0);
    e1000_write_reg(E1000_REG_TDT, 0);
    e1000_write_reg(E1000_REG_TCTL, 0x4010A); // Enable transmit
    e1000_write_reg(E1000_REG_TIPG, 0x60100A); // Inter-packet gap

    serial_puts("E1000 initialized\n");
}

void e1000_send_packet(void* data, int len) {
    if (len > TX_BUFFER_SIZE) return;

    int cur = e1000_dev.tx_cur;
    for (int i = 0; i < len; i++) {
        e1000_dev.tx_buffers[cur * TX_BUFFER_SIZE + i] = ((uint8_t*)data)[i];
    }
    e1000_dev.tx_descs[cur].length = len;
    e1000_dev.tx_descs[cur].cmd = 0xB; // End of packet, report status
    e1000_dev.tx_descs[cur].status = 0;

    e1000_dev.tx_cur = (e1000_dev.tx_cur + 1) % TX_DESC_COUNT;
    e1000_write_reg(E1000_REG_TDT, e1000_dev.tx_cur);
}

int e1000_receive_packet(void* buffer, int max_len) {
    int cur = e1000_dev.rx_cur;
    if (!(e1000_dev.rx_descs[cur].status & 0x1)) return 0; // No packet

    int len = e1000_dev.rx_descs[cur].length;
    if (len > max_len) len = max_len;
    for (int i = 0; i < len; i++) {
        ((uint8_t*)buffer)[i] = e1000_dev.rx_buffers[cur * RX_BUFFER_SIZE + i];
    }

    e1000_dev.rx_descs[cur].status = 0;
    e1000_dev.rx_cur = (e1000_dev.rx_cur + 1) % RX_DESC_COUNT;
    e1000_write_reg(E1000_REG_RDT, (e1000_dev.rx_cur + RX_DESC_COUNT - 1) % RX_DESC_COUNT);

    return len;
}