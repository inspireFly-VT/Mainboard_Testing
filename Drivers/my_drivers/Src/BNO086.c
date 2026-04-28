#include "../Inc/BNO086.h"

int bno086_init(bno086_dev_t *dev, bno086_hal_t *hal)
{
    if (dev == 0 || hal == 0) {
        return BNO086_ERR_SPI;
    }

    dev->hal = *hal;

    if (dev->hal.cs_set != 0) {
        dev->hal.cs_set(1);
    }

    return BNO086_OK;
}

int bno086_spi_write(bno086_dev_t *dev, uint8_t *data, uint16_t len)
{
    int ret;

    if (dev == 0 || data == 0 || dev->hal.spi_transfer == 0 || dev->hal.cs_set == 0) {
        return BNO086_ERR_SPI;
    }

    dev->hal.cs_set(0);
    ret = dev->hal.spi_transfer(data, 0, len);
    dev->hal.cs_set(1);

    return (ret == 0) ? BNO086_OK : BNO086_ERR_SPI;
}

int bno086_spi_read(bno086_dev_t *dev, uint8_t *data, uint16_t len)
{
    int ret;
    uint16_t i;
    uint8_t dummy[256];

    if (dev == 0 || data == 0 || dev->hal.spi_transfer == 0 || dev->hal.cs_set == 0) {
        return BNO086_ERR_SPI;
    }

    if (len > sizeof(dummy)) {
        return BNO086_ERR_SPI;
    }

    for (i = 0; i < len; i++) {
        dummy[i] = 0xFF;
    }

    dev->hal.cs_set(0);
    ret = dev->hal.spi_transfer(dummy, data, len);
    dev->hal.cs_set(1);

    return (ret == 0) ? BNO086_OK : BNO086_ERR_SPI;
}
