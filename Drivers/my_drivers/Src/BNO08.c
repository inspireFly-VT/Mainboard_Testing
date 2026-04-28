///*
// * BNO08.c
// *
// *  Created on: Apr 15, 2026
// *      Author: julia
// */
//#include "../Inc/BNO08.h"
//#include <stdio.h>
//
//// Helper to toggle CS pin (Assuming PB12 for CS, adjust to your actual CS pin)
//// BNO08x.c - corrected pin definitions
//#define BNO_CS_LOW()    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4,  GPIO_PIN_RESET)
//#define BNO_CS_HIGH()   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4,  GPIO_PIN_SET)
//#define BNO_RESET_LOW() HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, GPIO_PIN_RESET)
//#define BNO_RESET_HIGH() HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, GPIO_PIN_SET)
//#define BNO_INT_READ()  HAL_GPIO_ReadPin(GPIOC,  GPIO_PIN_13)
//#define BNO_WAKE_LOW()  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8,  GPIO_PIN_RESET)
//#define BNO_WAKE_HIGH() HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8,  GPIO_PIN_SET)
//
////HAL_StatusTypeDef BNO086_SPI_Init(SPI_HandleTypeDef *hspi) {
////    // 1. Hard Reset
////    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET); // RST
////    HAL_Delay(10);
////    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
////    HAL_Delay(100);
////
////    // 2. Enable Rotation Vector (SHTP Command over SPI)
////    uint8_t cmd[] = {
////        21, 0, 2, 0,          // Header (Length 21, Chan 2)
////        0xFD, 0x05, 0, 0, 0,  // Set Feature, Rotation Vector
////        0, 0, 0, 0,           // Sensitivity
////        0xA0, 0x86, 0x01, 0,  // 100ms interval
////        0, 0, 0, 0            // Batch
////    };
////
////    BNO_CS_LOW();
////    HAL_StatusTypeDef res = HAL_SPI_Transmit(hspi, cmd, sizeof(cmd), 100);
////    BNO_CS_HIGH();
////
////    return res;
////}
//HAL_StatusTypeDef BNO086_SPI_Init(SPI_HandleTypeDef *hspi) {
//	BNO_WAKE_LOW();   // Force SPI mode
//	BNO_CS_HIGH();    // Deassert CS before reset
//
//	// Hard reset
//	BNO_RESET_LOW();
//	HAL_Delay(10);
//	BNO_RESET_HIGH();
//	HAL_Delay(100);
//
//	// 2. Wait for First Interrupt (Advertisement)
//	while (BNO_INT_READ() == GPIO_PIN_SET);
//		;
//	BNO086_SPI_ClearPacket(hspi); // Dummy read to clear the buffer
//
//	// 3. Wait for Second Interrupt (Reset Message)
//	while (BNO_INT_READ() == GPIO_PIN_SET)
//		;  // wait for INT to go LOW
//	BNO086_SPI_ClearPacket(hspi);
//
//	// 4. NOW send the enable command for Rotation Vector (Report ID 0x05)
//	uint8_t enableRotation[] = { 21, 0, 2, 0, // SHTP Header (Length 21, Channel 2)
//			0xFD,                 // Set Feature Command [cite: 184]
//			0x05,              // Feature Report ID: Rotation Vector [cite: 252]
//			0, 0, 0,              // Flags
//			0, 0, 0, 0,           // Change Sensitivity
//			0x80, 0x38, 0x01, 0, // Report Interval (80,000 microseconds = 80ms)
//			0, 0, 0, 0            // Batch Interval
//			};
//
//	BNO_CS_LOW();
//	HAL_SPI_Transmit(hspi, enableRotation, sizeof(enableRotation), 100);
//	BNO_CS_HIGH();
//
//	return HAL_OK;
//}
//
//void BNO086_SPI_ReadRotation(SPI_HandleTypeDef *hspi, BNO086_Rotation_t *rot) {
//	uint8_t header[4];
//	uint8_t payload[26];
//
//	BNO_CS_LOW();
//	// Read 4-byte SHTP Header
//	if (HAL_SPI_Receive(hspi, header, 4, 10) == HAL_OK) {
//		uint16_t len = ((header[1] << 8) | header[0]) & 0x7FFF;
//		if (len > 4 && len < 30) {
//			// Read remaining payload
//			HAL_SPI_Receive(hspi, payload, len - 4, 10);
//
//			// Check if it's the Rotation Vector report
////			if (payload[5] == SENSOR_REPORTID_ROTATION_VECTOR) {
////				int16_t raw_i = (payload[10] << 8) | payload[9];
////				int16_t raw_j = (payload[12] << 8) | payload[11];
////				int16_t raw_k = (payload[14] << 8) | payload[13];
////				int16_t raw_real = (payload[16] << 8) | payload[15];
//			// Verify these offsets match BNO086 datasheet Table 2-13
//			if (payload[5] == SENSOR_REPORTID_ROTATION_VECTOR) {
//			    int16_t raw_i    = (int16_t)((payload[10] << 8) | payload[9]);
//			    int16_t raw_j    = (int16_t)((payload[12] << 8) | payload[11]);
//			    int16_t raw_k    = (int16_t)((payload[14] << 8) | payload[13]);
//			    int16_t raw_real = (int16_t)((payload[16] << 8) | payload[15]);
//
//				rot->x = raw_i / 16384.0f;
//				rot->y = raw_j / 16384.0f;
//				rot->z = raw_k / 16384.0f;
//				rot->w = raw_real / 16384.0f;
//			}
//		}
//	}
//	BNO_CS_HIGH();
//}
//
//void BNO086_SendSetFeatureCommand(SPI_HandleTypeDef *hspi) {
//	// The Set Feature Command is a 21-byte SHTP packet
//	uint8_t setFeaturePayload[21] = { 21, 0, // Byte 0-1: Packet Length (LSB, MSB)
//			2,              // Byte 2: Channel 2 (SH-2 Control Channel)
//			0,              // Byte 3: Sequence Number (managed by sensor)
//			0xFD,           // Byte 4: Report ID for "Set Feature Command"
//			0x05,          // Byte 5: Feature Report ID (0x05 = Rotation Vector)
//			0,              // Byte 6: Feature Flags
//			0, 0, 0,   // Byte 7-9: Change Sensitivity (0 = report every sample)
//			0x80, 0x38, 0x01, 0, // Byte 10-13: Report Interval in microseconds
//								 // (0x013880 = 80,000us = 12.5Hz) [cite: 319]
//			0, 0, 0, 0,     // Byte 14-17: Batch Interval (0 = no batching)
//			0, 0, 0     // Byte 18-20: Sensor-specific configuration (reserved)
//			};
//
//	// Pull CS low, transmit, then pull CS high
//	BNO_CS_HIGH();
//	HAL_SPI_Transmit(hspi, setFeaturePayload, 21, 100);
//	BNO_CS_LOW();
//}
//
//void BNO086_SPI_ClearPacket(SPI_HandleTypeDef *hspi) {
//	uint8_t header[4];
//
//	// 1. Pull Chip Select LOW to start the transaction
//	BNO_CS_LOW();
//
//	// 2. Read the 4-byte SHTP Header (Length LSB, Length MSB, Channel, SeqNum)
//	// SHTP transmits Length as a 16-bit little-endian value[cite: 306, 312].
//	HAL_SPI_Receive(hspi, header, 4, 10);
//
//	uint16_t packetSize = (uint16_t) ((header[1] << 8) | header[0]);
//	packetSize &= 0x7FFF; // Remove the MSB if it is used for "continuation" flags
//
//	// 3. If there is more data in this packet beyond the header, read it to clear it
//	if (packetSize > 4) {
//		uint8_t dummyBuffer[256]; // Typical max SHTP packet is < 256 bytes
//		uint16_t remainingBytes = packetSize - 4;
//
//		if (remainingBytes > 256)
//			remainingBytes = 256; // Safety cap
//		HAL_SPI_Receive(hspi, dummyBuffer, remainingBytes, 10);
//	}
//
//	// 4. Pull Chip Select HIGH to end the transaction
//	BNO_CS_HIGH();;
//}

/*
 * BNO08.c
 *
 *  Created on: Apr 15, 2026
 *      Author: julia
 */
#include "../Inc/BNO08.h"
#include <stdio.h>

// Corrected pin definitions matching actual hardware pinout
#define BNO_CS_LOW()     HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4,  GPIO_PIN_RESET)
#define BNO_CS_HIGH()    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4,  GPIO_PIN_SET)
#define BNO_RESET_LOW()  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, GPIO_PIN_RESET)
#define BNO_RESET_HIGH() HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, GPIO_PIN_SET)
#define BNO_INT_READ()   HAL_GPIO_ReadPin(GPIOC,  GPIO_PIN_13)
#define BNO_WAKE_LOW()   HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8,  GPIO_PIN_RESET)
#define BNO_WAKE_HIGH()  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8,  GPIO_PIN_SET)

static uint8_t shtpSeqNum[6] = { 0 }; // Sequence number per channel

HAL_StatusTypeDef BNO086_SPI_Init(SPI_HandleTypeDef *hspi) {
	BNO_WAKE_LOW();   // Force SPI mode via PS0
	BNO_CS_HIGH();    // Deassert CS before reset

	// Hard reset
	BNO_RESET_LOW();
	HAL_Delay(10);
	BNO_RESET_HIGH();
	HAL_Delay(100);

	// Wait for First Interrupt (Advertisement packet)
	while (BNO_INT_READ() == GPIO_PIN_SET)
		;
	BNO086_SPI_ClearPacket(hspi);

	// Wait for Second Interrupt (Reset complete message)
	while (BNO_INT_READ() == GPIO_PIN_SET)
		;
	BNO086_SPI_ClearPacket(hspi);

	// Send enable command for Rotation Vector (Report ID 0x05)
	uint8_t enableRotation[] = { 21, 0, // SHTP Header: Packet Length (LSB, MSB)
			2,                   // Channel 2 (SH-2 Control Channel)
			shtpSeqNum[2]++,     // Sequence number for channel 2
			0xFD,                // Set Feature Command
			0x05,                // Feature Report ID: Rotation Vector
			0, 0, 0,             // Feature Flags
			0, 0, 0, 0,          // Change Sensitivity (0 = report every sample)
			0x80, 0x38, 0x01, 0, // Report Interval: 80,000us = 12.5Hz
			0, 0, 0, 0           // Batch Interval (0 = no batching)
			};

	BNO_CS_LOW();
	HAL_SPI_Transmit(hspi, enableRotation, sizeof(enableRotation), 100);
	BNO_CS_HIGH();

	return HAL_OK;
}

void BNO086_SPI_ReadRotation(SPI_HandleTypeDef *hspi, BNO086_Rotation_t *rot) {
	// Only read when sensor signals data is ready (INT goes LOW)
	if (BNO_INT_READ() == GPIO_PIN_SET)
		return;

	uint8_t header[4];
	uint8_t payload[26];

	BNO_CS_LOW();

	// Read 4-byte SHTP Header
	if (HAL_SPI_Receive(hspi, header, 4, 10) == HAL_OK) {
		uint16_t len = ((header[1] << 8) | header[0]) & 0x7FFF;

		if (len > 4 && len <= 30) {
			// Read remaining payload
			HAL_SPI_Receive(hspi, payload, len - 4, 10);

			// payload[5] is the Report ID (offset 9 from packet start, offset 5 from post-header)
			if (payload[5] == SENSOR_REPORTID_ROTATION_VECTOR) {
				int16_t raw_i = (int16_t) ((payload[10] << 8) | payload[9]);
				int16_t raw_j = (int16_t) ((payload[12] << 8) | payload[11]);
				int16_t raw_k = (int16_t) ((payload[14] << 8) | payload[13]);
				int16_t raw_real = (int16_t) ((payload[16] << 8) | payload[15]);

				rot->x = raw_i / 16384.0f;
				rot->y = raw_j / 16384.0f;
				rot->z = raw_k / 16384.0f;
				rot->w = raw_real / 16384.0f;
			}
		}
	}

	BNO_CS_HIGH();
}

void BNO086_SendSetFeatureCommand(SPI_HandleTypeDef *hspi) {
	uint8_t setFeaturePayload[21] = { 21, 0, // Byte 0-1: Packet Length (LSB, MSB)
			2,                   // Byte 2: Channel 2 (SH-2 Control Channel)
			shtpSeqNum[2]++,     // Byte 3: Sequence Number for channel 2
			0xFD,                // Byte 4: Report ID for Set Feature Command
			0x05,                // Byte 5: Feature Report ID (Rotation Vector)
			0,                   // Byte 6: Feature Flags
			0, 0, 0,          // Byte 7-9: Change Sensitivity (0 = every sample)
			0x80, 0x38, 0x01, 0, // Byte 10-13: Report Interval (80,000us = 12.5Hz)
			0, 0, 0, 0,          // Byte 14-17: Batch Interval (0 = no batching)
			0, 0, 0            // Byte 18-20: Sensor-specific config (reserved)
			};

	BNO_CS_LOW();
	HAL_SPI_Transmit(hspi, setFeaturePayload, 21, 100);
	BNO_CS_HIGH();
}

void BNO086_SPI_ClearPacket(SPI_HandleTypeDef *hspi) {
	uint8_t header[4];

	BNO_CS_LOW();

	// Read the 4-byte SHTP Header (Length LSB, Length MSB, Channel, SeqNum)
	HAL_SPI_Receive(hspi, header, 4, 10);

	uint16_t packetSize = (uint16_t) ((header[1] << 8) | header[0]);
	packetSize &= 0x7FFF; // Mask off continuation bit

	// Read and discard remaining bytes to clear the packet
	if (packetSize > 4) {
		uint8_t dummyBuffer[256];
		uint16_t remainingBytes = packetSize - 4;
		if (remainingBytes > 256)
			remainingBytes = 256; // Safety cap
		HAL_SPI_Receive(hspi, dummyBuffer, remainingBytes, 10);
	}

	BNO_CS_HIGH();
}
