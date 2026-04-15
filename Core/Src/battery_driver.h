#ifndef BQ25756E_H
#define BQ25756E_H

#include "stm32l4xx_hal.h"

// --- I2C Address ---
// ADDR pin = GND → 7-bit address is 0x6A
#define BQ25756E_I2C_ADDR       (0x6A << 1)  // HAL uses 8-bit addresses

// --- Key Register Addresses (from TI datasheet) ---
#define BQ_REG_CHARGE_CURRENT   0x02  // Charge current limit
#define BQ_REG_CHARGE_VOLTAGE   0x04  // Max charge voltage
#define BQ_REG_INPUT_CURRENT    0x06  // Input current limit (ILIM)
#define BQ_REG_CHARGE_CTRL      0x0F  // Charge control (enable/disable)
#define BQ_REG_STATUS           0x1B  // Charger status
#define BQ_REG_FAULT_STATUS_0   0x1C  // Fault flags (why it stopped)
#define BQ_REG_FAULT_STATUS_1   0x1D  // More fault flags

// --- Charge Control Bits ---
#define BQ_CHARGE_ENABLE_BIT    (1 << 5)

// --- Status Bits (REG 0x1B) ---
#define BQ_STATUS_CHARGING      (0x3 << 3)  // Bits [4:3] = charging phase

// --- Function Declarations ---
HAL_StatusTypeDef BQ_ReadReg(uint8_t reg, uint8_t *data);
HAL_StatusTypeDef BQ_WriteReg(uint8_t reg, uint8_t data);
HAL_StatusTypeDef BQ_Init(uint16_t charge_current_mA, uint16_t charge_voltage_mV);
HAL_StatusTypeDef BQ_EnableCharging(void);
HAL_StatusTypeDef BQ_DisableCharging(void);
HAL_StatusTypeDef BQ_ReadFaults(uint8_t *fault0, uint8_t *fault1);
void              BQ_PrintStatus(void);

#endif
