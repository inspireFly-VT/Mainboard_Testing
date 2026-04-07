#include "main.h"
#include "../../Drivers/my_drivers/Inc/BNO086.h"
#include <stdio.h>
#include <string.h>

/* Use the SPI peripheral connected to the BNO086 */
extern SPI_HandleTypeDef hspi1;

#define BNO_CS_GPIO_Port    GPIOC
#define BNO_CS_Pin          GPIO_PIN_8

#define BNO_WAKE_GPIO_Port  GPIOC
#define BNO_WAKE_Pin        GPIO_PIN_7

#define BNO_RST_GPIO_Port   GPIOE
#define BNO_RST_Pin         GPIO_PIN_14

#define BNO_HINT_GPIO_Port  GPIOE
#define BNO_HINT_Pin        GPIO_PIN_9

static bno086_dev_t imu;

static int stm_bno_spi_transfer(uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len)
{
    HAL_StatusTypeDef st;
    st = HAL_SPI_TransmitReceive(&hspi1, tx_buf, rx_buf, len, 100);
    return (st == HAL_OK) ? 0 : -1;
}

static void stm_bno_cs_set(uint8_t state)
{
    HAL_GPIO_WritePin(BNO_CS_GPIO_Port, BNO_CS_Pin,
                      state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void stm_bno_wake_set(uint8_t state)
{
    HAL_GPIO_WritePin(BNO_WAKE_GPIO_Port, BNO_WAKE_Pin,
                      state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void stm_bno_reset_set(uint8_t state)
{
    HAL_GPIO_WritePin(BNO_RST_GPIO_Port, BNO_RST_Pin,
                      state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void stm_bno_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

static uint8_t stm_bno_get_h_intn(void)
{
    GPIO_PinState s = HAL_GPIO_ReadPin(BNO_HINT_GPIO_Port, BNO_HINT_Pin);
    return (s == GPIO_PIN_SET) ? 1u : 0u;
}

static void bno_test_setup(void)
{
    memset(&imu, 0, sizeof(imu));

    imu.hal.spi_transfer = stm_bno_spi_transfer;
    imu.hal.cs_set       = stm_bno_cs_set;
    imu.hal.wake_set     = stm_bno_wake_set;
    imu.hal.reset_pin    = stm_bno_reset_set;
    imu.hal.delay_ms     = stm_bno_delay_ms;
    imu.hal.get_h_intn   = stm_bno_get_h_intn;
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();

    bno_test_setup();

    printf("Starting BNO086 init...\r\n");

    int ret = bno086_init(&imu, 1);

    if (ret == BNO086_OK) {
        printf("BNO086 init OK\r\n");
    } else {
        printf("BNO086 init FAILED: %d\r\n", ret);
    }

    while (1)
    {
        HAL_Delay(1000);
    }
}
