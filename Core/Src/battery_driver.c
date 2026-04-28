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
	    uint8_t reg_val;

	    // 1. Verify Communication
	    status = BQ_ReadReg(BQ_REG_STATUS, &reg_val);
	    if (status != HAL_OK) {
	        printf("BQ25756E: I2C FAILED.\r\n");
	        return status;
	    }

	    // 2. DISABLE WATCHDOG (Crucial!)
	    // Register 0x0E: Bits [5:4] control the Watchdog. 00 = Disable.
	    // We also set other bits to 0 to ensure a clean slate.
	    status = BQ_WriteReg(0x0E, 0x00);
	    if (status != HAL_OK) return status;

	    // 3. Set Input Current Limit (IINDPM)
	    // If this is 0, the charger won't pull any power.
	    // Setting to ~2000mA (assuming 50mA steps, check datasheet for your specific R_sns)
	    status = BQ_WriteReg(BQ_REG_INPUT_CURRENT, (2000 / 50));
	    if (status != HAL_OK) return status;

	    // 4. Set Charge Current
	    uint8_t current_reg = (uint8_t)(charge_current_mA / 50);
	    status = BQ_WriteReg(BQ_REG_CHARGE_CURRENT, current_reg);
	    if (status != HAL_OK) return status;

	    // 5. Set Charge Voltage
	    uint8_t voltage_reg = (uint8_t)(charge_voltage_mV / 16);
	    status = BQ_WriteReg(BQ_REG_CHARGE_VOLTAGE, voltage_reg);
	    if (status != HAL_OK) return status;

	    // 6. Clear Faults
	    // Many TI chargers require you to read the fault registers to clear the latched state
	    uint8_t f0, f1;
	    BQ_ReadFaults(&f0, &f1);

	    printf("BQ25756E: Configured. Watchdog Disabled. Faults Cleared.\r\n");
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
    /*uint8_t status, fault0, fault1;

    if (BQ_ReadReg(BQ_REG_STATUS, &status) == HAL_OK) {
        printf("BQ Status Reg: 0x%02X\r\n", status);
        if ((status & BQ_STATUS_CHARGING) == BQ_STATUS_CHARGING)
            printf(" Currently charging\r\n");
        else
            printf(" Not charging\r\n");
    }

    if (BQ_ReadFaults(&fault0, &fault1) == HAL_OK) {
        if (fault0 || fault1) {
            printf("  FAULT_0=0x%02X  FAULT_1=0x%02X\r\n", fault0, fault1);
        } else {
            printf("  No faults.\r\n");
        }
    }*/
	uint8_t status_reg;
	BQ_ReadReg(BQ_REG_STATUS, &status_reg);

	// Isolate bits 4 and 3
	uint8_t charge_bits = (status_reg >> 3) & 0x03;

	if (charge_bits > 0) {
	    printf("Charging! Phase: %u\n", charge_bits);
	} else {
	    printf("Not charging.\n");
	}
}

uint16_t BQ_GetBatteryVoltage_mV(void) {
    uint8_t data[2];
    if (HAL_I2C_Mem_Read(&hi2c2, BQ25756E_I2C_ADDR, 0x26, I2C_MEMADD_SIZE_8BIT, data, 2, 100) == HAL_OK) {
        uint16_t raw_val = (data[1] << 8) | data[0];

        // 2mV per LSB, so just multiply by 2
        return raw_val * 2;
    }
    return 0;
}
