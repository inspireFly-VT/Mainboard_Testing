#ifndef BNO086_H
#define BNO086_H

#include <stdint.h>
#include <stddef.h>

// ============================================================
//  SHTP Channels (datasheet section 1.3.1)
// ============================================================
#define SHTP_CHAN_COMMAND      0   // SHTP command channel
#define SHTP_CHAN_EXECUTABLE   1   // reset / sleep / on commands
#define SHTP_CHAN_CONTROL      2   // sensor hub control (config)
#define SHTP_CHAN_REPORTS      3   // sensor input reports (non-wake)
#define SHTP_CHAN_WAKE_REPORTS 4   // wake sensor reports
#define SHTP_CHAN_GYRO_RV      5   // gyro rotation vector (low latency)

// ============================================================
//  SHTP Header size
//  Every packet starts with 4 bytes:
//  [len_lsb, len_msb, channel, seq_num]
// ============================================================
#define SHTP_HEADER_SIZE        4

// ============================================================
//  Max packet size we will ever try to read/write
// ============================================================
#define BNO086_MAX_PACKET_SIZE  256

// ============================================================
//  HAL function pointer struct — SPI version
//
//  You fill this in with YOUR MCU's functions before calling
//  bno086_init().
//
//  spi_transfer : full duplex SPI transfer.
//                 Send `len` bytes from `tx_buf` while
//                 simultaneously receiving into `rx_buf`.
//                 Either buffer may be NULL if you don't
//                 care about that direction.
//                 Returns 0 on success, non-zero on failure.
//                 Do NOT assert/deassert CS inside this function
//                 — the driver controls CS itself.
//
//  cs_set       : control the H_CSN (chip select) pin.
//                 state=0 → pull CS LOW  (select device, active low)
//                 state=1 → pull CS HIGH (deselect)
//
//  wake_set     : control the PS0/WAKE pin.
//                 state=0 → pull LOW  (assert wake, active low)
//                 state=1 → pull HIGH (idle)
//                 IMPORTANT: PS0 must be HIGH before and during
//                 reset along with PS1=HIGH to select SPI mode.
//                 After reset it becomes the WAKE signal.
//
//  reset_pin    : control the NRST pin.
//                 state=0 → pull LOW  (assert reset, active low)
//                 state=1 → pull HIGH (release reset)
//                 Set to NULL if you are not controlling reset.
//
//  delay_ms     : block for `ms` milliseconds.
//
//  get_h_intn   : read the H_INTN interrupt pin.
//                 return 0 if pin is LOW  (interrupt asserted)
//                 return 1 if pin is HIGH (no interrupt)
// ============================================================
typedef struct {
    int     (*spi_transfer) (uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len);
    void    (*cs_set)       (uint8_t state);
    void    (*wake_set)     (uint8_t state);
    void    (*reset_pin)    (uint8_t state);  // can be NULL
    void    (*delay_ms)     (uint32_t ms);
    uint8_t (*get_h_intn)   (void);
} bno086_hal_t;

// ============================================================
//  Device struct
//
//  One of these per physical sensor on your board.
//  Pass a pointer to this into every driver function.
// ============================================================
typedef struct {
    bno086_hal_t hal;
    uint8_t      seq_num[6];                      // sequence counter per channel
    uint8_t      tx_buf[BNO086_MAX_PACKET_SIZE];  // internal transmit buffer
    uint8_t      rx_buf[BNO086_MAX_PACKET_SIZE];  // internal receive buffer
    uint8_t      initialized;
} bno086_dev_t;

// ============================================================
//  Return codes
// ============================================================
#define BNO086_OK               0
#define BNO086_ERR_SPI         -1   // SPI transfer failed
#define BNO086_ERR_TIMEOUT     -2   // waited too long for H_INTN
#define BNO086_ERR_BAD_PACKET  -3   // packet length out of range
#define BNO086_ERR_NOT_INIT    -4   // called function before init

// ============================================================
//  Function declarations
// ============================================================

/*
 * bno086_init
 *
 * Full SPI startup sequence:
 *   1. Ensures WAKE and CS are idle (high) before anything
 *   2. Optionally toggles NRST to force a clean reset
 *   3. Waits for H_INTN to assert (device boot complete)
 *   4. Reads SHTP advertisement packet on channel 0
 *   5. Reads reset-complete message on channel 1
 *   6. Reads initialization message on channel 2
 *
 * dev      : pointer to bno086_dev_t — fill in hal before calling
 * do_reset : 1 = toggle reset pin first, 0 = skip reset
 *
 * Returns BNO086_OK on success, negative error code on failure.
 * You MUST call this before any other driver function.
 */
int bno086_init(bno086_dev_t *dev, uint8_t do_reset);

/*
 * bno086_wake
 *
 * Wakes the device from sleep via the PS0/WAKE pin.
 * Sequence: pull WAKE low → wait for H_INTN → release WAKE.
 * The device will assert H_INTN when awake and ready.
 *
 * Call this before bno086_read_packet if the device might be
 * sleeping. Safe to call even if device is already awake.
 *
 * Returns BNO086_OK or BNO086_ERR_TIMEOUT.
 */
int bno086_wake(bno086_dev_t *dev);

/*
 * bno086_read_packet
 *
 * Reads one SHTP packet from the device over SPI.
 *
 * Sequence:
 *   assert CS → transfer 4-byte header → parse length →
 *   transfer remaining bytes → deassert CS
 *
 * Does NOT wait for H_INTN internally — caller should check
 * H_INTN or call bno086_wake() before calling this.
 *
 * channel_out : filled with channel number of received packet
 * data_out    : filled with payload bytes (header stripped)
 * len_out     : filled with payload length in bytes
 *
 * Returns BNO086_OK on success, negative error code on failure.
 */
int bno086_read_packet(bno086_dev_t *dev,
                       uint8_t  *channel_out,
                       uint8_t  *data_out,
                       uint16_t *len_out);

/*
 * bno086_write_packet
 *
 * Writes one SHTP packet to the device over SPI.
 * Builds the 4-byte SHTP header automatically.
 *
 * Sequence:
 *   assert CS → transfer header + payload → deassert CS
 *
 * channel : SHTP channel to send on
 * data    : payload bytes to send
 * len     : number of payload bytes
 *
 * Returns BNO086_OK on success, negative error code on failure.
 */
int bno086_write_packet(bno086_dev_t *dev,
                        uint8_t   channel,
                        uint8_t  *data,
                        uint16_t  len);

#endif // BNO086_H
