/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/* SPI driver */

#include <stdio.h>
#include <stdint.h>
#include <platsupport/spi.h>
#include "../../services.h"

//#define DEBUG_SPI
#ifdef DEBUG_SPI
#define DSPI(args...) \
    do { \
        printf("SPI %s(%d):", __func__, __LINE__); \
        printf(args); \
        printf("\n"); \
    } while(0)
#else
#define DSPI(...) do{}while(0)
#endif


/* SPI configuration */
#define CH_CFG_HIGH_SPEED_EN       BIT(6)
#define CH_CFG_SW_RST              BIT(5)
#define CH_CFG_SLAVE               BIT(4)
#define CH_CFG_CPOL                BIT(3)
#define CH_CFG_CPHA                BIT(2)
#define CH_CFG_RX_CH_ON            BIT(1)
#define CH_CFG_TX_CH_ON            BIT(0)

/* FIFO control */
#define FIFO_SIZE                   64
#define MODE_CFG_CH_WIDTH_SHF      (29)
#define MODE_CFG_TRAILING_CNT_SHF  (19)
#define MODE_CFG_BUS_WIDTH_SHF     (17)
#define MODE_CFG_RX_RDY_LVL_SHF    (11)
#define MODE_CFG_TX_RDY_LVL_SHF    (5)
#define MODE_CFG_RX_DMA_SW         BIT(2)
#define MODE_CFG_TX_DMA_SW         BIT(1)
#define MODE_CFG_DMA_TYPE          BIT(0)

/* Slave selection control */
#define CS_REG_NCS_TIME_COUNT_SHF  (4)
#define CS_REG_AUTO_N_MANUAL       BIT(1)
#define CS_REG_NSSOUT              BIT(0)

/* Interrupt enable */
#define INT_EN_TRAILING            BIT(6)
#define INT_EN_RX_OVERRUN          BIT(5)
#define INT_EN_RX_UNDERRUN         BIT(4)
#define INT_EN_TX_OVERRUN          BIT(3)
#define INT_EN_TX_UNDERRUN         BIT(2)
#define INT_EN_RX_FIFO_RDY         BIT(1)
#define INT_EN_TX_FIFO_RDY         BIT(0)

/* SPI status */
#define STATUS_TX_DONE             BIT(25)
#define STATUS_TRAILING_BYTE       BIT(24)
#define STATUS_RX_FIFO_LVL_SHF     (15)
#define STATUS_TX_FIFO_LVL_SHF     (6)
#define STATUS_RX_OVERRUN          BIT(5)
#define STATUS_RX_UNDERRUN         BIT(4)
#define STATUS_TX_OVERRUN          BIT(3)
#define STATUS_TX_UNDERRUN         BIT(2)
#define STATUS_RX_FIFO_RDY         BIT(1)
#define STATUS_TX_FIFO_RDY         BIT(0)

/* Packet count */
#define PACKET_CNT_EN              BIT(16)
#define PACKET_CNT_VALUE_SHF       BIT(0)

/* Interrupt pending clear */
#define PENDING_CLR_TX_UNDERRUN    BIT(4)
#define PENDING_CLR_TX_OVERRUN     BIT(3)
#define PENDING_CLR_RX_UNDERRUN    BIT(2)
#define PENDING_CLR_RX_OVERRUN     BIT(1)
#define PENDING_CLR_TRAILING       BIT(0)

/* Swap configuration */
#define SWAP_CFG_RX_HWORD          BIT(7)
#define SWAP_CFG_RX_BYTE           BIT(6)
#define SWAP_CFG_RX_BIT            BIT(5)
#define SWAP_CFG_RX_EN             BIT(4)
#define SWAP_CFG_TX_HWORD          BIT(3)
#define SWAP_CFG_TX_BYTE           BIT(2)
#define SWAP_CFG_TX_BIT            BIT(1)
#define SWAP_CFG_TX_EN             BIT(0)

/* Feedback clock selection */
#define FB_CLK_SEL_SHF             (0)

struct spi_regs {
    uint32_t ch_cfg;
    uint32_t res;
    uint32_t mode_cfg;
    uint32_t cs_reg;
    uint32_t int_en;
    uint32_t status;
    uint32_t tx_data;
    uint32_t rx_data;
    uint32_t packet_cnt;
    uint32_t pending_clr;
    uint32_t swap_cfg;
    uint32_t fb_clk_sel;
};

enum spi_mode {
    MASTER_MODE = 0,
    SLAVE_MODE
};

enum spi_cs_state {
    SPI_CS_ASSERT,
    SPI_CS_RELAX
};

struct spi_bus {
    volatile struct spi_regs* regs;
    enum mux_feature mux;
    int mode: 1;              //0 -- Master, 1 -- Slave
    int high_speed: 1;        //High speed operation in slave mode.
    int cs_auto: 1;           //Auto chip selection.

    /* Transfer management */
    const char *txbuf;
    char *rxbuf;
    size_t txcnt, rxcnt;
    size_t txsize, rxsize;
    spi_callback_fn cb;
    void* token;
};

static spi_bus_t _spi[NSPI] = {
    { .regs = NULL, .mux = MUX_SPI0  },
    { .regs = NULL, .mux = MUX_SPI1  },
    { .regs = NULL, .mux = MUX_SPI2  },
#if defined(PLAT_EXYNOS4)
#elif defined(PLAT_EXYNOS5)
    { .regs = NULL, .mux = MUX_SPI0_ISP },
    { .regs = NULL, .mux = MUX_SPI1_ISP },
#else  /* EXYNOS? */
#error Unknown Exynos regsd platform
#endif /* EXYNOSX */
};


static void
spi_reset(spi_bus_t *spi_bus)
{
    uint32_t v;

    /* Turn off the channel */
    v = spi_bus->regs->ch_cfg;
    v &= ~(CH_CFG_RX_CH_ON | CH_CFG_TX_CH_ON);
    spi_bus->regs->ch_cfg = v;

    /* Write to reset bit */
    v = spi_bus->regs->ch_cfg;
    v |= CH_CFG_SW_RST;
    spi_bus->regs->ch_cfg = v;

    /* Clear reset bit */
    v = spi_bus->regs->ch_cfg;
    v &= ~CH_CFG_SW_RST;
    spi_bus->regs->ch_cfg = v;

    /* Turn on the channel */
    v = spi_bus->regs->ch_cfg;
    v |= (CH_CFG_RX_CH_ON | CH_CFG_TX_CH_ON);
    spi_bus->regs->ch_cfg = v;
}


static void
spi_config(spi_bus_t *spi_bus)
{
    uint32_t v;

    /* Step1: SPI configuration */
    v = 0;
    /* Master/Slave mode */
    if (spi_bus->mode == SLAVE_MODE) {
        v |= CH_CFG_SLAVE;

        /* High speed mode is slave mode only */
        if (spi_bus->high_speed) {
            v |= CH_CFG_HIGH_SPEED_EN;
            v &= ~CH_CFG_CPHA;
        }
    }
    spi_bus->regs->ch_cfg = v;

    /*
     * Step2: Feedback clock
     * The default clock is 33MHz, so we use a 180 degree phase feedback.
     * The feedback clock only works in the master mode.
     */
    if (spi_bus->mode == MASTER_MODE) {
        v = (0x0 << FB_CLK_SEL_SHF);
        spi_bus->regs->fb_clk_sel = v;
    }

    /*
     * Step3: FIFO control
     * Channel width to Byte, No DMA, only need to set trigger level.
     */
    v = (0x20 << MODE_CFG_RX_RDY_LVL_SHF) | (0x20 << MODE_CFG_TX_RDY_LVL_SHF);
    spi_bus->regs->mode_cfg = v;

    /* Step4: Interrupts */
    spi_bus->regs->int_en = 0x0;
    spi_bus->regs->pending_clr = 0xff;

    /* Step5: Packet control */
    spi_bus->regs->packet_cnt = 0;

    /* Step6: Turn on the channel */
    v = spi_bus->regs->ch_cfg;
    v |= (CH_CFG_RX_CH_ON | CH_CFG_TX_CH_ON);
    spi_bus->regs->ch_cfg = v;

    /* Step7: Chip selection */
    v = (0x0 << CS_REG_NCS_TIME_COUNT_SHF);
    if (spi_bus->cs_auto) {
        v |= CS_REG_AUTO_N_MANUAL;
    } else {
        v |= CS_REG_NSSOUT;
    }
    spi_bus->regs->cs_reg = v;
}

static void
spi_cs(spi_bus_t* spi_bus, enum spi_cs_state state)
{
    if (!spi_bus->cs_auto) {
        uint32_t v;
        v = spi_bus->regs->cs_reg;
        if (state == SPI_CS_ASSERT) {
            v &= ~CS_REG_NSSOUT;
        } else {
            v |= CS_REG_NSSOUT;
        }
        spi_bus->regs->cs_reg = v;
    }
}

static int
spi_init_common(spi_bus_t* spi_bus, mux_sys_t* mux_sys, clock_sys_t* clock_sys)
{
    if (mux_sys && mux_sys_valid(mux_sys)) {
        mux_feature_enable(mux_sys, spi_bus->mux);
    } else {
        LOG_INFO("SPI: Skipping MUX initialisation as no mux subsystem was provided\n");
    }

    if (clock_sys && clock_sys_valid(clock_sys)) {
        LOG_INFO("SPI: Assuming default clock frequent (Implement me)\n");
    } else {
        LOG_INFO("SPI: Assuming default clock frequency as no clock subsystem was provided\n");
    }

    spi_bus->mode = 0;
    spi_bus->high_speed = 0;
    spi_bus->cs_auto = 0;

    spi_reset(spi_bus);
    spi_config(spi_bus);
    return 0;
}

int
transfer_data(spi_bus_t* spi_bus)
{
    int rxfifo_cnt, txfifo_cnt, txfifo_space;
    uint32_t stat;
    stat = spi_bus->regs->status;
    rxfifo_cnt = (stat >> STATUS_RX_FIFO_LVL_SHF) & 0x1FF;
    txfifo_cnt = (stat >> STATUS_TX_FIFO_LVL_SHF) & 0x1FF;
    txfifo_space = FIFO_SIZE - txfifo_cnt - 1;

    /* Check for fatal events */
    if (stat & STATUS_RX_OVERRUN) {
        printf("SPI RX overrun\n");
    }
    if (stat & STATUS_RX_UNDERRUN) {
        printf("SPI RX underrun\n");
    }
    if (stat & STATUS_TX_OVERRUN) {
        printf("SPI TX overrun\n");
    }
    if (stat & STATUS_TX_UNDERRUN) {
        printf("SPI TX underrun\n");
    }

    /* Drain the RX FIFO */
    while (rxfifo_cnt--) {
        uint32_t d;
        d = spi_bus->regs->rx_data;
        /* Store the data only if we are in RX phase */
        if (spi_bus->rxcnt >= spi_bus->txsize) {
            assert(spi_bus->rxcnt - spi_bus->txsize < spi_bus->rxsize);
        }
        *spi_bus->rxbuf++ = d;
        spi_bus->rxcnt++;
    }

    /* Fill the TX FIFO */
    if (txfifo_space > spi_bus->txsize + spi_bus->rxsize - spi_bus->rxcnt) {
        txfifo_space = spi_bus->txsize + spi_bus->rxsize - spi_bus->txcnt;
    }
    while (txfifo_space--) {
        uint32_t d;
        if (spi_bus->txcnt < spi_bus->txsize) {
            d = *spi_bus->txbuf++;
        } else {
            d = 0xff;
        }
        spi_bus->regs->tx_data = d;
        spi_bus->txcnt++;
    }

    /* Check for completion */
    if ((stat & STATUS_TX_DONE) && spi_bus->rxcnt == spi_bus->rxsize + spi_bus->txsize) {
        spi_bus->regs->int_en = 0;
        spi_cs(spi_bus, SPI_CS_RELAX);
        if (spi_bus->cb) {
            spi_bus->cb(spi_bus, spi_bus->txcnt, spi_bus->token);
        }

        return 0;
    }

    return 1;
}

void
spi_handle_irq(spi_bus_t* spi_bus)
{
    transfer_data(spi_bus);
}



int
spi_xfer(spi_bus_t* spi_bus, const void* txdata, size_t txcnt,
         void* rxdata, size_t rxcnt, spi_callback_fn cb, void* token)
{
    spi_bus->txbuf = (const char*)txdata;
    spi_bus->rxbuf = (char*)rxdata;
    spi_bus->rxsize = rxcnt;
    spi_bus->txsize = txcnt;
    spi_bus->rxcnt = 0;
    spi_bus->txcnt = 0;

    DSPI("Starting transfer: TX: from 0x%x, %d bytes. RX to 0x%x, %d bytes\n",
         (uint32_t)txdata, txcnt, (uint32_t)rxdata, rxcnt);

    transfer_data(spi_bus);
    spi_cs(spi_bus, SPI_CS_ASSERT);
    if (cb == NULL) {
        while (transfer_data(spi_bus));
    } else {
        uint32_t v;
        spi_bus->cb = cb;
        spi_bus->token = token;
        v = INT_EN_RX_OVERRUN  | INT_EN_RX_UNDERRUN
            | INT_EN_TX_OVERRUN  | INT_EN_TX_UNDERRUN
            | INT_EN_RX_FIFO_RDY | INT_EN_TX_FIFO_RDY
            | INT_EN_TRAILING;
        spi_bus->regs->pending_clr = 0xFFFFFFFF;
        spi_bus->regs->int_en = v;
    }

    return spi_bus->rxcnt;
}


long
spi_set_speed(spi_bus_t* spi_bus, long bps)
{
    return 0;
}

int
exynos_spi_init(enum spi_id id, void* base,
                mux_sys_t* mux_sys, clock_sys_t* clock_sys,
                spi_bus_t** ret_spi_bus)
{
    if (id >= 0 && id < NSPI) {
        spi_bus_t* spi_bus = _spi + id;
        *ret_spi_bus = spi_bus;
        spi_bus->regs = base;
        return spi_init_common(spi_bus, mux_sys, clock_sys);
    } else {
        return -1;
    }
}

int
spi_init(enum spi_id id, ps_io_ops_t* io_ops, spi_bus_t** ret_spi_bus)
{
    spi_bus_t* spi_bus = _spi + id;
    *ret_spi_bus = spi_bus;
    /* Map memory */
    DSPI("Mapping spi %d\n", id);
    switch (id) {
    case SPI0:
        MAP_IF_NULL(io_ops, EXYNOS_SPI0,      spi_bus->regs);
        break;
    case SPI1:
        MAP_IF_NULL(io_ops, EXYNOS_SPI1,      spi_bus->regs);
        break;
    case SPI2:
        MAP_IF_NULL(io_ops, EXYNOS_SPI2,      spi_bus->regs);
        break;
    case SPI0_ISP:
        MAP_IF_NULL(io_ops, EXYNOS_SPI0_ISP,  spi_bus->regs);
        break;
    case SPI1_ISP:
        MAP_IF_NULL(io_ops, EXYNOS_SPI1_ISP,  spi_bus->regs);
        break;
    default:
        return -1;
    }
    return spi_init_common(spi_bus, &io_ops->mux_sys, &io_ops->clock_sys);
}

