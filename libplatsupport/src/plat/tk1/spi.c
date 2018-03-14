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

#include <stdio.h>
#include <stdbool.h>
#include <platsupport/spi.h>
#include <utils/zf_log.h>

#define SPI_PAGE_PADDR 0x7000D000

#define SPI0_PADDR 0x7000D400
#define SPI1_PADDR 0x7000D600
#define SPI2_PADDR 0x7000D800
#define SPI3_PADDR 0x7000DA00
#define SPI4_PADDR 0x7000DC00
#define SPI5_PADDR 0x7000DE00

#define SPI0_OFFSET (SPI0_PADDR - SPI_PAGE_PADDR)

/* COMMAND1 */
#define SPI_CMD1_GO                     (BIT(31))
#define SPI_CMD1_M_S                    (BIT(30))
#define SPI_CMD1_MODE_MASK              0x3
#define SPI_CMD1_MODE_SHIFT             28
#define SPI_CMD1_CS_SEL_MASK            0x3
#define SPI_CMD1_CS_SEL_SHIFT           26
#define SPI_CMD1_CS_POL_INACTIVE3       (BIT(25))
#define SPI_CMD1_CS_POL_INACTIVE2       (BIT(24))
#define SPI_CMD1_CS_POL_INACTIVE1       (BIT(23))
#define SPI_CMD1_CS_POL_INACTIVE0       (BIT(22))
#define SPI_CMD1_CS_SW_HW               (BIT(21))
#define SPI_CMD1_CS_SW_VAL              (BIT(20))
#define SPI_CMD1_IDLE_SDA_MASK          0x3
#define SPI_CMD1_IDLE_SDA_SHIFT         18
#define SPI_CMD1_BIDIR                  (BIT(17))
#define SPI_CMD1_LSBI_FE                (BIT(16))
#define SPI_CMD1_LSBY_FE                (BIT(15))
#define SPI_CMD1_BOTH_EN_BIT            (BIT(14))
#define SPI_CMD1_BOTH_EN_BYTE           (BIT(13))
#define SPI_CMD1_RX_EN                  (BIT(12))
#define SPI_CMD1_TX_EN                  (BIT(11))
#define SPI_CMD1_PACKED                 (BIT(5))
#define SPI_CMD1_BIT_LEN_MASK           0x1F
#define SPI_CMD1_BIT_LEN_SHIFT          0

/* COMMAND2 */
#define SPI_CMD2_TX_CLK_TAP_DELAY       (BIT(6))
#define SPI_CMD2_TX_CLK_TAP_DELAY_MASK  (0x3F << 6)
#define SPI_CMD2_RX_CLK_TAP_DELAY       (BIT(0))
#define SPI_CMD2_RX_CLK_TAP_DELAY_MASK  (0x3F << 0)

/* TRANSFER STATUS */
#define SPI_XFER_STS_RDY                (BIT(30))

/* FIFO STATUS */
#define SPI_FIFO_STS_CS_INACTIVE        (BIT(31))
#define SPI_FIFO_STS_FRAME_END          (BIT(30))
#define SPI_FIFO_STS_RX_FIFO_FLUSH      (BIT(15))
#define SPI_FIFO_STS_TX_FIFO_FLUSH      (BIT(14))
#define SPI_FIFO_STS_ERR                (BIT(8))
#define SPI_FIFO_STS_TX_FIFO_OVF        (BIT(7))
#define SPI_FIFO_STS_TX_FIFO_UNR        (BIT(6))
#define SPI_FIFO_STS_RX_FIFO_OVF        (BIT(5))
#define SPI_FIFO_STS_RX_FIFO_UNR        (BIT(4))
#define SPI_FIFO_STS_TX_FIFO_FULL       (BIT(3))
#define SPI_FIFO_STS_TX_FIFO_EMPTY      (BIT(2))
#define SPI_FIFO_STS_RX_FIFO_FULL       (BIT(1))
#define SPI_FIFO_STS_RX_FIFO_EMPTY      (BIT(0))

/* SPI CS Timing */
#define CS_SETUP_TIME_0_SHIFT           4
#define CS_HOLD_TIME_0_SHIFT            0

/* DMA CTL */
#define SPI_IE_RX                       (BIT(29))
#define SPI_IE_TX                       (BIT(28))

#define CLK_TAP_DELAY                   0x1f
#define CS_SETUP_TIME                   0xf
#define CS_HOLD_TIME                    0xf

#define FIFO_SIZE                       64

struct spi_regs {
    volatile uint32_t command1;       /* 000:SPI_COMMAND1 register */
    volatile uint32_t command2;       /* 004:SPI_COMMAND2 register */
    volatile uint32_t timing1;        /* 008:SPI_CS_TIM1 register */
    volatile uint32_t timing2;        /* 00c:SPI_CS_TIM2 register */
    volatile uint32_t xfer_status;    /* 010:SPI_TRANS_STATUS register */
    volatile uint32_t fifo_status;    /* 014:SPI_FIFO_STATUS register */
    volatile uint32_t tx_data;        /* 018:SPI_TX_DATA register */
    volatile uint32_t rx_data;        /* 01c:SPI_RX_DATA register */
    volatile uint32_t dma_ctl;        /* 020:SPI_DMA_CTL register */
    volatile uint32_t dma_blk;        /* 024:SPI_DMA_BLK register */
    PAD_STRUCT_BETWEEN(0x24, 0x108, uint32_t);
    volatile uint32_t tx_fifo;        /* 108:SPI_FIFO1 register */
    PAD_STRUCT_BETWEEN(0x108, 0x188, uint32_t);
    volatile uint32_t rx_fifo;        /* 188:SPI_FIFO2 register */
    volatile uint32_t spare_ctl;      /* 18c:SPI_SPARE_CTRL register */
};

struct spi_bus {
    volatile struct spi_regs* regs;
    uint32_t clock_mode;           // see table 147 on page 2436 of the TRM
    bool in_progress;
    uint8_t *txbuf;
    uint8_t *rxbuf;
    size_t txsize, rxsize;
    spi_chipselect_fn cs;
    spi_callback_fn cb;
    void* token;
    spi_slave_config_t *curr_slave;
};

uint32_t spi_controller_offsets[] = {
    [SPI0] = SPI0_OFFSET
};

static spi_bus_t _spi[] = {
    [SPI0] = { .regs = NULL, .clock_mode = 0 },
};

int
tegra_spi_init(enum spi_id id, volatile void* base, spi_chipselect_fn cs_func,
               mux_sys_t* mux_sys, clock_sys_t* clock_sys,
               spi_bus_t** ret_spi_bus)
{
    spi_bus_t* spi_bus = &_spi[id];
    spi_bus->cs = cs_func;
    spi_bus->regs = base + spi_controller_offsets[id];

    // Clear relevant bits in fifo status register
    uint32_t fifo_status = SPI_FIFO_STS_RX_FIFO_FLUSH |
                  SPI_FIFO_STS_TX_FIFO_FLUSH |
                  SPI_FIFO_STS_ERR           |
                  SPI_FIFO_STS_TX_FIFO_OVF   |
                  SPI_FIFO_STS_TX_FIFO_UNR   |
                  SPI_FIFO_STS_RX_FIFO_OVF   |
                  SPI_FIFO_STS_RX_FIFO_UNR;
    spi_bus->regs->fifo_status = fifo_status;

    uint32_t command1 = spi_bus->regs->command1;
    command1 |= (spi_bus->clock_mode << SPI_CMD1_MODE_SHIFT);
    if (spi_bus->cs != NULL) {
        // Use software chip select if spi_chipselect_fn is provided
        command1 |= SPI_CMD1_CS_SW_HW;
    }
    command1 |= SPI_CMD1_TX_EN | SPI_CMD1_RX_EN;
    command1 |= (8 - 1) << SPI_CMD1_BIT_LEN_SHIFT;
    spi_bus->regs->command1 = command1;

    uint32_t command2 = spi_bus->regs->command2;
    command2 |= (CLK_TAP_DELAY & SPI_CMD2_RX_CLK_TAP_DELAY_MASK);
    spi_bus->regs->command2 = command2;

    uint32_t timing1 = spi_bus->regs->timing1;
    timing1 |= (CS_SETUP_TIME << CS_SETUP_TIME_0_SHIFT);
    timing1 |= (CS_HOLD_TIME << CS_HOLD_TIME_0_SHIFT);
    spi_bus->regs->timing1 = timing1;

    uint32_t dma_ctl = spi_bus->regs->dma_ctl;
    dma_ctl |= SPI_IE_RX | SPI_IE_TX;
    spi_bus->regs->dma_ctl = dma_ctl;

    spi_bus->regs->xfer_status |= SPI_XFER_STS_RDY;

    *ret_spi_bus = spi_bus;
    return 0;
}

static void
finish_spi_transfer(spi_bus_t* spi_bus) {
    // Drain RX FIFO
    size_t size = spi_bus->txsize + spi_bus->rxsize;
    for (int i = 0; i < size; i++) {
        uint32_t data_in = spi_bus->regs->rx_fifo;
        if (spi_bus->rxbuf != NULL) {
            spi_bus->rxbuf[i] = (uint8_t) (data_in & 0xFF);
        }
    }

    spi_bus->regs->xfer_status |= SPI_XFER_STS_RDY;
    spi_bus->in_progress = false;

    // Release chip select
    if (spi_bus->cs != NULL) {
        spi_bus->cs(spi_bus->curr_slave, SPI_CS_RELAX);
    }

    spi_bus->cb(spi_bus, size, spi_bus->token);
}

void
spi_handle_irq(spi_bus_t* spi_bus)
{
    if (spi_bus->regs->xfer_status & SPI_XFER_STS_RDY) {
        finish_spi_transfer(spi_bus);
    } else {
        uint32_t fifo_status = spi_bus->regs->fifo_status;
        ZF_LOGE("FIFO error, status = 0x%08x", fifo_status);

        // Abort transfer
        spi_bus->regs->command1 &= ~SPI_CMD1_GO;
        // Clear FIFO status, flush FIFOs
        fifo_status |= SPI_FIFO_STS_RX_FIFO_FLUSH | SPI_FIFO_STS_TX_FIFO_FLUSH;
        spi_bus->regs->fifo_status = fifo_status;
        // Re-initialize transfer status
        spi_bus->regs->xfer_status |= SPI_XFER_STS_RDY;
        spi_bus->in_progress = false;
        // Release chip select
        if (spi_bus->cs != NULL) {
            spi_bus->cs(spi_bus->curr_slave, SPI_CS_RELAX);
        }
        // Indicate failure to user
        spi_bus->cb(spi_bus, -1, spi_bus->token);
    }
}

static void
start_spi_transfer(spi_bus_t* spi_bus) {
    // Assert chip select
    if (spi_bus->cs != NULL) {
        spi_bus->cs(spi_bus->curr_slave, SPI_CS_ASSERT);
    }
    size_t size = spi_bus->txsize + spi_bus->rxsize;
    spi_bus->regs->dma_blk = size - 1;

    // Load TX FIFO
    for (int i = 0; i < size; i++) {
        if (i < spi_bus->txsize) {
            spi_bus->regs->tx_fifo = spi_bus->txbuf[i];
        } else {
            spi_bus->regs->tx_fifo = 0;
        }
    }

    // Signal transfer to begin
    spi_bus->regs->command1 |= SPI_CMD1_GO;
}

int
spi_xfer(spi_bus_t* spi_bus, const void* txdata, size_t txcnt,
         void* rxdata, size_t rxcnt, spi_callback_fn cb, void* token)
{
    if (spi_bus->in_progress) {
        ZF_LOGE("SPI transaction in progress");
        return -1;
    }
    if (txcnt + rxcnt > FIFO_SIZE) {
        ZF_LOGE("SPI transaction size (%d) exceeds FIFO size (%d)", txcnt + rxcnt, FIFO_SIZE);
        return -2;
    }
    if (cb == NULL) {
        ZF_LOGE("Synchronous SPI transactions are not implemented");
        return -3;
    }

    spi_bus->txbuf = (uint8_t*) txdata;
    spi_bus->rxbuf = (uint8_t*) rxdata;
    spi_bus->txsize = txcnt;
    spi_bus->rxsize = rxcnt;
    spi_bus->in_progress = true;
    spi_bus->cb = cb;
    spi_bus->token = token;

    start_spi_transfer(spi_bus);
    return 0;
}

void
spi_prepare_transfer(spi_bus_t* spi_bus, const spi_slave_config_t* cfg)
{
    // TODO: implement support for multiple slaves
    spi_bus->curr_slave = (spi_slave_config_t *)cfg;
}
