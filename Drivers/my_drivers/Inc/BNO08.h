/*
 * BNO08.h
 *
 *  Created on: Apr 15, 2026
 *      Author: julia
 */

#ifndef MY_DRIVERS_INC_BNO08_H_
#define MY_DRIVERS_INC_BNO08_H_

#include "stm32L4xx_hal.h" // Adjust based on your specific STM32 series

#include "main.h" // Ensures SPI_HandleTypeDef is defined

// Standard BNO08x SHTP constants
#define SENSOR_REPORTID_ROTATION_VECTOR 0x05

#define BNO_CS_LOW()     HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4,  GPIO_PIN_RESET)
#define BNO_CS_HIGH()    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4,  GPIO_PIN_SET)
#define BNO_RESET_LOW()  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, GPIO_PIN_RESET)
#define BNO_RESET_HIGH() HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, GPIO_PIN_SET)
#define BNO_INT_READ()   HAL_GPIO_ReadPin(GPIOC,  GPIO_PIN_13)
#define BNO_WAKE_LOW()   HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8,  GPIO_PIN_RESET)
#define BNO_WAKE_HIGH()  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8,  GPIO_PIN_SET)


typedef struct {
    float x, y, z, w;
} BNO086_Rotation_t;

// Function prototypes
HAL_StatusTypeDef BNO086_SPI_Init(SPI_HandleTypeDef *hspi);
void BNO086_SPI_ReadRotation(SPI_HandleTypeDef *hspi, BNO086_Rotation_t *rot);
void BNO086_SendSetFeatureCommand(SPI_HandleTypeDef *hspi);
void BNO086_SPI_ClearPacket(SPI_HandleTypeDef *hspi);

#endif /* MY_DRIVERS_INC_BNO08_H_ */
