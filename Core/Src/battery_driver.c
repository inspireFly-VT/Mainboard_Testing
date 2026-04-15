#include "battery_driver.h"
#include "main.h"
#include <stdio.h>

// ─────────────────────────────────────────────
// Low-level register read/write
// ─────────────────────────────────────────────

HAL_StatusTypeDef BQ_ReadReg(uint8_t reg, uint8_t *data)
{
    // Ask the chip for 1 byte from register 'reg', timeout = 100ms
    return HAL_I2C_Mem_Read(
        &hi2c2,
        BQ25756E_I2C_ADDR,
        reg,
        I2C_MEMADD_SIZE_8BIT,
        data,
        1,
        100
    );
}

HAL_StatusTypeDef BQ_WriteReg(uint8_t reg, uint8_t data)
{
    return HAL_I2C_Mem_Write(
        &hi2c2,
        BQ25756E_I2C_ADDR,
        reg,
        I2C_MEMADD_SIZE_8BIT,
        &data,
        1,
        100
    );
}

// ─────────────────────────────────────────────
// Initialization — call once after I2C is ready
// ─────────────────────────────────────────────

HAL_StatusTypeDef BQ_Init(uint16_t charge_current_mA, uint16_t charge_voltage_mV)
{
    HAL_StatusTypeDef status;

    // Sanity-check: try reading a register. If this fails,
    // your I2C wiring or address is wrong.
    uint8_t test;
    status = BQ_ReadReg(BQ_REG_STATUS, &test);
    if (status != HAL_OK) {
        printf("BQ25756E: I2C communication FAILED. Check wiring & address.\r\n");
        return status;
    }

    // --- Set charge current ---
    // The BQ25756E encodes current in steps of 50mA (register value = mA / 50)
    uint8_t current_reg = (uint8_t)(charge_current_mA / 50);
    status = BQ_WriteReg(BQ_REG_CHARGE_CURRENT, current_reg);
    if (status != HAL_OK) return status;

    // --- Set charge voltage ---
    // Encoded in steps of 16mV (register value = mV / 16)
    uint8_t voltage_reg = (uint8_t)(charge_voltage_mV / 16);
    status = BQ_WriteReg(BQ_REG_CHARGE_VOLTAGE, voltage_reg);
    if (status != HAL_OK) return status;

    printf("BQ25756E: Initialized. Current=%umA, Voltage=%umV\r\n",
           charge_current_mA, charge_voltage_mV);
    return HAL_OK;
}

// ─────────────────────────────────────────────
// Enable / Disable charging
// ─────────────────────────────────────────────

HAL_StatusTypeDef BQ_EnableCharging(void)
{
    uint8_t ctrl;
    HAL_StatusTypeDef s = BQ_ReadReg(BQ_REG_CHARGE_CTRL, &ctrl);
    if (s != HAL_OK) return s;

    ctrl |= BQ_CHARGE_ENABLE_BIT;        // Set the enable bit
    return BQ_WriteReg(BQ_REG_CHARGE_CTRL, ctrl);
}

HAL_StatusTypeDef BQ_DisableCharging(void)
{
    uint8_t ctrl;
    HAL_StatusTypeDef s = BQ_ReadReg(BQ_REG_CHARGE_CTRL, &ctrl);
    if (s != HAL_OK) return s;

    ctrl &= ~BQ_CHARGE_ENABLE_BIT;       // Clear the enable bit
    return BQ_WriteReg(BQ_REG_CHARGE_CTRL, ctrl);
}

// ─────────────────────────────────────────────
// Fault handling — call this when PE9 goes LOW
// ─────────────────────────────────────────────

HAL_StatusTypeDef BQ_ReadFaults(uint8_t *fault0, uint8_t *fault1)
{
    HAL_StatusTypeDef s;
    s = BQ_ReadReg(BQ_REG_FAULT_STATUS_0, fault0);
    if (s != HAL_OK) return s;
    return BQ_ReadReg(BQ_REG_FAULT_STATUS_1, fault1);
}

void BQ_PrintStatus(void)
{
    uint8_t status, fault0, fault1;

    if (BQ_ReadReg(BQ_REG_STATUS, &status) == HAL_OK) {
        printf("BQ Status Reg: 0x%02X\r\n", status);
        if ((status & BQ_STATUS_CHARGING) == BQ_STATUS_CHARGING)
            printf("  → Currently charging\r\n");
        else
            printf("  → Not charging\r\n");
    }

    if (BQ_ReadFaults(&fault0, &fault1) == HAL_OK) {
        if (fault0 || fault1) {
            printf("  FAULT_0=0x%02X  FAULT_1=0x%02X\r\n", fault0, fault1);
        } else {
            printf("  No faults.\r\n");
        }
    }
}
