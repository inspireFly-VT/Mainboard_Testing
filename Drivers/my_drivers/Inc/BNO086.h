#ifndef BNO086_H
#define BNO086_H

#include <stdint.h>

#define BNO086_OK      0
#define BNO086_ERR_SPI -1

typedef struct {
    int  (*spi_transfer)(uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len);
    void (*cs_set)(uint8_t state);
} bno086_hal_t;

typedef struct {
    bno086_hal_t hal;
} bno086_dev_t;

int bno086_init(bno086_dev_t *dev, bno086_hal_t *hal);
int bno086_spi_write(bno086_dev_t *dev, uint8_t *data, uint16_t len);
int bno086_spi_read(bno086_dev_t *dev, uint8_t *data, uint16_t len);

#endif
