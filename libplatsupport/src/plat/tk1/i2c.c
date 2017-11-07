/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */
#include <stdbool.h>
#include <assert.h>

#include <platsupport/io.h>
#include <platsupport/i2c.h>
#include <platsupport/plat/i2c.h>

#include <utils/arith.h>
#include <utils/fence.h>
#include <utils/attribute.h>
#include "../../services.h"

/** @file TK1 I2C driver.
 *
 * This driver only supports master mode. It doesn't support acting as a slave.
 * It also doesn't support multi-master (masters other than itself connected
 * to the same bus).
 *
 * We also don't support UF mode (ultra-fast mode).
 *
 * To use, just follow the API in `arch_include/arm/platsupport/i2c.h`.
 *
 *  PREREQUISITES:
 * This driver currently assumes that the MMIO registers it accesses are mapped
 * as strongly ordered and uncached. The driver makes no attempts whatsoever at
 * managing the write buffer or managing ordering of reads and writes.
 *
 *  KNOWN BUGS:
 * * Attempting to read or write more than 32bytes will cause the driver to
 *   freeze. Specifically, the first 32 or so will be transmitted, but then
 *   the IRQ to tell the controller that the TX FIFO is empty will fail to come
 *   in, so the driver will just sit there waiting for the next IRQ.
 */

/* Notes from TK1 TRM:
 *  Section 33.0:
 * "The host interface runs on the APB clock (pclk). The APB clock is derived
 * from the system clock and can be 1.0, 1/2, 1/3 or 1/4 times the system clock
 * (sclk)"
 * "The I2C interface controller clock runs on a frequency up to 136 MHz."
 * "The 136 MHz clock is derived from PLLP. Even though you can mux between
 * PLLP, PLLC, PLLM and OSC clocks, PLLP is always selected and used in normal
 * operation"
 */

/* Notes from I2C specification rev 6:
 *
 *  Section 5.1, "Fast mode":
 *  "If the power supply to a Fast-mode device is switched off, the SDA and SCL
 *  I/O pins must be floating so that they do not obstruct the bus lines."
 *
 * I.e, mux them off and leave them floating when they are not switched on.
 */
#define CHECK_FOR_SDA_LOW

#define PREFIX "I2C %p: "

#define TK1I2C_DATA_MAX_NBYTES                      (4096)
#define TK1I2C_NBYTES_PER_FIFO_WORD                 (4)

#define TK1I2C_SLAVE_FLAGS_XFER_IN_PROGRESS_BIT     (BIT(1))

#define TK1I2C_BUS_CLEAR_STATUS_SDA_NORMAL_BIT      (BIT(0))

#define TK1I2C_BUS_CLEAR_CONFIG_IN_PROGRESS_BIT     (BIT(0))
#define TK1I2C_BUS_CLEAR_CONFIG_TERMINATE_IMMEDIATE_BIT (BIT(1))
#define TK1I2C_BUS_CLEAR_CONFIG_SEND_STOP_BIT       (BIT(2))
#define TK1I2C_BUS_CLEAR_CONFIG_THRESHOLD_SHIFT     (16)
#define TK1I2C_BUS_CLEAR_CONFIG_THRESHOLD_MASK      (0xFF)

#define TK1I2C_INTSTATUS_MMODE_RX_FIFO_DATA_REQ_BIT             (BIT(0))
#define TK1I2C_INTSTATUS_MMODE_TX_FIFO_DATA_REQ_BIT             (BIT(1))
#define TK1I2C_INTSTATUS_MMODE_ARBITRATION_LOST_BIT             (BIT(2))
#define TK1I2C_INTSTATUS_MMODE_NO_ACK_BIT                       (BIT(3))
#define TK1I2C_INTSTATUS_MMODE_RX_FIFO_UNR_BIT                  (BIT(4))
#define TK1I2C_INTSTATUS_MMODE_TX_FIFO_OVF_BIT                  (BIT(5))
#define TK1I2C_INTSTATUS_MMODE_ALL_PACKETS_XFER_COMPLETE_BIT    (BIT(6))
#define TK1I2C_INTSTATUS_MMODE_PACKET_XFER_COMPLETE_BIT         (BIT(7))

#define TK1I2C_INTSTATUS_SMODE_RX_FIFO_DATA_REQ_BIT             (BIT(16))
#define TK1I2C_INTSTATUS_SMODE_TX_FIFO_DATA_REQ_BIT             (BIT(17))
#define TK1I2C_INTSTATUS_SMODE_RX_FIFO_UNR_BIT                  (BIT(20))
#define TK1I2C_INTSTATUS_SMODE_TX_FIFO_OVF_BIT                  (BIT(21))
#define TK1I2C_INTSTATUS_SMODE_PACKET_XFER_COMPLETE_BIT         (BIT(22))
#define TK1I2C_INTSTATUS_SMODE_RX_BUFFER_FULL_BIT               (BIT(23))
#define TK1I2C_INTSTATUS_SMODE_TX_BUFFER_REQ_BIT                (BIT(24))
#define TK1I2C_INTSTATUS_SMODE_PACKET_XFER_ERROR_BIT            (BIT(25))
#define TK1I2C_INTSTATUS_SMODE_SWITCHED_WRITE2READ_BIT          (BIT(26))
#define TK1I2C_INTSTATUS_SMODE_SWITCHED_READ2WRITE_BIT          (BIT(27))
#define TK1I2C_INTSTATUS_SMODE_ACK_WITHHELD_BIT                 (BIT(28))

#define TK1I2C_INTMASK_MMODE_RX_FIFO_DATA_REQ_BIT               (BIT(0))
#define TK1I2C_INTMASK_MMODE_TX_FIFO_DATA_REQ_BIT               (BIT(1))
#define TK1I2C_INTMASK_MMODE_ARBITRATION_LOST_BIT               (BIT(2))
#define TK1I2C_INTMASK_MMODE_NO_ACK_BIT                         (BIT(3))
#define TK1I2C_INTMASK_MMODE_RX_FIFO_UNR_BIT                    (BIT(4))
#define TK1I2C_INTMASK_MMODE_TX_FIFO_OVF_BIT                    (BIT(5))
#define TK1I2C_INTMASK_MMODE_ALL_PACKETS_XFER_COMPLETE_BIT      (BIT(6))
#define TK1I2C_INTMASK_MMODE_PACKET_XFER_COMPLETE_BIT           (BIT(7))
#define TK1I2C_INTMASK_MMODE_BUS_CLEAR_DONE_BIT                 (BIT(11))

#define TK1I2C_INTMASK_SMODE_RX_FIFO_DATA_REQ_BIT               (BIT(16))
#define TK1I2C_INTMASK_SMODE_TX_FIFO_DATA_REQ_BIT               (BIT(17))
#define TK1I2C_INTMASK_SMODE_RX_FIFO_UNR_BIT                    (BIT(20))
#define TK1I2C_INTMASK_SMODE_TX_FIFO_OVF_BIT                    (BIT(21))
#define TK1I2C_INTMASK_SMODE_PACKET_XFER_COMPLETE_BIT           (BIT(22))
#define TK1I2C_INTMASK_SMODE_RX_BUFFER_FULL_BIT                 (BIT(23))
#define TK1I2C_INTMASK_SMODE_TX_BUFFER_REQ_BIT                  (BIT(24))
#define TK1I2C_INTMASK_SMODE_PACKET_XFER_ERROR_BIT              (BIT(25))
#define TK1I2C_INTMASK_SMODE_SWITCHED_WRITE2READ_BIT            (BIT(26))
#define TK1I2C_INTMASK_SMODE_SWITCHED_READ2WRITE_BIT            (BIT(27))
#define TK1I2C_INTMASK_SMODE_ACK_WITHHELD_BIT                   (BIT(28))

#define TK1I2C_CONFIG_LOAD_MASTER_BIT               (BIT(0))
#define TK1I2C_CONFIG_LOAD_SLAVE_BIT                (BIT(1))
#define TK1I2C_CONFIG_LOAD_TIMEOUT_BIT              (BIT(2))
#define COMMIT_MASTER_BIT                           (TK1I2C_CONFIG_LOAD_MASTER_BIT)
#define COMMIT_SLAVE_BIT                            (TK1I2C_CONFIG_LOAD_SLAVE_BIT)
#define COMMIT_TIMEOUT_BIT                          (TK1I2C_CONFIG_LOAD_TIMEOUT_BIT)

#define TK1I2C_FIFO_CONTROL_MMODE_TX_TRIG_SHIFT     (5)
#define TK1I2C_FIFO_CONTROL_MMODE_TX_TRIG_MASK      (0x7)
#define TK1I2C_FIFO_CONTROL_MMODE_RX_TRIG_SHIFT     (2)
#define TK1I2C_FIFO_CONTROL_MMODE_RX_TRIG_MASK      (0x7)

#define TK1I2C_FIFO_CONTROL_SMODE_TX_TRIG_SHIFT     (13)
#define TK1I2C_FIFO_CONTROL_SMODE_TX_TRIG_MASK      (0x7)
#define TK1I2C_FIFO_CONTROL_SMODE_RX_TRIG_SHIFT     (10)
#define TK1I2C_FIFO_CONTROL_SMODE_RX_TRIG_MASK      (0x7)

#define TK1I2C_FIFO_CONTROL_MMODE_FLUSH_TX_BIT      (BIT(1))
#define TK1I2C_FIFO_CONTROL_MMODE_FLUSH_RX_BIT      (BIT(0))
#define TK1I2C_FIFO_CONTROL_SMODE_FLUSH_TX_BIT      (BIT(9))
#define TK1I2C_FIFO_CONTROL_SMODE_FLUSH_RX_BIT      (BIT(8))

#define TK1I2C_CNFG_PACKET_MODE_EN_BIT              (BIT(10))
#define TK1I2C_CNFG_ADDRESS_10BIT_EN_BIT            (BIT(0))
#define TK1I2C_CNFG_2_SLAVE_BIT                     (BIT(4))
#define TK1I2C_CNFG_SEND_START_BIT                  (BIT(5))
#define TK1I2C_CNFG_NOACK_BIT                       (BIT(8))
#define TK1I2C_CNFG_BEGIN_XFER_BIT                  (BIT(9))
#define TK1I2C_CNFG_XFER_LENGTH_SHIFT               (1)
#define TK1I2C_CNFG_XFER_LENGTH_MASK                (0x7)
#define TK1I2C_CNFG_DEBOUNCE_COUNT_SHIFT            (12)
#define TK1I2C_CNFG_DEBOUNCE_COUNT_MASK             (0x7)

#define TK1I2C_SL_CNFG_SLAVE_EN_BIT                 (BIT(3))
#define TK1I2C_SL_CNFG_PACKET_MODE_EN_BIT           (BIT(4))
#define TK1I2C_SL_CNFG_FIFO_XFER_EN_BIT             (BIT(20))

#define TK1I2C_FIFO_STATUS_MMODE_TX_EMPTY_SHIFT     (4)
#define TK1I2C_FIFO_STATUS_MMODE_TX_EMPTY_MASK      (0xF)
#define TK1I2C_FIFO_STATUS_MMODE_RX_FULL_SHIFT      (0)
#define TK1I2C_FIFO_STATUS_MMODE_RX_FULL_MASK       (0xF)
#define TK1I2C_FIFO_STATUS_SMODE_TX_EMPTY_SHIFT     (20)
#define TK1I2C_FIFO_STATUS_SMODE_TX_EMPTY_MASK      (0xF)
#define TK1I2C_FIFO_STATUS_SMODE_RX_FULL_SHIFT      (16)
#define TK1I2C_FIFO_STATUS_SMODE_RX_FULL_MASK       (0xF)

#define TK1I2C_FIFO_DEPTH                           (8)

/* These constants are used by tk1_i2c_prepare_mmode_xfer_headers().
 * They are the values for the bit positions in the header words used by the
 * controller.
 */
#define TK1I2C_TXPKT0_PKTTYPE_SHIFT              (0)
#define TK1I2C_TXPKT0_PKTTYPE_MASK               (0x7)

#define TK1I2C_TXPKT0_PROTOCOL_SHIFT             (4)
#define TK1I2C_TXPKT0_PROTOCOL_MASK              (0xF)
#define TK1I2C_TXPKT0_PROTOCOL_I2C               (1)

#define TK1I2C_TXPKT0_CONTROLLER_ID_SHIFT        (12)
#define TK1I2C_TXPKT0_CONTROLLER_ID_MASK         (0xF)

#define TK1I2C_TXPKT0_PKTID_SHIFT                (16)
#define TK1I2C_TXPKT0_PKTID_MASK                 (0xFF)

#define TK1I2C_TXPKT0_PROTHDR_SIZE_SHIFT         (28)
#define TK1I2C_TXPKT0_PROTHDR_SIZE_MASK          (0x3)

#define TK1I2C_TXPKT1_PAYLOAD_SIZE_SHIFT         (0)
#define TK1I2C_TXPKT1_PAYLOAD_SIZE_MASK          (0xFFF)

#define TK1I2C_I2CPKT_SLAVE_ADDR_10BIT_SHIFT     (0)
#define TK1I2C_I2CPKT_SLAVE_ADDR_10BIT_MASK      (0x3FF)

#define TK1I2C_I2CPKT_SLAVE_ADDR_7BIT_SHIFT      (1)
#define TK1I2C_I2CPKT_SLAVE_ADDR_7BIT_MASK       (0x7F)

#define TK1I2C_I2CPKT_HS_MASTER_ADDR_SHIFT       (12)
#define TK1I2C_I2CPKT_HS_MASTER_ADDR_MASK        (0x7)

#define TK1I2C_I2CPKT_CONTINUE_XFER_BIT             (BIT(15))
#define TK1I2C_I2CPKT_SEND_REPEAT_START_NOT_STOP_BIT    (BIT(16))
#define TK1I2C_I2CPKT_IRQ_ON_COMPLETION_BIT         (BIT(17))
#define TK1I2C_I2CPKT_10_BIT_ADDRESS_BIT            (BIT(18))
#define TK1I2C_I2CPKT_READ_XFER_BIT                 (BIT(19))
#define TK1I2C_I2CPKT_SEND_START_BYTE_BIT           (BIT(20))
#define TK1I2C_I2CPKT_CONTINUE_ON_NACK_BIT          (BIT(21))
#define TK1I2C_I2CPKT_HS_MODE_BIT                   (BIT(22))
#define TK1I2C_I2CPKT_RESPONSE_PKT_ENABLE_BIT       (BIT(24))
#define TK1I2C_I2CPKT_RESPONSE_PKT_FREQ_BIT         (BIT(25))

/* These next few #defines pertain to tk1_i2c_set_speed() and its helper
 * functions tk1_i2c_calc_divisor_value_for() and
 * tk1_i2c_calc_baud_resulting_from().
 */
#define TK1_CAR_PLLP_INPUT_FREQ_HZ              (408000000)
#define TK1_CAR_PLLP_DIVISOR                    (1)

#define TK1I2C_IFACE_TIMING0_TLOW_SHIFT         (0)
#define TK1I2C_IFACE_TIMING0_TLOW_MASK          (0x3F)
#define TK1I2C_IFACE_TIMING0_THIGH_SHIFT        (8)
#define TK1I2C_IFACE_TIMING0_THIGH_MASK         (0x3F)

#define TK1I2C_CLK_DIVISOR_STD_FAST_MODE_SHIFT  (16)
#define TK1I2C_CLK_DIVISOR_STD_FAST_MODE_MASK   (0xFFFF)

#define TK1I2C_HS_IFACE_TIMING0_TLOW_SHIFT         (0)
#define TK1I2C_HS_IFACE_TIMING0_TLOW_MASK          (0x3F)
#define TK1I2C_HS_IFACE_TIMING0_THIGH_SHIFT        (8)
#define TK1I2C_HS_IFACE_TIMING0_THIGH_MASK         (0x3F)

#define TK1I2C_CLK_DIVISOR_HS_MODE_SHIFT        (0)
#define TK1I2C_CLK_DIVISOR_HS_MODE_MASK         (0xFFFF)

enum i2c_xfer_mode {
    I2C_MODE_MASTER,
    I2C_MODE_SLAVE
};

typedef volatile struct tk1_i2c_regs_ {
    uint32_t cnfg;
    uint32_t cmd_addr0, cmd_addr1;
    uint32_t cmd_data0, cmd_data1;
    PAD_STRUCT_BETWEEN(0x10, 0x1c, uint32_t);
    uint32_t status;
    PAD_STRUCT_BETWEEN(0x1c, 0x20, uint32_t);
    uint32_t sl_cnfg;
    uint32_t sl_rcvd, sl_status;
    uint32_t sl_addr0, sl_addr1;
    uint32_t tlow_sext;
    PAD_STRUCT_BETWEEN(0x34, 0x3c, uint32_t);
    uint32_t sl_delay_count;
    uint32_t sl_interrupt_mask, sl_interrupt_source, sl_interrupt_set;
    PAD_STRUCT_BETWEEN(0x48, 0x50, uint32_t);
    uint32_t tx_packet_fifo, rx_fifo, packet_transfer_status;
    uint32_t fifo_control, fifo_status;
    uint32_t interrupt_mask, interrupt_status;
    uint32_t clk_divisor;
    uint32_t interrupt_source, interrupt_set;
    uint32_t sl_tx_packet_fifo, sl_rx_fifo, sl_packet_status;
    uint32_t bus_clear_config, bus_clear_status;
    uint32_t config_load;
    PAD_STRUCT_BETWEEN(0x8c, 0x94, uint32_t);
    uint32_t interface_timing0, interface_timing1;
    uint32_t hs_interface_timing0, hs_interface_timing1;
} tk1_i2c_regs_t;

typedef struct tk1_i2c_state_ {
    tk1_i2c_regs_t *regs;
    int controller_id;

    struct {
        int hsmode_master_address;
        int slave_id;
        bool is_write;
        uint8_t *buff;
        size_t nbytes;
        ssize_t xfer_cursor;
    } master;
    struct {
        int self_id;
        bool is_write;
        uint8_t *buff;
        size_t nbytes;
        ssize_t xfer_cursor;
    } slave;
} tk1_i2c_state_t;

static inline tk1_i2c_state_t *
tk1_i2c_get_state(i2c_bus_t *ib)
{
    assert(ib != NULL);
    assert(ib->priv != NULL);
    return (tk1_i2c_state_t *)ib->priv;
}

static inline tk1_i2c_regs_t *
tk1_i2c_get_priv(i2c_bus_t *ib)
{
    assert(tk1_i2c_get_state(ib)->regs != NULL);
    return (tk1_i2c_regs_t *)tk1_i2c_get_state(ib)->regs;
}

typedef struct tk1_i2c_pktheaders_ {
    uint32_t io0;
    uint32_t io1;
    uint32_t i2c;
} tk1_i2c_pktheaders_t;

static void
tk1_i2c_set_packet_mode(i2c_bus_t *ib, bool enabled)
{
    tk1_i2c_regs_t *r = tk1_i2c_get_priv(ib);

    if (enabled) {
        r->cnfg |= TK1I2C_CNFG_PACKET_MODE_EN_BIT;
    } else {
        r->cnfg &= ~TK1I2C_CNFG_PACKET_MODE_EN_BIT;
    }
}

static void
tk1_i2c_config_commit(i2c_bus_t *ib, uint8_t which_configs)
{
    tk1_i2c_regs_t *r = tk1_i2c_get_priv(ib);
    uint32_t val = 0;

    if (which_configs & COMMIT_MASTER_BIT) {
        val |= TK1I2C_CONFIG_LOAD_MASTER_BIT;
    }
    if (which_configs & COMMIT_SLAVE_BIT) {
        val |= TK1I2C_CONFIG_LOAD_SLAVE_BIT;
    }
    if (which_configs & COMMIT_TIMEOUT_BIT) {
        val |= TK1I2C_CONFIG_LOAD_TIMEOUT_BIT;
    }

    r->config_load = val;

    /* The commit bits are hardware auto-cleared when the hardware is done
     * performing the operation.
     */
    while (r->config_load != 0) {
        /* Do nothing */
    }
}

static void
tk1_i2c_flush_fifos(i2c_bus_t *ib, enum i2c_xfer_mode mode)
{
    tk1_i2c_regs_t *r = tk1_i2c_get_priv(ib);
    uint32_t mask;

    if (mode == I2C_MODE_MASTER) {
        mask = TK1I2C_FIFO_CONTROL_MMODE_FLUSH_RX_BIT
               | TK1I2C_FIFO_CONTROL_MMODE_FLUSH_TX_BIT;
    } else {
        mask = TK1I2C_FIFO_CONTROL_SMODE_FLUSH_RX_BIT
               | TK1I2C_FIFO_CONTROL_SMODE_FLUSH_TX_BIT;
    }

    r->fifo_control |= mask;
    while ((r->fifo_control & mask) != 0) {
        /* Poll bits waiting for them to be auto-cleared by hardware. */
    }
}

static void
tk1_i2c_set_mode(i2c_bus_t *ib, enum i2c_xfer_mode mode)
{
    tk1_i2c_regs_t *r = tk1_i2c_get_priv(ib);

    if (mode == I2C_MODE_SLAVE) {
        r->sl_cnfg |= TK1I2C_SL_CNFG_SLAVE_EN_BIT;
    } else {
        r->sl_cnfg &= ~TK1I2C_SL_CNFG_SLAVE_EN_BIT;
    }

    tk1_i2c_config_commit(ib, COMMIT_MASTER_BIT);
}

static int
tk1_i2c_handle_arbitration_loss(i2c_bus_t *ib)
{
    UNUSED tk1_i2c_regs_t *r = tk1_i2c_get_priv(ib);

    /* I2C Reference rev6, 3.1.8: "Arbitration":
     *  "If a master also incorporates a slave function and it loses arbitration
     *  during the addressing stage, it is possible that the winning master is
     *  trying to address it. The losing master must therefore switch over
     *  immediately to its slave mode"
     */
    tk1_i2c_set_mode(ib, I2C_MODE_SLAVE);
    return 0;
}

static int
tk1_i2c_handle_nack(i2c_bus_t *ib)
{
    /* I2C Reference rev6, 33.4.4: "Error Handling":
     *  "When an error occurs due to NACK, a stop condition is put on the bus
     *  and an interrupt is generated.
     *  The status register is updated with the current error packet ID at which
     *  the error occured.
     *  Software should reset the controller.
     *  In case of a DMA transfer, DMA needs to be restarted."
     *
     * ^ This handling makes sense, but only if we are the master-sender.
     * If we are the slave-sender, we should not be sending a STOP signal.
     *
     *  "In packet mode ... if an error occurs during packet transfer then the
     *  controller should be reset ... and the entire packet needs to be resent
     *  since there is no accurate way of knowing how many bytes made it to the
     *  I2C slave."
     *
     * I believe that the controller which must be reset is the receiver and
     * not the transmitter, in this case, because the receiver may be held in
     * limbo expecting a resend. There is no reason to think the transmitter
     * needs to be reset.
     */
    UNUSED tk1_i2c_regs_t *r = tk1_i2c_get_priv(ib);
    return 0;
}

static bool
tk1_i2c_bus_is_locked_up(i2c_bus_t *ib)
{
    tk1_i2c_regs_t *r = tk1_i2c_get_priv(ib);

    return !(r->bus_clear_status & TK1I2C_BUS_CLEAR_STATUS_SDA_NORMAL_BIT);
}

/** Forces CLK pulses down the CLK line to trick a slave device that is holding
 * SDA low, to release SDA eventually. The slave is expected to release SDA
 * within 9 CLK pulses.
 *
 * Reasons for an arbitration failure for the SDA line:
 *  * Line isn't muxed
 *  * High-Z is asserted
 *  * Actual bus lockup caused by slave holding SDA low.
 *
 * @param[in] send_stop_afterward   Whether or not to automatically append a
 *                                  STOP signal after the bus clear operation.
 * @return true if the bus clear operation was successful.
 */
static bool
tk1_i2c_bus_clear(i2c_bus_t *ib, bool send_stop_afterward)
{
    tk1_i2c_regs_t *r = tk1_i2c_get_priv(ib);

    ZF_LOGW(PREFIX"Performing bus clear operation.", r);

    while (r->bus_clear_config & TK1I2C_BUS_CLEAR_CONFIG_IN_PROGRESS_BIT) {
        /* Just in case the bit was set before we began. */
    }

    r->bus_clear_config = 9 << TK1I2C_BUS_CLEAR_CONFIG_THRESHOLD_SHIFT
                          | (send_stop_afterward
                             ? TK1I2C_BUS_CLEAR_CONFIG_SEND_STOP_BIT : 0);

    tk1_i2c_config_commit(ib, COMMIT_MASTER_BIT);

    r->bus_clear_config |= TK1I2C_BUS_CLEAR_CONFIG_IN_PROGRESS_BIT;
    while (r->bus_clear_config & TK1I2C_BUS_CLEAR_CONFIG_IN_PROGRESS_BIT) {
        /* Do nothing until the device clears the bit to signal that it's done.
         */
    };

    return !tk1_i2c_bus_is_locked_up(ib);
}

static const uint32_t i2c_speed_freqs[] = {
    [I2C_SLAVE_SPEED_STANDARD] = 100000,
    [I2C_SLAVE_SPEED_FAST] = 400000,
    [I2C_SLAVE_SPEED_FASTPLUS] = 1000000,
    [I2C_SLAVE_SPEED_HIGHSPEED] = 3400000
};

static const char *i2c_speed_names[] = {
    [I2C_SLAVE_SPEED_STANDARD] = "standard",
    [I2C_SLAVE_SPEED_FAST] = "fast",
    [I2C_SLAVE_SPEED_FASTPLUS] = "fast+",
    [I2C_SLAVE_SPEED_HIGHSPEED] = "high-speed"
};

static int32_t
tk1_i2c_calc_divisor_value_for(i2c_bus_t *bus,
                               enum i2c_slave_speed speed,
                               uint32_t variant_constant)
{
    tk1_i2c_regs_t *r = tk1_i2c_get_priv(bus);
    uint32_t        tlow, thigh;
    uint32_t        target_scl_freq_hz;

    assert(variant_constant == 2 || variant_constant == 3);

    /* Section 33.1.2.1 has the instructions for setting the bus speed based
     * on the CAR controller's divisors.
     *
     * See also 33.3.1.
     *
     *  FIXME:
     *
     * This driver works with the assumption that the input clock source is
     * PLLP, and furthermore that the PLLP divisor value is 0.
     *
     * This will have to be the case until we have a proper driver API.
     *
     *  EXPLANATION:
     *
     * The SCL line's frequency is set according to the following equation:
     *  SCL = 408000000 / ((TLOW+THIGH+VC) * (CLK_DIVISOR + 1) * TK1_CAR_PLLP_DIVISOR)
     *
     * VC = variant_constant, either 2 or 3. This is just a magic number that
     *  is given in the TK1 manual with no explanation.
     * TLOW = the value in I2C_I2C_INTERFACE_TIMING_0_0.
     * THIGH = the value in I2C_I2C_INTERFACE_TIMING_0_0.
     * CLK_DIVISOR = the value in I2C_I2C_CLK_DIVISOR_REGISTER_0.
     * TK1_CAR_PLLP_DIVISOR = the divisor value in the clock and reset
     *  controller's PLLP clock source. Specifically, the resultant value that
     *  the divider will divide the input frequency by. I.e, N+1, not N.
     *  So for example, right now, this driver depends on us using '0' as the
     *  literal divisor value, and a value of '0' in the divisor register will
     *  cause the divider to divide the input frequency by 1 (N+1).
     *  So I2C_FREQENCY_DIVISOR in this equation should be 1, not 0.
     *
     * But we need to derive CLK_DIVISOR. So we rearrange that (algebra!) to
     * solve for CLK_DIVISOR, and get:
     *  CLK_DIVISOR = (408000000 / (SCL * TK1_CAR_PLLP_DIVISOR * (TLOW+THIGH+VC))) - 1
     */
    switch (speed) {
    case I2C_SLAVE_SPEED_HIGHSPEED:
        tlow = read_masked(&r->hs_interface_timing0,
                           (TK1I2C_HS_IFACE_TIMING0_TLOW_MASK << TK1I2C_HS_IFACE_TIMING0_TLOW_SHIFT))
                           >> TK1I2C_HS_IFACE_TIMING0_TLOW_SHIFT;
        thigh = read_masked(&r->hs_interface_timing0,
                           (TK1I2C_HS_IFACE_TIMING0_THIGH_MASK << TK1I2C_HS_IFACE_TIMING0_THIGH_SHIFT))
                           >> TK1I2C_HS_IFACE_TIMING0_THIGH_SHIFT;

        ZF_LOGD("Reading highspeed tlow/high: tLOW %d, tHIGH %d.",
                tlow, thigh);
        break;

    case I2C_SLAVE_SPEED_STANDARD:
    case I2C_SLAVE_SPEED_FAST:
    case I2C_SLAVE_SPEED_FASTPLUS:
        tlow = read_masked(&r->interface_timing0,
                           (TK1I2C_IFACE_TIMING0_TLOW_MASK << TK1I2C_IFACE_TIMING0_TLOW_SHIFT))
                           >> TK1I2C_IFACE_TIMING0_TLOW_SHIFT;
        thigh = read_masked(&r->interface_timing0,
                            (TK1I2C_IFACE_TIMING0_THIGH_MASK << TK1I2C_IFACE_TIMING0_THIGH_SHIFT))
                            >> TK1I2C_IFACE_TIMING0_THIGH_SHIFT;

        ZF_LOGD("Reading std/fast/f+ tlow/high: tLOW %d, tHIGH %d.",
                tlow, thigh);
        break;

    default:
        return -1;
    }

    ZF_LOGD("tLOW is %d, tHIGH is %d.", tlow, thigh);
    target_scl_freq_hz = i2c_speed_freqs[speed];
    return ((TK1_CAR_PLLP_INPUT_FREQ_HZ / target_scl_freq_hz)
        / (tlow+thigh+variant_constant)) - 1;
}

static uint32_t
tk1_i2c_calc_baud_resulting_from(i2c_bus_t *ib,
                                      uint32_t divisor,
                                      enum i2c_slave_speed speed,
                                      uint32_t vc)
{
    tk1_i2c_regs_t *r = tk1_i2c_get_priv(ib);
    uint32_t        tlow, thigh;

    switch (speed) {
    case I2C_SLAVE_SPEED_HIGHSPEED:
        tlow = read_masked(&r->hs_interface_timing0,
                           (TK1I2C_HS_IFACE_TIMING0_TLOW_MASK << TK1I2C_HS_IFACE_TIMING0_TLOW_SHIFT))
                           >> TK1I2C_HS_IFACE_TIMING0_TLOW_SHIFT;
        thigh = read_masked(&r->hs_interface_timing0,
                           (TK1I2C_HS_IFACE_TIMING0_THIGH_MASK << TK1I2C_HS_IFACE_TIMING0_THIGH_SHIFT))
                           >> TK1I2C_HS_IFACE_TIMING0_THIGH_SHIFT;
        break;

    case I2C_SLAVE_SPEED_STANDARD:
    case I2C_SLAVE_SPEED_FAST:
    case I2C_SLAVE_SPEED_FASTPLUS:
        tlow = read_masked(&r->interface_timing0,
                           (TK1I2C_IFACE_TIMING0_TLOW_MASK << TK1I2C_IFACE_TIMING0_TLOW_SHIFT))
                           >> TK1I2C_IFACE_TIMING0_TLOW_SHIFT;
        thigh = read_masked(&r->interface_timing0,
                            (TK1I2C_IFACE_TIMING0_THIGH_MASK << TK1I2C_IFACE_TIMING0_THIGH_SHIFT))
                            >> TK1I2C_IFACE_TIMING0_THIGH_SHIFT;
        break;

    default:
        return -1;
    }

    return TK1_CAR_PLLP_INPUT_FREQ_HZ / ((tlow+thigh+vc) * (divisor));
}

static long
tk1_i2c_set_speed(i2c_bus_t* bus, enum i2c_slave_speed speed)
{
    int32_t         divisor;
    tk1_i2c_regs_t *r = tk1_i2c_get_priv(bus);
    uint8_t         variant_constant = 3;

    divisor = tk1_i2c_calc_divisor_value_for(bus, speed, variant_constant);
    if (divisor < 0 || divisor > UINT16_MAX) {
        return -1;
    }

    if (divisor > 3) {
        variant_constant = 2;
        divisor = tk1_i2c_calc_divisor_value_for(bus, speed, variant_constant);
        if (divisor < 0 || divisor > UINT16_MAX) {
            return -1;
        }
    }

    while (tk1_i2c_calc_baud_resulting_from(bus, divisor,
                                            speed, variant_constant)
           > i2c_speed_freqs[speed]) {
        ZF_LOGV(PREFIX"Resulting baud is %d. Recalculating divisor up from %d to %d.",
                r, tk1_i2c_calc_baud_resulting_from(bus, divisor, speed, variant_constant),
                divisor, divisor+1);
        divisor++;
    }
    ZF_LOGV(PREFIX"Calculated I2C divisor at %d. Effective baud is %d. Previous values: std %d, hs %d.",
            r, divisor,
            tk1_i2c_calc_baud_resulting_from(bus, divisor, speed, variant_constant),
            (r->clk_divisor >> 16 & 0xFFFF),
            (r->clk_divisor & 0xFFFF));

    switch (speed) {
    case I2C_SLAVE_SPEED_HIGHSPEED:
        write_masked(&r->clk_divisor,
                     0xFFFF,
                     ((divisor & TK1I2C_CLK_DIVISOR_HS_MODE_MASK)
                     << TK1I2C_CLK_DIVISOR_HS_MODE_SHIFT));
        break;

    case I2C_SLAVE_SPEED_STANDARD:
    case I2C_SLAVE_SPEED_FAST:
    case I2C_SLAVE_SPEED_FASTPLUS:
        write_masked(&r->clk_divisor,
                     0xFFFF0000,
                     ((divisor & TK1I2C_CLK_DIVISOR_STD_FAST_MODE_MASK)
                     << TK1I2C_CLK_DIVISOR_STD_FAST_MODE_SHIFT));
        break;

    default:
        return -1;
    }

    ZF_LOGD("For I2C speed %s, divisor was calculated to be %d.",
            i2c_speed_names[speed], divisor);

    tk1_i2c_config_commit(bus, COMMIT_MASTER_BIT);
    return i2c_speed_freqs[speed];
}

static inline void
tk1_i2c_toggle_irq(i2c_bus_t *ib, bool enable, uint32_t irqs)
{
    tk1_i2c_regs_t *r = tk1_i2c_get_priv(ib);

    if (enable) {
        r->interrupt_mask |= irqs;
    } else {
        r->interrupt_mask &= ~irqs;
    }
}

static inline void
tk1_i2c_acknowledge_irq(i2c_bus_t *ib, uint32_t irqs)
{
    tk1_i2c_regs_t *r = tk1_i2c_get_priv(ib);
    
    r->interrupt_status |= irqs;
}

static inline void
tk1_i2c_callback(i2c_bus_t *ib, enum i2c_stat stat, size_t nbytes)
{
    if (ib->cb != NULL) {
        ib->cb(ib, stat, nbytes, ib->token);
    }
}

static tk1_i2c_pktheaders_t
tk1_i2c_prepare_mmode_xfer_headers(i2c_slave_t *sl, size_t nbytes,
                                   bool is_write,
                                   bool send_repeat_start_not_stop)
{
    tk1_i2c_pktheaders_t headers;
    tk1_i2c_state_t *state;

    assert(sl != NULL);
    assert(sl->bus != NULL);
    assert(nbytes > 0);

    state = tk1_i2c_get_state(sl->bus);

    headers.io0 = headers.io1 = headers.i2c = 0;

    headers.io0 = (0 << TK1I2C_TXPKT0_PKTTYPE_SHIFT)
        | (TK1I2C_TXPKT0_PROTOCOL_I2C << TK1I2C_TXPKT0_PROTOCOL_SHIFT)
        /* Linux uses a 0-based device index for the ID. */
        | (state->controller_id << TK1I2C_TXPKT0_CONTROLLER_ID_SHIFT)
        /* The PktID is optional, AFAICT. Linux uses the value "1" always. */
        | (1 << TK1I2C_TXPKT0_PKTID_SHIFT)
        /* TK1 TRM Section 33.2.2.1: Table 133:
         *  "For I2C, ProtHdrSize = 0 for request packets and 1 for response
         *  packets"
         *
         * Values:
         *  0 = 1 word; 1 = 2 words; 3 = 4 words.
         */
        | (0 << TK1I2C_TXPKT0_PROTHDR_SIZE_SHIFT);

    headers.io1 = ((nbytes - 1) & TK1I2C_TXPKT1_PAYLOAD_SIZE_MASK)
        << TK1I2C_TXPKT1_PAYLOAD_SIZE_SHIFT;

    headers.i2c = TK1I2C_I2CPKT_IRQ_ON_COMPLETION_BIT
        | (send_repeat_start_not_stop ? TK1I2C_I2CPKT_SEND_REPEAT_START_NOT_STOP_BIT : 0)
        | (is_write ? 0 : TK1I2C_I2CPKT_READ_XFER_BIT)
        | TK1I2C_I2CPKT_RESPONSE_PKT_ENABLE_BIT;

    if (sl->address_size == I2C_SLAVE_ADDR_10BIT) {
        headers.i2c |= TK1I2C_I2CPKT_10_BIT_ADDRESS_BIT;
        headers.i2c |= (sl->address & TK1I2C_I2CPKT_SLAVE_ADDR_10BIT_MASK)
                << TK1I2C_I2CPKT_SLAVE_ADDR_10BIT_SHIFT;
    } else {
        /* TK1 TRM Section 33.2.5, Table 135:
         *  "Slave Address. Bit 0 is ignored for 7-bit addressing, but should
         *  always match bit 19 (READ/WRITE)."
         */
        headers.i2c |= (is_write ? 0 : 1);
        headers.i2c |= (sl->address & TK1I2C_I2CPKT_SLAVE_ADDR_7BIT_MASK)
                << TK1I2C_I2CPKT_SLAVE_ADDR_7BIT_SHIFT;
    }

    if (sl->max_speed == I2C_SLAVE_SPEED_HIGHSPEED) {
        /* Enable HS mode bit and set HS master mode address. */
        headers.i2c |= TK1I2C_I2CPKT_HS_MODE_BIT;
        /* Shouldn't need to mask it because we masked it in
         * set_hsmode_master_addr.
         */
        headers.i2c |= state->master.hsmode_master_address
            << TK1I2C_I2CPKT_HS_MASTER_ADDR_SHIFT;
    }

    if (sl->i2c_opts & I2C_SLAVE_OPTS_DEVICE_DOES_NOT_ACK) {
        /* Enable handling of NACK when ACK is expected. */
        headers.i2c |= TK1I2C_I2CPKT_CONTINUE_ON_NACK_BIT;
    }

    if (sl->i2c_opts & TK1I2C_SLAVE_FLAGS_XFER_IN_PROGRESS_BIT) {
        headers.i2c |= TK1I2C_I2CPKT_CONTINUE_XFER_BIT;
    }

    return headers;
}

/** Takes a buffer and attempts to write as much of it as it can to the master
 * or slave mode TX FIFO.
 *
 * Always begins reading the data from index 0 in the buffer.
 *
 * @return number of bytes that were written out to the FIFO from the buffer.
 */
static size_t
tk1_i2c_fill_tx_fifo(i2c_bus_t *ib, enum i2c_xfer_mode mode,
                     const void *const _data, size_t buffsize)
{
    tk1_i2c_regs_t *r = tk1_i2c_get_priv(ib); 
    volatile uint32_t *tx_fifo_reg;
    size_t i, empty_fifo_words, buff_remaining;
    uint32_t *data = (uint32_t *)_data;

    if (mode == I2C_MODE_MASTER) {
        tx_fifo_reg = &r->tx_packet_fifo;
        empty_fifo_words = r->fifo_status >> TK1I2C_FIFO_STATUS_MMODE_TX_EMPTY_SHIFT
              & TK1I2C_FIFO_STATUS_MMODE_TX_EMPTY_MASK;
    } else {
        tx_fifo_reg = &r->tx_packet_fifo;
        empty_fifo_words = r->fifo_status >> TK1I2C_FIFO_STATUS_SMODE_TX_EMPTY_SHIFT
              & TK1I2C_FIFO_STATUS_SMODE_TX_EMPTY_MASK;
    }

    buff_remaining = buffsize;
    for (i = 0; i < empty_fifo_words && buff_remaining >= TK1I2C_NBYTES_PER_FIFO_WORD; i++) {
        *tx_fifo_reg = data[i];
        buff_remaining -= TK1I2C_NBYTES_PER_FIFO_WORD;
    }

    if (i < empty_fifo_words && buff_remaining > 0) {
        uint8_t *last_word_data = (uint8_t *)&data[i];
        uint32_t last_word = 0;

        if (buff_remaining >= TK1I2C_NBYTES_PER_FIFO_WORD) {
            ZF_LOGF("Bug: shifting last word with %d bytes remaining in buffer.",
                    buff_remaining);
        }

        for (int j = 0; j < buff_remaining; j++) {
            last_word |= last_word_data[j] << (8 * j);
        }
        *tx_fifo_reg = last_word;
        buff_remaining -= buff_remaining;
    }

    return buffsize - buff_remaining;
}

/** Takes a buffer and attempts to read as much data from the master or slave
 * mode RX FIFO into that buffer.
 *
 * Always begins writing into the buffer at index 0.
 *
 * @return number of bytes read from the FIFO into the buffer.
 */
static size_t
tk1_i2c_drain_rx_fifo(i2c_bus_t *ib, enum i2c_xfer_mode mode,
                      void *const _data, size_t buffsize)
{
    tk1_i2c_regs_t *r = tk1_i2c_get_priv(ib);
    volatile uint32_t *rx_fifo_reg;
    size_t i, avail_fifo_words, buff_remaining;
    uint32_t *data32 = (uint32_t *)_data;

    if (mode == I2C_MODE_MASTER) {
        rx_fifo_reg = &r->rx_fifo;
        avail_fifo_words = r->fifo_status >> TK1I2C_FIFO_STATUS_MMODE_RX_FULL_SHIFT
              & TK1I2C_FIFO_STATUS_MMODE_RX_FULL_MASK;
    } else {
        rx_fifo_reg = &r->rx_fifo;
        avail_fifo_words = r->fifo_status >> TK1I2C_FIFO_STATUS_SMODE_RX_FULL_SHIFT
              & TK1I2C_FIFO_STATUS_SMODE_RX_FULL_MASK;
    }

    buff_remaining = buffsize;
    for (i = 0; i < avail_fifo_words && buff_remaining >= TK1I2C_NBYTES_PER_FIFO_WORD; i++) {
        data32[i] = *rx_fifo_reg;
        buff_remaining -= TK1I2C_NBYTES_PER_FIFO_WORD;
    }

    if (i < avail_fifo_words && buff_remaining > 0) {
        uint8_t *last_word_data = (uint8_t *)&data32[i];
        uint32_t last_word = *rx_fifo_reg;

        if (buff_remaining >= TK1I2C_NBYTES_PER_FIFO_WORD) {
            ZF_LOGF("Bug: shifting last word out of RX fifo when it's not the "
                    "last word.");
        }

        for (int j = 0; j < buff_remaining; j++) {
            last_word_data[j] = last_word & 0xFF;
            last_word >>= 8;
        }
        buff_remaining -= buff_remaining;
        /* Increment i for the case where there were more words in the FIFO.
         * This case will be picked up below.
         */
        i++;
    }

    /* Handle the case where the user provided a buffer that is smaller than the
     * amount of data that the device will actually send. We flush the FIFOS
     * here to deal with this case.
     *
     * There is always 1 extra word in the RX FIFO at the end, so ignore it.
     */
    if (i < avail_fifo_words - 1 && buff_remaining == 0) {
        ZF_LOGW("Bug: usermode buffer is too small for received data. %d of %d words read from fifo.\n"
                "\tOther word was 0x%x. pkt trans status 0x%x. After reading, fifo status is %d",
                i, avail_fifo_words, *rx_fifo_reg, r->packet_transfer_status,
                (r->fifo_status >> TK1I2C_FIFO_STATUS_MMODE_RX_FULL_SHIFT
              & TK1I2C_FIFO_STATUS_MMODE_RX_FULL_MASK));
        tk1_i2c_flush_fifos(ib, mode);
    }

    return buffsize - buff_remaining;
}

/** Handle a master-mode IRQ.
 *
 * Basically either fill the Tx fifo for an ongoing send operation, or empty the
 * RX fifo for an ongoing receive operation.
 */
#define IRQPREFIX "I2C %p: IRQ: "
static void
tk1_i2c_mmode_handle_irq(i2c_bus_t* ib)
{
    tk1_i2c_regs_t *r = tk1_i2c_get_priv(ib);
    tk1_i2c_state_t *s = tk1_i2c_get_state(ib);
    UNUSED uint32_t int_status;
    int err, do_callback=0, callback_status, callback_nbytes=0;

    int_status = r->interrupt_status;
    if (int_status & TK1I2C_INTSTATUS_MMODE_TX_FIFO_OVF_BIT) {
        ZF_LOGF(IRQPREFIX "TX FIFO has been overflowed.", r);
        do_callback = 1;
        callback_status = I2CSTAT_ERROR;
        callback_nbytes = s->master.xfer_cursor;
    }

    if (int_status & TK1I2C_INTSTATUS_MMODE_RX_FIFO_UNR_BIT) {
        ZF_LOGF(IRQPREFIX "RX FIFO has been underrun.", r);
        do_callback = 1;
        callback_status = I2CSTAT_ERROR;
        callback_nbytes = s->master.xfer_cursor;
    }

    if (int_status & TK1I2C_INTSTATUS_MMODE_NO_ACK_BIT) {
        ZF_LOGE(IRQPREFIX "Slave failed to send ACK. Does this slave "
                "require continue_on_nack?", r);
        tk1_i2c_handle_nack(ib);
        do_callback = 1;
        callback_status = I2CSTAT_NACK;
        callback_nbytes = s->master.xfer_cursor;
    }

    if (int_status & TK1I2C_INTSTATUS_MMODE_ARBITRATION_LOST_BIT) {
        ZF_LOGW(IRQPREFIX "Lost arbitration.", r);
        tk1_i2c_handle_arbitration_loss(ib);
        do_callback = 1;
        callback_status = I2CSTAT_ARBITRATION_LOST;
        callback_nbytes = s->master.xfer_cursor;
    }

    if (int_status & TK1I2C_INTSTATUS_MMODE_TX_FIFO_DATA_REQ_BIT
        && s->master.is_write) {

        /* Refill TX fifo. */
        if (s->master.xfer_cursor <= s->master.nbytes) {
            err = tk1_i2c_fill_tx_fifo(ib, I2C_MODE_MASTER,
                                       &s->master.buff[s->master.xfer_cursor],
                                       s->master.nbytes - s->master.xfer_cursor);

            if (err < 0) {
                ZF_LOGF(IRQPREFIX "TX FIFO trigger IRQ came in, but "
                        "TX FIFO is full.", r);
            }

            s->master.xfer_cursor += err;
            if (s->master.xfer_cursor > s->master.nbytes) {
                ZF_LOGF("Bug: TX userspace buffer has been overshot by driver!");
            }

            if (s->master.xfer_cursor == s->master.nbytes) {
                /* Disable TX FIFO trigger IRQ and then when the XFER_COMPLETE
                 * IRQ comes in, callback.
                 */
                tk1_i2c_toggle_irq(ib, false, TK1I2C_INTMASK_MMODE_TX_FIFO_DATA_REQ_BIT);
            }
        } else {
            ZF_LOGF(IRQPREFIX "TX FIFO data req IRQ has read beyond "
                    "userspace buffer!\n"
                    "\txfer_cursor %d, nbytes %d.",
                    r, s->master.xfer_cursor, s->master.nbytes);
        }
    }

    if (int_status & TK1I2C_INTSTATUS_MMODE_RX_FIFO_DATA_REQ_BIT
        && !s->master.is_write) {

        /* Drain RX fifo. */
        if (s->master.xfer_cursor <= s->master.nbytes) {
            err = tk1_i2c_drain_rx_fifo(ib, I2C_MODE_MASTER,
                                        &s->master.buff[s->master.xfer_cursor],
                                        s->master.nbytes - s->master.xfer_cursor);

            if (err < 0) {
                ZF_LOGF(IRQPREFIX "RX FIFO trigger IRQ came in, but "
                        "RX FIFO is empty.", r);
            }

            s->master.xfer_cursor += err;
            if (s->master.xfer_cursor > s->master.nbytes) {
                ZF_LOGF("Bug: RX userspace buffer has been overshot by driver!");
            }

            if (s->master.xfer_cursor == s->master.nbytes) {
                /* Disable RX FIFO trigger IRQ and then when the XFER_COMPLETE
                 * IRQ comes in, callback.
                 */
                tk1_i2c_toggle_irq(ib, false, TK1I2C_INTMASK_MMODE_RX_FIFO_DATA_REQ_BIT);
            }
        } else {
            ZF_LOGF(IRQPREFIX "RX FIFO data req IRQ has overrun userspace "
                    "buffer.\n"
                    "\txfer_cursor %d, nbytes %d.",
                    r, s->master.xfer_cursor, s->master.nbytes);
        }
    }

    if (int_status & TK1I2C_INTSTATUS_MMODE_PACKET_XFER_COMPLETE_BIT
        || int_status & TK1I2C_INTSTATUS_MMODE_ALL_PACKETS_XFER_COMPLETE_BIT) {
        if (s->master.xfer_cursor < s->master.nbytes) {
            ZF_LOGF(IRQPREFIX"Bug: Got \"PKT_XFER_COMPLETE\" IRQ when there "
                    "were bytes remaining to be xmitted.", r);
        }

        if (int_status & TK1I2C_INTSTATUS_MMODE_PACKET_XFER_COMPLETE_BIT) {
            ZF_LOGD(IRQPREFIX"Xfer complete, pktstatus is 0x%x, i2cstatus is 0x%x.", r,
                    r->packet_transfer_status, r->status);
            /* XFER_COMPLETE IRQ has occured: callback into userspace.
             * Keep in mind that the controller can set both the RX/TX trigger
             * IRQ status bits and the XFER_COMPLETE IRQ status bit,
             * so the XFER_COMPLETE IRQ doesn't actually have to be a separate
             * IRQ.
             */
            do_callback = 1;
            callback_status = I2CSTAT_COMPLETE;
            callback_nbytes = s->master.xfer_cursor;
        }
        if (int_status & TK1I2C_INTSTATUS_MMODE_ALL_PACKETS_XFER_COMPLETE_BIT) {
            ZF_LOGD(IRQPREFIX"Got an \"ALL_PKTS_XFER_COMPLETE\" IRQ.", r);
        }
    }

    /* TK1 TRM Section 33.5.23:
     *  "This register indicates the status bit for which the interrupt is
     *  set. If the interrupt is set, write a 1 to clear it."
     */
    if (do_callback == 1) {
        tk1_i2c_callback(ib, callback_status, callback_nbytes);
    }
    tk1_i2c_acknowledge_irq(ib, int_status);
    return;
}

UNUSED static int
tk1_i2c_mmode_xfer(i2c_slave_t *sl, void *data, size_t nbytes, bool is_write)
{
    return 0;
}

UNUSED static int
tk1_i2c_smode_xfer(void *data, size_t nbytes, bool is_write)
{
    return 0;
}

static int
tk1_i2c_mmode_read(i2c_slave_t* slave, void* buf, size_t size,
                   bool end_with_repeat_start,
                   i2c_callback_fn cb, void* token)
{
    assert(slave != NULL);
    assert(slave->bus != NULL);
    int error;

    tk1_i2c_state_t *s = tk1_i2c_get_state(slave->bus);
    tk1_i2c_regs_t *r = tk1_i2c_get_priv(slave->bus);
    tk1_i2c_pktheaders_t headers;

    if (size == 0) {
        tk1_i2c_callback(slave->bus, I2CSTAT_COMPLETE, 0);
        return 0;
    }

    error = tk1_i2c_set_speed(slave->bus, slave->max_speed);
    if (error < 0) {
        ZF_LOGE(PREFIX"Failed to set speed to %d for slave 0x%x.",
                r, i2c_speed_freqs[slave->max_speed], slave->address);
        return -1;
    }

    tk1_i2c_flush_fifos(slave->bus, I2C_MODE_MASTER);

    if (tk1_i2c_bus_is_locked_up(slave->bus)) {
        if (!tk1_i2c_bus_clear(slave->bus, true)) {
            ZF_LOGE(PREFIX"slave is holding SDA line low, and bus clear "
                    "failed.", r);
            return -1;
        }
    }

    s->master.slave_id = slave->address;
    s->master.is_write = false;
    s->master.buff = buf;
    s->master.nbytes = size;
    s->master.xfer_cursor = 0;

    slave->bus->cb = cb;
    slave->bus->token = token;

    tk1_i2c_set_mode(slave->bus, I2C_MODE_MASTER);
    tk1_i2c_config_commit(slave->bus, COMMIT_MASTER_BIT);

    headers = tk1_i2c_prepare_mmode_xfer_headers(slave, size, false,
                                                 end_with_repeat_start);
    r->tx_packet_fifo = headers.io0;
    r->tx_packet_fifo = headers.io1;
    r->tx_packet_fifo = headers.i2c;

    /* Enable RX FIFO trigger IRQ. */
    tk1_i2c_toggle_irq(slave->bus, true, TK1I2C_INTMASK_SMODE_RX_FIFO_DATA_REQ_BIT);

    return 0;
}

static int
tk1_i2c_mmode_write(i2c_slave_t *slave, const void* buf, size_t size,
                    bool end_with_repeat_start,
                    i2c_callback_fn cb, void* token)
{
    assert(slave != NULL);
    assert(slave->bus != NULL);
    int error;

    size_t ret;
    tk1_i2c_regs_t *r = tk1_i2c_get_priv(slave->bus);
    tk1_i2c_state_t *s = tk1_i2c_get_state(slave->bus);
    tk1_i2c_pktheaders_t headers;

    if (size == 0) {
        tk1_i2c_callback(slave->bus, I2CSTAT_COMPLETE, 0);
        return 0;
    }

    if (size > TK1I2C_DATA_MAX_NBYTES) {
        ZF_LOGE(PREFIX"Write: xfer sizes > %d not supported.",
                r, TK1I2C_DATA_MAX_NBYTES);
        return -1;
    }

    error = tk1_i2c_set_speed(slave->bus, slave->max_speed);
    if (error < 0) {
        ZF_LOGE(PREFIX"Failed to set speed to %d for slave 0x%x.",
                r, i2c_speed_freqs[slave->max_speed], slave->address);
        return -1;
    }

    tk1_i2c_flush_fifos(slave->bus, I2C_MODE_MASTER);

    if (tk1_i2c_bus_is_locked_up(slave->bus)) {
        if (!tk1_i2c_bus_clear(slave->bus, true)) {
            ZF_LOGE(PREFIX"slave is holding SDA line low, and bus clear "
                    "failed.", r);
            return -1;
        }
    }

    s->master.slave_id = slave->address;
    s->master.is_write = true;
    s->master.buff = (uint8_t *)buf;
    s->master.nbytes = size;
    s->master.xfer_cursor = 0;

    slave->bus->cb = cb;
    slave->bus->token = token;

    tk1_i2c_set_mode(slave->bus, I2C_MODE_MASTER);
    tk1_i2c_config_commit(slave->bus, COMMIT_MASTER_BIT);

    headers = tk1_i2c_prepare_mmode_xfer_headers(slave, size, true,
                                                 end_with_repeat_start);
    r->tx_packet_fifo = headers.io0;
    r->tx_packet_fifo = headers.io1;
    r->tx_packet_fifo = headers.i2c;
    ret = tk1_i2c_fill_tx_fifo(slave->bus, I2C_MODE_MASTER, buf, size);
    if (ret < 1) {
        /* Keep in mind that not being able to write bytes out to the FIFO
         * doesn't mean an error occured.
         */
        ZF_LOGE(PREFIX"Failed to write out bytes to line.", r);
        return -1;
    } else {
        s->master.xfer_cursor = ret;
    }

    /* Enable TX FIFO trigger IRQ */
    tk1_i2c_toggle_irq(slave->bus, true, TK1I2C_INTMASK_MMODE_TX_FIFO_DATA_REQ_BIT);

    return s->master.xfer_cursor;
}

static void
tk1_i2c_set_hsmode_master_address(i2c_bus_t* ib, int addr)
{
    tk1_i2c_state_t *s = tk1_i2c_get_state(ib);

    s->master.hsmode_master_address = addr & TK1I2C_I2CPKT_HS_MASTER_ADDR_MASK;
}

static void
tk1_i2c_register_slave_event_handler(i2c_bus_t* ib,
                                     i2c_aas_callback_fn aas_cb, void *token)
{
    ib->aas_cb = aas_cb;
    ib->aas_token = token;
}

static int
tk1_i2c_slave_init(i2c_bus_t* ib, int address,
               enum i2c_slave_address_size address_size,
               enum i2c_slave_speed max_speed,
               uint32_t flags, i2c_slave_t* sl)
{
    assert(sl != NULL);

    if (address_size == I2C_SLAVE_ADDR_7BIT) {
        address = i2c_extract_address(address);
    }

    /* Internally in this driver, we discard the RW bit */
    sl->address = address;
    sl->address_size = address_size;
    sl->max_speed = max_speed;
    sl->i2c_opts = flags;
    sl->bus = ib;

    sl->slave_read      = &tk1_i2c_mmode_read;
    sl->slave_write     = &tk1_i2c_mmode_write;

    return 0;
}

static void
tk1_i2c_set_mmode_trigger_levels(i2c_bus_t *ib,
                                  uint8_t tx_trig, uint8_t rx_trig)
{
    tk1_i2c_regs_t *r = tk1_i2c_get_priv(ib);

    r->fifo_control &= ~((TK1I2C_FIFO_CONTROL_MMODE_TX_TRIG_MASK << TK1I2C_FIFO_CONTROL_MMODE_TX_TRIG_SHIFT)
        | (TK1I2C_FIFO_CONTROL_MMODE_RX_TRIG_MASK << TK1I2C_FIFO_CONTROL_MMODE_RX_TRIG_SHIFT));

    r->fifo_control |= ((tx_trig & TK1I2C_FIFO_CONTROL_MMODE_TX_TRIG_MASK)
                        << TK1I2C_FIFO_CONTROL_MMODE_TX_TRIG_SHIFT)
                    | ((rx_trig & TK1I2C_FIFO_CONTROL_MMODE_RX_TRIG_MASK)
                        << TK1I2C_FIFO_CONTROL_MMODE_RX_TRIG_SHIFT);
}

UNUSED static void
tk1_i2c_set_smode_trigger_levels(i2c_bus_t *ib,
                                  uint8_t tx_trig, uint8_t rx_trig)
{
    tk1_i2c_regs_t *r = tk1_i2c_get_priv(ib);

    r->fifo_control &= ~((TK1I2C_FIFO_CONTROL_SMODE_TX_TRIG_MASK << TK1I2C_FIFO_CONTROL_SMODE_TX_TRIG_SHIFT)
        | (TK1I2C_FIFO_CONTROL_SMODE_RX_TRIG_MASK << TK1I2C_FIFO_CONTROL_SMODE_RX_TRIG_SHIFT));

    r->fifo_control |= ((tx_trig & TK1I2C_FIFO_CONTROL_SMODE_TX_TRIG_MASK)
                        << TK1I2C_FIFO_CONTROL_SMODE_TX_TRIG_SHIFT)
                    | ((rx_trig & TK1I2C_FIFO_CONTROL_SMODE_RX_TRIG_MASK)
                        << TK1I2C_FIFO_CONTROL_SMODE_RX_TRIG_SHIFT);
}

static int
tk1_i2c_master_stop(i2c_bus_t *ib)
{
    return tk1_i2c_bus_clear(ib, true);
}

static int
tk1_i2c_initialize_controller(i2c_bus_t *ib)
{
    tk1_i2c_regs_t *r = tk1_i2c_get_priv(ib);

    tk1_i2c_set_packet_mode(ib, true);

    if (tk1_i2c_bus_is_locked_up(ib)) {
        if (!tk1_i2c_bus_clear(ib, true)) {
            ZF_LOGE(PREFIX"slave is holding SDA line low, and bus clear "
                    "failed.", r);
            return -1;
        }
    }

    tk1_i2c_set_mmode_trigger_levels(ib, 7, 0);
    tk1_i2c_flush_fifos(ib, I2C_MODE_MASTER);
    tk1_i2c_flush_fifos(ib, I2C_MODE_SLAVE);

    /* ACK all IRQs that the chip currently has active, in case one was 
     * asserted. Perhaps the driver is being restarted, for example.
     */
    tk1_i2c_acknowledge_irq(ib, r->interrupt_status);

    /* Disable those IRQs we either will never want, or will conditionally need
     * to enable later.
     *
     * Master mode IRQs.
     */
    tk1_i2c_toggle_irq(ib, false,
                       TK1I2C_INTMASK_MMODE_TX_FIFO_DATA_REQ_BIT
                       | TK1I2C_INTMASK_MMODE_RX_FIFO_DATA_REQ_BIT
                       | TK1I2C_INTMASK_MMODE_BUS_CLEAR_DONE_BIT
                       | TK1I2C_INTMASK_MMODE_ALL_PACKETS_XFER_COMPLETE_BIT);

    tk1_i2c_toggle_irq(ib, true,
                       TK1I2C_INTMASK_MMODE_ARBITRATION_LOST_BIT
                       | TK1I2C_INTMASK_MMODE_NO_ACK_BIT
                       | TK1I2C_INTMASK_MMODE_TX_FIFO_OVF_BIT
                       | TK1I2C_INTMASK_MMODE_RX_FIFO_UNR_BIT
                       | TK1I2C_INTMASK_MMODE_PACKET_XFER_COMPLETE_BIT);

    /* Slave mode IRQs. */
    tk1_i2c_toggle_irq(ib, false,
                       TK1I2C_INTMASK_SMODE_TX_FIFO_DATA_REQ_BIT
                       | TK1I2C_INTMASK_SMODE_RX_FIFO_DATA_REQ_BIT
                       | TK1I2C_INTMASK_SMODE_TX_BUFFER_REQ_BIT
                       | TK1I2C_INTMASK_SMODE_RX_BUFFER_FULL_BIT);

    tk1_i2c_toggle_irq(ib, true,
                       TK1I2C_INTMASK_SMODE_TX_FIFO_OVF_BIT
                       | TK1I2C_INTMASK_SMODE_RX_FIFO_UNR_BIT
                       | TK1I2C_INTMASK_SMODE_PACKET_XFER_COMPLETE_BIT
                       | TK1I2C_INTMASK_SMODE_PACKET_XFER_ERROR_BIT
                       | TK1I2C_INTMASK_SMODE_SWITCHED_READ2WRITE_BIT
                       | TK1I2C_INTMASK_SMODE_SWITCHED_WRITE2READ_BIT
                       | TK1I2C_INTMASK_SMODE_ACK_WITHHELD_BIT);

    tk1_i2c_config_commit(ib, COMMIT_MASTER_BIT | COMMIT_SLAVE_BIT);
    return 0;
}

int
tegra_i2c_init(int controller_id, void *vaddr, ps_io_ops_t *io_ops,
               i2c_bus_t *ib)
{
    tk1_i2c_state_t *s;
    int error;

    error = ps_malloc(&io_ops->malloc_ops, sizeof(*s), (void **)&s);
    if (error != 0 || s == NULL) {
        ZF_LOGE(PREFIX"Failed to malloc internal state.", vaddr);
        return -1;
    };

    s->regs = vaddr;
    s->controller_id = controller_id;
    ib->priv = s;

    ib->cb = NULL;
    ib->token = NULL;
    ib->aas_cb = NULL;
    ib->aas_token = NULL;

    ib->slave_init                  = &tk1_i2c_slave_init;
    ib->handle_irq                  = &tk1_i2c_mmode_handle_irq;
    ib->set_speed                   = &tk1_i2c_set_speed;
    ib->set_self_slave_address      = NULL;
    ib->register_slave_event_handler= &tk1_i2c_register_slave_event_handler;
    ib->set_hsmode_master_address   = &tk1_i2c_set_hsmode_master_address;
    /* Bus clear */
    ib->master_stop                 = &tk1_i2c_master_stop;

    error = tk1_i2c_initialize_controller(ib);
    if (error != 0) {
        ZF_LOGE(PREFIX"Failed to initialize I2C controller.", vaddr);
        ib->priv = NULL;
        ps_free(&io_ops->malloc_ops, sizeof(*s), s);
        return -1;
    }
    return 0;
}

int
i2c_init(enum i2c_id id, ps_io_ops_t* io_ops, i2c_bus_t* i2c_bus)
{
    void *vaddr;

    assert(io_ops != NULL);
    assert(i2c_bus != NULL);

    switch (id) {
    case TK1_I2C0:
        vaddr = RESOURCE(io_ops, TK1_I2C0);
        break;

    case TK1_I2C1:
        vaddr = RESOURCE(io_ops, TK1_I2C1);
        break;

    case TK1_I2C2:
        vaddr = RESOURCE(io_ops, TK1_I2C2);
        break;

    case TK1_I2C3:
        vaddr = RESOURCE(io_ops, TK1_I2C3);
        break;

    case TK1_I2C4:
        vaddr = RESOURCE(io_ops, TK1_I2C4);
        break;

    case TK1_I2C5:
        vaddr = RESOURCE(io_ops, TK1_I2C5);
        break;

    default:
        ZF_LOGE("Unknown I2C controller %d.", id);
        return -1;
    }

    if (vaddr == NULL) {
        ZF_LOGE("Failed to map in TK1 I2C controller %d.", id);
        return -1;
    }

    return tegra_i2c_init(id, vaddr, io_ops, i2c_bus);
}
