#ifndef E1000_H
#define E1000_H
#include <stdint.h>
#define E1000_REG_CTRL        0x0000
#define E1000_REG_STATUS      0x0008
#define E1000_REG_EECD        0x0010
#define E1000_REG_EERD        0x0014
#define E1000_REG_CTRL_EXT    0x0018
#define E1000_REG_MDIC        0x0020
#define E1000_REG_FCAL        0x0028
#define E1000_REG_FCAH        0x002C
#define E1000_REG_FCT         0x0030
#define E1000_REG_VET         0x0038
#define E1000_REG_ICR         0x00C0
#define E1000_REG_ICS         0x00C8
#define E1000_REG_IMS         0x00D0
#define E1000_REG_IMC         0x00D8
#define E1000_REG_RCTL        0x0100
#define E1000_REG_FCTTV       0x0170
#define E1000_REG_TIPG        0x0410
#define E1000_REG_RDBAL       0x2800
#define E1000_REG_RDBAH       0x2804
#define E1000_REG_RDRLEN      0x2808
#define E1000_REG_RDH         0x2810
#define E1000_REG_RDT         0x2818
#define E1000_REG_RDTR        0x2820
#define E1000_REG_RADV        0x282C
#define E1000_REG_TDBAL       0x3800
#define E1000_REG_TDBAH       0x3804
#define E1000_REG_TDLEN       0x3808
#define E1000_REG_TDH         0x3810
#define E1000_REG_TDT         0x3818
#define E1000_REG_TCTL        0x0400
#define E1000_REG_TIPG        0x0410
#define E1000_REG_RAL         0x5400
#define E1000_REG_RAH         0x5404
typedef struct {
    uint64_t addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} __attribute__((packed)) e1000_rx_desc_t;
typedef struct {
    uint64_t addr;
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t status;
    uint8_t css;
    uint16_t special;
} __attribute__((packed)) e1000_tx_desc_t;
typedef struct {
    uint32_t io_base;
    uint8_t mac[6];
    e1000_rx_desc_t* rx_descs;
    e1000_tx_desc_t* tx_descs;
    uint8_t* rx_buffers;
    uint8_t* tx_buffers;
    int rx_cur;
    int tx_cur;
} e1000_device_t;
void e1000_init(uint32_t io_base);
extern e1000_device_t e1000_dev;
void e1000_send_packet(void* data, int len);
int e1000_receive_packet(void* buffer, int max_len);
#endif