/*
 * BNO086.c
 *
 *  Created on: Mar 31, 2026
 *      Author: zaida
 */

#include "../Inc/BNO086.h"

// #include "bno086.h"

// ============================================================
//  Internal constants
// ============================================================

// How many ms to wait total for H_INTN before giving up
#define HINT_TIMEOUT_MS  500

// Executable channel response codes (datasheet Figure 1-27)
#define EXECUTABLE_RESET_COMPLETE  0x01

// ============================================================
//  Internal helper: wait for H_INTN to go LOW
//
//  H_INTN is active low. The device pulls it low when it has
//  data ready or has finished booting.
//
//  Returns BNO086_OK if pin went low within HINT_TIMEOUT_MS,
//  BNO086_ERR_TIMEOUT otherwise.
// ============================================================
static int wait_for_hint(bno086_dev_t *dev)
{
    for (int ms = 0; ms < HINT_TIMEOUT_MS; ms++) {
        if (dev->hal.get_h_intn() == 0) {  // 0 = LOW = asserted
            return BNO086_OK;
        }
        dev->hal.delay_ms(1);
    }
    return BNO086_ERR_TIMEOUT;
}

// ============================================================
//  bno086_wake
//
//  Wakes the device using PS0/WAKE pin (datasheet section 1.2.4.3)
//
//  Sequence from Figure 1-22:
//    1. Pull PS0/WAKE LOW  (assert wake)
//    2. Wait for H_INTN to assert LOW (device is awake)
//    3. Pull PS0/WAKE HIGH (release wake)
//
//  The device will de-assert H_INTN as soon as it sees CS go low,
//  so we leave H_INTN asserted here — the read_packet call that
//  follows will handle it.
// ============================================================
int bno086_wake(bno086_dev_t *dev)
{
    int ret;

    dev->hal.wake_set(0);       // assert WAKE low
    ret = wait_for_hint(dev);   // wait for device to respond
    dev->hal.wake_set(1);       // release WAKE high

    return ret;
}

// ============================================================
//  bno086_read_packet
//
//  SPI read sequence:
//
//  The key difference from I2C is that SPI is full duplex —
//  you have to clock out bytes (we send 0xFF as dummy) while
//  simultaneously clocking in data from the device.
//
//  We do this in two transfers:
//    Transfer 1: clock out 4 dummy bytes, receive 4-byte header
//                → tells us the total packet length
//    Transfer 2: clock out (len-4) dummy bytes, receive payload
//
//  Both transfers happen within a single CS assertion.
//  Deasserting CS in between would abort the packet.
//
//  Note: H_INTN is de-asserted by the device as soon as it
//  sees CS go low (datasheet section 6.5.4, tcsid = 800ns).
// ============================================================
int bno086_read_packet(bno086_dev_t *dev,
                       uint8_t  *channel_out,
                       uint8_t  *data_out,
                       uint16_t *len_out)
{
    int ret;

    // Fill TX buffer with 0xFF (SPI dummy bytes — we're receiving,
    // not sending, but we still have to clock something out)
    for (int i = 0; i < SHTP_HEADER_SIZE; i++) {
        dev->tx_buf[i] = 0xFF;
    }

    // Assert CS — this also causes device to de-assert H_INTN
    dev->hal.cs_set(0);

    // Transfer 1: receive the 4-byte SHTP header
    // Header layout (datasheet Figure 1-26):
    //   byte 0: length LSB
    //   byte 1: length MSB  (bit 15 = continuation, bits 14:0 = total length)
    //   byte 2: channel number
    //   byte 3: sequence number
    ret = dev->hal.spi_transfer(dev->tx_buf, dev->rx_buf, SHTP_HEADER_SIZE);
    if (ret != 0) {
        dev->hal.cs_set(1);  // always deassert CS on error
        return BNO086_ERR_SPI;
    }

    // Parse packet length — mask off continuation bit
    uint16_t packet_len = ((uint16_t)(dev->rx_buf[1] & 0x7F) << 8)
                          | dev->rx_buf[0];

    // Zero length means device has nothing to send right now
    if (packet_len == 0) {
        dev->hal.cs_set(1);
        *len_out     = 0;
        *channel_out = dev->rx_buf[2];
        return BNO086_OK;
    }

    // Sanity check
    if (packet_len > BNO086_MAX_PACKET_SIZE) {
        dev->hal.cs_set(1);
        return BNO086_ERR_BAD_PACKET;
    }

    // Store header in rx_buf from the beginning
    // We already have header bytes 0-3 in rx_buf[0..3]

    // Transfer 2: receive the payload (everything after the header)
    uint16_t payload_len = packet_len - SHTP_HEADER_SIZE;

    if (payload_len > 0) {
        // Dummy TX bytes for the payload transfer
        for (uint16_t i = 0; i < payload_len; i++) {
            dev->tx_buf[i] = 0xFF;
        }

        // Receive into rx_buf starting after where the header sits
        ret = dev->hal.spi_transfer(dev->tx_buf,
                                    dev->rx_buf + SHTP_HEADER_SIZE,
                                    payload_len);
        if (ret != 0) {
            dev->hal.cs_set(1);
            return BNO086_ERR_SPI;
        }
    }

    // Deassert CS — packet complete
    dev->hal.cs_set(1);

    // Give caller the channel and payload
    *channel_out = dev->rx_buf[2];
    *len_out     = payload_len;

    if (data_out != NULL && payload_len > 0) {
        for (uint16_t i = 0; i < payload_len; i++) {
            data_out[i] = dev->rx_buf[SHTP_HEADER_SIZE + i];
        }
    }

    return BNO086_OK;
}

// ============================================================
//  bno086_write_packet
//
//  SPI write sequence:
//
//  Build the full packet (header + payload) in tx_buf,
//  then assert CS, transfer everything in one shot, deassert CS.
//
//  We still receive into rx_buf during the transfer (SPI is full
//  duplex) but we discard what comes back during a write.
// ============================================================
int bno086_write_packet(bno086_dev_t *dev,
                        uint8_t   channel,
                        uint8_t  *data,
                        uint16_t  len)
{
    uint16_t total_len = len + SHTP_HEADER_SIZE;

    if (total_len > BNO086_MAX_PACKET_SIZE) {
        return BNO086_ERR_BAD_PACKET;
    }

    // Build SHTP header
    // Length field includes the 4 header bytes themselves
    dev->tx_buf[0] = (uint8_t)(total_len & 0xFF);          // length LSB
    dev->tx_buf[1] = (uint8_t)((total_len >> 8) & 0x7F);   // length MSB (no continuation)
    dev->tx_buf[2] = channel;
    dev->tx_buf[3] = dev->seq_num[channel]++;               // increment seq per channel

    // Copy payload after header
    for (uint16_t i = 0; i < len; i++) {
        dev->tx_buf[SHTP_HEADER_SIZE + i] = data[i];
    }

    // Assert CS, transfer, deassert CS
    dev->hal.cs_set(0);

    int ret = dev->hal.spi_transfer(dev->tx_buf, dev->rx_buf, total_len);

    dev->hal.cs_set(1);

    if (ret != 0) {
        return BNO086_ERR_SPI;
    }

    return BNO086_OK;
}

// ============================================================
//  bno086_init
//
//  Full SPI startup sequence (datasheet section 5.2):
//
//  Before reset, PS1 and PS0/WAKE must BOTH be HIGH to select
//  SPI mode. They are sampled at reset.
//
//  After reset the sequence is:
//    1. Wait for H_INTN to assert — boot complete (~94ms)
//    2. Read SHTP advertisement packet on channel 0
//    3. Read reset-complete on channel 1
//    4. Read SH-2 init message on channel 2
//
//  After this the device sits idle with all sensors disabled,
//  waiting for Set Feature Commands from the host.
// ============================================================
int bno086_init(bno086_dev_t *dev, uint8_t do_reset)
{
    int      ret;
    uint8_t  channel;
    uint8_t  payload[BNO086_MAX_PACKET_SIZE];
    uint16_t len;

    // Clear state
    for (int i = 0; i < 6; i++) {
        dev->seq_num[i] = 0;
    }
    dev->initialized = 0;

    // ----------------------------------------------------------
    // Step 1: Make sure CS and WAKE are idle HIGH before reset
    //
    // PS1 must also be HIGH (tie it to VDDIO on your board).
    // PS0/WAKE starts HIGH here so it is sampled HIGH at reset,
    // selecting SPI mode. After reset it becomes the WAKE signal.
    // ----------------------------------------------------------
    dev->hal.cs_set(1);    // CS idle high
    dev->hal.wake_set(1);  // WAKE/PS0 idle high — selects SPI mode at reset

    // ----------------------------------------------------------
    // Step 2: Optional hardware reset
    //
    // Hold NRST low for 10ms (min is 10ns per datasheet section
    // 6.5.3, we use 10ms to be safe), then release.
    // Device needs ~90ms (t1) + ~4ms (t2) after release before
    // it asserts H_INTN. Our wait_for_hint() timeout covers this.
    // ----------------------------------------------------------
    if (do_reset && dev->hal.reset_pin != NULL) {
        dev->hal.reset_pin(0);   // assert reset LOW
        dev->hal.delay_ms(10);
        dev->hal.reset_pin(1);   // release reset HIGH
        // Don't add a fixed delay here — just let wait_for_hint
        // poll until the device is actually ready
    }

    // ----------------------------------------------------------
    // Step 3: Wait for H_INTN to assert
    //
    // Device pulls H_INTN LOW when boot routine completes.
    // ----------------------------------------------------------
    ret = wait_for_hint(dev);
    if (ret != BNO086_OK) {
        return BNO086_ERR_TIMEOUT;
    }

    // ----------------------------------------------------------
    // Step 4: Read SHTP advertisement packet (channel 0)
    //
    // First packet after boot describes the channel map and
    // built-in applications. We read and discard it.
    // ----------------------------------------------------------
    ret = bno086_read_packet(dev, &channel, payload, &len);
    if (ret != BNO086_OK) {
        return ret;
    }

    // ----------------------------------------------------------
    // Step 5: Read reset-complete on channel 1
    //
    // The executable application sends 0x01 (reset complete)
    // on the executable channel after leaving reset state.
    // Datasheet Figure 1-27.
    // ----------------------------------------------------------

    // H_INTN will assert again when channel 1 message is ready
    ret = wait_for_hint(dev);
    if (ret != BNO086_OK) {
        return BNO086_ERR_TIMEOUT;
    }

    ret = bno086_read_packet(dev, &channel, payload, &len);
    if (ret != BNO086_OK) {
        return ret;
    }

    // ----------------------------------------------------------
    // Step 6: Read SH-2 initialization message on channel 2
    //
    // SH-2 sends an unsolicited init message on the control
    // channel after reset. Read and discard it.
    // After this the device is fully ready.
    // ----------------------------------------------------------
    ret = wait_for_hint(dev);
    if (ret != BNO086_OK) {
        return BNO086_ERR_TIMEOUT;
    }

    ret = bno086_read_packet(dev, &channel, payload, &len);
    if (ret != BNO086_OK) {
        return ret;
    }

    dev->initialized = 1;
    return BNO086_OK;
}
