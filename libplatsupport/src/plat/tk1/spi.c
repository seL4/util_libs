/*
 * Copyright 2016, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(D61_BSD)
 */

#include <platsupport/spi.h>
#include <sel4/sel4.h>
#include <stdio.h>
#include <utils/ansi.h>

#define SPI0_PADDR 0x7000D400
#define SPI1_PADDR 0x7000D600
#define SPI2_PADDR 0x7000D800
#define SPI3_PADDR 0x7000DA00
#define SPI4_PADDR 0x7000DC00
#define SPI5_PADDR 0x7000DE00

#define PRINT_FUNC_NAME A_FG_G "%s " A_FG_RESET
#define XFER_SPI_FUNC_NAME A_FG_Y "%s " A_FG_RESET

#define SPI_CS_0                        0
#define SPI_CS_1                        1
#define SPI_CS_2                        2
#define SPI_CS_3                        3

/* COMMAND1 */
#define SPI_CMD1_GO			(1 << 31)
#define SPI_CMD1_M_S			(1 << 30)
#define SPI_CMD1_MODE_MASK		0x3
#define SPI_CMD1_MODE_SHIFT		28
#define SPI_CMD1_CS_SEL_MASK		0x3
#define SPI_CMD1_CS_SEL_SHIFT		26
#define SPI_CMD1_CS_POL_INACTIVE3	(1 << 25)
#define SPI_CMD1_CS_POL_INACTIVE2	(1 << 24)
#define SPI_CMD1_CS_POL_INACTIVE1	(1 << 23)
#define SPI_CMD1_CS_POL_INACTIVE0	(1 << 22)
#define SPI_CMD1_CS_SW_HW		(1 << 21)
#define SPI_CMD1_CS_SW_VAL		(1 << 20)
#define SPI_CMD1_IDLE_SDA_MASK		0x3
#define SPI_CMD1_IDLE_SDA_SHIFT		18
#define SPI_CMD1_BIDIR			(1 << 17)
#define SPI_CMD1_LSBI_FE		(1 << 16)
#define SPI_CMD1_LSBY_FE		(1 << 15)
#define SPI_CMD1_BOTH_EN_BIT		(1 << 14)
#define SPI_CMD1_BOTH_EN_BYTE		(1 << 13)
#define SPI_CMD1_RX_EN			(1 << 12)
#define SPI_CMD1_TX_EN			(1 << 11)
#define SPI_CMD1_PACKED			(1 << 5)
#define SPI_CMD1_BIT_LEN_MASK		0x1F
#define SPI_CMD1_BIT_LEN_SHIFT		0

/* COMMAND2 */
#define SPI_CMD2_TX_CLK_TAP_DELAY	(1 << 6)
#define SPI_CMD2_TX_CLK_TAP_DELAY_MASK	(0x3F << 6)
#define SPI_CMD2_RX_CLK_TAP_DELAY	(1 << 0)
#define SPI_CMD2_RX_CLK_TAP_DELAY_MASK	(0x3F << 0)

/* TRANSFER STATUS */
#define SPI_XFER_STS_RDY		(1 << 30)

/* FIFO STATUS */
#define SPI_FIFO_STS_CS_INACTIVE	(1 << 31)
#define SPI_FIFO_STS_FRAME_END		(1 << 30)
#define SPI_FIFO_STS_RX_FIFO_FLUSH	(1 << 15)
#define SPI_FIFO_STS_TX_FIFO_FLUSH	(1 << 14)
#define SPI_FIFO_STS_ERR		(1 << 8)
#define SPI_FIFO_STS_TX_FIFO_OVF	(1 << 7)
#define SPI_FIFO_STS_TX_FIFO_UNR	(1 << 6)
#define SPI_FIFO_STS_RX_FIFO_OVF	(1 << 5)
#define SPI_FIFO_STS_RX_FIFO_UNR	(1 << 4)
#define SPI_FIFO_STS_TX_FIFO_FULL	(1 << 3)
#define SPI_FIFO_STS_TX_FIFO_EMPTY	(1 << 2)
#define SPI_FIFO_STS_RX_FIFO_FULL	(1 << 1)
#define SPI_FIFO_STS_RX_FIFO_EMPTY	(1 << 0)

#define SPI_TX_FIFO_EMPTY_COUNT(val)		(((val) >> 16) & 0x7F)
#define SPI_RX_FIFO_FULL_COUNT(val)		(((val) >> 23) & 0x7F)

/* SPI CS Timing */
#define CS_SETUP_TIME_0_SHIFT 4
#define CS_HOLD_TIME_0_SHIFT 0

/* DMA CTL */
#define SPI_IE_TX				(1 << 28)
#define SPI_IE_RX				(1 << 29)

#define SPI_TIMEOUT		1000
#define TEGRA_SPI_MAX_FREQ	52000000

#define clrbits(addr, clear) \
          addr = (addr & ~(clear))

#define clrsetbits(addr, clear, set) \
          addr = ((addr & ~(clear)) | (set) )

#define setbits(addr, set) \
         addr = (addr | (set))

#define FIFO_SIZE 64

#define DARPA_CLK_TAP_DELAY 0x1f
#define DARPA_CS_SETUP_TIME 0x1e
#define DARPA_CS_HOLD_TIME 0x1e
#define DARPA_FIFO_MAX 60

enum spi_interrupt_type {
    RDY_BIT_SET,
    FIFO_ERROR
};

//#define SPI_DEBUG_XFER
//#define SPI_DEBUG_TRANSFER
//#define DEBUG_SPI
//#define DEBUG_IRQ

//#define SPI_DEBUG
//#define SPI_DEBUG_REGS

//#define SPI_DEBUG_DATA

//#define SPI_DEBUG_RX
//#define SPI_DEBUG_ERROR

#ifdef DEBUG_SPI_XFER
#define DSPI(args...) \
    do { \
        ZF_LOGE("SPI %s(%d):", __func__, __LINE__); \
        ZF_LOGE(args); \
        ZF_LOGE("\n"); \
    } while(0)
#else
#define DSPI(...) do{}while(0)
#endif

#ifdef SPI_DEBUG_RX
#define spi_debug_rx(fmt, args...)  ZF_LOGE(fmt, ##args)
#else
#define spi_debug_rx(fmt,args...)
#endif


#ifdef SPI_DEBUG_TRANSFER
#define spi_debug_transfer(fmt, args...) \
    do { \
         ZF_LOGE(fmt, ##args); \
         ZF_LOGE("\n"); \
    } while(0)
#else
#define spi_debug_transfer(fmt,args...)
#endif

#ifdef SPI_DEBUG
#define spi_debug(fmt, args...) \
    do { \
         ZF_LOGE(fmt, ##args); \
         ZF_LOGE("\n"); \
    } while(0)
#else
#define spi_debug(fmt,args...)
#endif

#ifdef SPI_DEBUG_ERROR
#define spi_debug_error(fmt, args...) \
    do { \
         ZF_LOGE(fmt, ##args); \
         ZF_LOGE("\n"); \
    } while(0)
#else
#define spi_debug_error(fmt,args...)
#endif

#ifdef SPI_DEBUG_XFER
#define spi_debug_xfer(fmt, args...) \
    do { \
         ZF_LOGE(fmt, ##args); \
         ZF_LOGE("\n"); \
    } while(0)
#else
#define spi_debug_xfer(fmt,args...)
#endif


#ifdef SPI_DEBUG_REGS
#define dump_regs() \
    do { \
        ZF_LOGE("%s, line: %d, cmd1: 0x%08x, fifo_stat: 0x%08x, xfer_stat: 0x%08x\n", \
                __func__, __LINE__, spi_bus->regs->command1, spi_bus->regs->fifo_status, spi_bus->regs->xfer_status); \
    } while(0)
#else
#define dump_regs() do{}while(0)
#endif

#ifdef SPI_DEBUG_DATA
#define spi_debug_data(fmt, args...)  ZF_LOGE(fmt, ##args)
#else
#define spi_debug_data(fmt,args...)
#endif

struct spi_controller_data {
    uint32_t data;
};

struct spi_regs {
    volatile uint32_t command1;	/* 000:SPI_COMMAND1 register */
    volatile uint32_t command2;	/* 004:SPI_COMMAND2 register */
    volatile uint32_t timing1;	/* 008:SPI_CS_TIM1 register */
    volatile uint32_t timing2;	/* 00c:SPI_CS_TIM2 register */
    volatile uint32_t xfer_status;/* 010:SPI_TRANS_STATUS register */
    volatile uint32_t fifo_status;/* 014:SPI_FIFO_STATUS register */
    volatile uint32_t tx_data;	/* 018:SPI_TX_DATA register */
    volatile uint32_t rx_data;	/* 01c:SPI_RX_DATA register */
    volatile uint32_t dma_ctl;	/* 020:SPI_DMA_CTL register */
    volatile uint32_t dma_blk;	/* 024:SPI_DMA_BLK register */
    volatile uint32_t rsvd[56];	/* 028-107 reserved */
    volatile uint32_t tx_fifo;	/* 108:SPI_FIFO1 register */
    volatile uint32_t rsvd2[31];	/* 10c-187 reserved */
    volatile uint32_t rx_fifo;	/* 188:SPI_FIFO2 register */
    volatile uint32_t spare_ctl;	/* 18c:SPI_SPARE_CTRL register */
};

unsigned int round_div(unsigned int dividend, unsigned int divisor)
{
    return (dividend + (divisor / 2)) / divisor;
}


struct spi_bus {
    volatile struct spi_regs* regs;
    uint32_t clock_mode;           // see table 147 on page 2436 of the TRM
    uint32_t clkid;
    uint8_t *txbuf;
    uint8_t *rxbuf;
    size_t txcnt, rxcnt;
    size_t txsize, rxsize, txtotal, rxtotal;
    seL4_CPtr spi_notification;
    struct spi_controller_data *controller_data;
    spi_callback_fn cb;
    int cs_auto;
    void* token;
    uint32_t bytes;
};

inline void start_transfer(spi_bus_t *spi_bus)
{
    setbits(spi_bus->regs->command1, SPI_CMD1_GO);
}

clock_sys_t clock_sys;

#define SPI0_OFFSET 0x400

uint32_t spi_controller_offsets[] = { SPI0_OFFSET };

static spi_bus_t _spi[1] = {
    { .regs = NULL, .clock_mode = 1 },
};


int get_spi_interrupt_type(spi_bus_t* spi_bus)
{
    if (spi_bus->regs->xfer_status & SPI_XFER_STS_RDY) {
        return RDY_BIT_SET;
    } else {
        return FIFO_ERROR;
    }
}

void spi_cs(spi_bus_t* spi_bus, enum spi_cs_state state)
{
    if (!spi_bus->cs_auto) {
        uint32_t v;
        v = spi_bus->regs->command1;
        if (state == SPI_CS_ASSERT) {
            setbits(v , SPI_CMD1_CS_SW_VAL);
        } else {
            clrbits(v , SPI_CMD1_CS_SW_VAL);
        }
        spi_bus->regs->command1 = v;
    }
}

int
tegra_spi_init(enum spi_id id, volatile void* base,
               mux_sys_t* mux_sys, clock_sys_t* clock_sys,
               spi_bus_t** ret_spi_bus)
{
    uint32_t fifo_status;
    spi_debug("%s line: %d spi_id: %d\n", __func__, __LINE__, id);
    spi_bus_t* spi_bus = _spi + id;

    spi_debug("%s line: %d\n", __func__, __LINE__);
    *ret_spi_bus = spi_bus;

    spi_debug("%s line: %d base: 0x%08x offset: 0x%08x\n", __func__, __LINE__, base, spi_controller_offsets[id]);
    spi_bus->regs = base + spi_controller_offsets[id];

    spi_debug("%s line: %d regs: 0x%08x\n", __func__, __LINE__, spi_bus->regs);
    fifo_status = spi_bus->regs->fifo_status;

    spi_debug("%s line: %d\n", __func__, __LINE__);
    fifo_status = SPI_FIFO_STS_ERR           |
                  SPI_FIFO_STS_TX_FIFO_OVF   |
                  SPI_FIFO_STS_TX_FIFO_UNR   |
                  SPI_FIFO_STS_RX_FIFO_OVF   |
                  SPI_FIFO_STS_RX_FIFO_UNR   |
                  SPI_FIFO_STS_TX_FIFO_FULL  |
                  SPI_FIFO_STS_TX_FIFO_EMPTY |
                  SPI_FIFO_STS_RX_FIFO_FULL  |
                  SPI_FIFO_STS_RX_FIFO_EMPTY;

    spi_debug("%s line: %d\n", __func__, __LINE__);
    spi_bus->regs->fifo_status = fifo_status;
    spi_debug("%s line: %d\n", __func__, __LINE__);
    spi_bus->regs->command1 = SPI_CMD1_M_S | (spi_bus->clock_mode << SPI_CMD1_MODE_SHIFT) |
                              SPI_CMD1_CS_POL_INACTIVE0	| SPI_CMD1_CS_SW_HW | SPI_CMD1_CS_SW_VAL;

    spi_debug("%s line: %d\n", __func__, __LINE__);
    return 0;
}


void print_fifo_error(uint32_t fifo_status)
{
    if (fifo_status & SPI_FIFO_STS_TX_FIFO_OVF) {
        spi_debug_error("tx FIFO overflow ");
    }
    if (fifo_status & SPI_FIFO_STS_TX_FIFO_UNR) {
        spi_debug_error("tx FIFO underrun ");
    }
    if (fifo_status & SPI_FIFO_STS_RX_FIFO_OVF) {
        spi_debug_error("rx FIFO overflow ");
    }
    if (fifo_status & SPI_FIFO_STS_RX_FIFO_UNR) {
        spi_debug_error("rx FIFO underrun ");
    }
    if (fifo_status & SPI_FIFO_STS_TX_FIFO_FULL) {
        spi_debug_error("tx FIFO full ");
    }
    if (fifo_status & SPI_FIFO_STS_TX_FIFO_EMPTY) {
        spi_debug_error("tx FIFO empty ");
    }
    if (fifo_status & SPI_FIFO_STS_RX_FIFO_FULL) {
        spi_debug_error("rx FIFO full ");
    }
    if (fifo_status & SPI_FIFO_STS_RX_FIFO_EMPTY) {
        spi_debug_error("rx FIFO empty ");
    }
    spi_debug_error("\n");
}

void
spi_prepare_transfer(spi_bus_t* spi_bus, const spi_slave_config_t* cfg)
{
    //TODO: implement support for multiple slaves
}

int transfer_data(spi_bus_t* spi_bus)
{
    volatile uint32_t stat, rxfifo_cnt;
    volatile uint32_t tmpdin;
    volatile uint32_t *rx_fifo_p;
    int txfifo_space;
    static int first_time = 1;

    spi_debug_transfer( PRINT_FUNC_NAME  "transfer started. Value=%08x, fifo_status = %08x\n",
                        __func__, *spi_bus->txbuf, spi_bus->regs->fifo_status);
    rx_fifo_p = &spi_bus->regs->rx_fifo;

    if ((spi_bus->txcnt == 0) && (spi_bus->rxcnt == 0)) {
        stat = spi_bus->regs->fifo_status;
        spi_bus->regs->fifo_status = stat;

        clrsetbits(spi_bus->regs->command1, SPI_CMD1_CS_SW_VAL,
                   SPI_CMD1_RX_EN | SPI_CMD1_TX_EN | SPI_CMD1_LSBY_FE |
                   (SPI_CS_0 << SPI_CMD1_CS_SEL_SHIFT));

        clrsetbits(spi_bus->regs->command1,
                   SPI_CMD1_BIT_LEN_MASK << SPI_CMD1_BIT_LEN_SHIFT,
                   (8 - 1) << SPI_CMD1_BIT_LEN_SHIFT );
        setbits(spi_bus->regs->xfer_status, SPI_XFER_STS_RDY);
        first_time = 1;
        setbits(spi_bus->regs->command2 , ( DARPA_CLK_TAP_DELAY & SPI_CMD2_RX_CLK_TAP_DELAY_MASK));
        setbits(spi_bus->regs->timing1 , (DARPA_CS_SETUP_TIME << CS_SETUP_TIME_0_SHIFT));
        setbits(spi_bus->regs->timing1 , (DARPA_CS_HOLD_TIME << CS_HOLD_TIME_0_SHIFT));
    }
    dump_regs();
    txfifo_space = SPI_TX_FIFO_EMPTY_COUNT(spi_bus->regs->fifo_status);

    spi_debug("%s, line: %d, txsize: %d, rxsize: %d, rxcnt: %d, txcnt: %d\n",
              __func__, __LINE__, spi_bus->txsize, spi_bus->rxsize, spi_bus->rxcnt,
              spi_bus->txcnt);
    spi_debug("%s, line: %d, txfifo_space: %d\n", __func__, __LINE__, txfifo_space);
    if (txfifo_space > ((int)(spi_bus->txsize + spi_bus->rxsize - spi_bus->rxcnt))) {
        //NOT, NOT! Whos there? One. One who? One or zero..........
        txfifo_space = MAX((int)(spi_bus->txsize + spi_bus->rxsize - spi_bus->txcnt), 0);
    }
    spi_debug("%s %d\n", __func__, __LINE__);
    if ((spi_bus->regs->xfer_status & SPI_XFER_STS_RDY) || first_time) {
        spi_debug("\n%s, line: %d, setting block size\n");
        spi_bus->regs->dma_blk = txfifo_space - 1;
        first_time = 0;
    }
    if (txfifo_space > DARPA_FIFO_MAX) {
        txfifo_space = DARPA_FIFO_MAX;
    }
    spi_debug("%s, line: %d, txfifo_space: %d\n \n", __func__, __LINE__, txfifo_space);
    dump_regs();
    while (txfifo_space--) {

        spi_debug_data("%s, line: %d, txbuf: 0x%x txfifo_space: %d\n",
                       __func__, __LINE__, *spi_bus->txbuf, txfifo_space);
        spi_bus->bytes = 1;
        dump_regs();
        spi_debug_data("%s, line: %d, txbuf: 0x%x\n", __func__, __LINE__, *spi_bus->txbuf);
        dump_regs();
        if (spi_bus->txcnt < spi_bus->txsize) {
            spi_bus->regs->tx_fifo = *spi_bus->txbuf;
            spi_bus->txbuf++;
        } else {
            spi_bus->regs->tx_fifo = 0;
        }
        spi_bus->txcnt++;
    }
    if (!(spi_bus->regs->fifo_status & SPI_FIFO_STS_TX_FIFO_EMPTY)) {
        start_transfer(spi_bus);
    }

    stat = spi_bus->regs->fifo_status;
    rxfifo_cnt = SPI_RX_FIFO_FULL_COUNT(stat);
    spi_debug("%s, line: %d, rxfifo_cnt: %d\n", __func__, __LINE__, rxfifo_cnt);
    dump_regs();

    while (rxfifo_cnt--) {

        uint32_t fifo_status, xfer_status;

        xfer_status = spi_bus->regs->xfer_status;
        if (!(xfer_status & SPI_XFER_STS_RDY)) {
            continue;
        }

        fifo_status = spi_bus->regs->fifo_status;

        if (fifo_status & SPI_FIFO_STS_ERR) {
            spi_debug_error("%s: got a fifo error: ", __func__);
            print_fifo_error(fifo_status);
            return 0;
            break;
        }
        spi_debug("%s, line: %d, rxfifo_cnt: %d rxfifo_cnt local: %d\n",
                  __func__, __LINE__, SPI_RX_FIFO_FULL_COUNT(spi_bus->regs->fifo_status), rxfifo_cnt);

        asm volatile("":::"memory");
        tmpdin = *rx_fifo_p;

        spi_debug("%s, line: %d, rxfifo_cnt: %d, bytes: %u\n",
                  __func__, __LINE__, SPI_RX_FIFO_FULL_COUNT(stat), spi_bus->bytes);

        if (NULL != spi_bus->rxbuf) {
            spi_debug_rx("%s, line: %d, rxdata: 0x%08x\n", __func__, __LINE__, (uint32_t)tmpdin);
            *spi_bus->rxbuf++ = (uint32_t)tmpdin & 0xff;
            spi_debug("%s, line: %d bytes: %d \n", __func__, __LINE__, spi_bus->bytes);
        }

        spi_bus->rxcnt += spi_bus->bytes;
    }
    dump_regs();
    spi_debug_transfer("%s, line: %d, rxcnt: %d, rxtotal: %d, txtotal: %d\n",
                       __func__, __LINE__, spi_bus->rxcnt, spi_bus->rxtotal, spi_bus->txtotal);
    if (spi_bus->rxcnt == spi_bus->rxtotal + spi_bus->txtotal) {
        setbits(spi_bus->regs->dma_ctl , (SPI_IE_TX | SPI_IE_RX));

        setbits(spi_bus->regs->xfer_status, SPI_XFER_STS_RDY);

        spi_debug("%s: transfer ended. Value=%08x, fifo_status = %08x\n",
                  __func__, tmpdin, spi_bus->regs->fifo_status);
        if (spi_bus->cb) {
            spi_debug("%s: line: %d calling call back\n", __func__, __LINE__);
            spi_bus->cb(spi_bus, spi_bus->txcnt, spi_bus->token);
        }
        spi_debug_transfer("%s: line: %d transfer complete\n", __func__, __LINE__);
        return 0;
    }

    spi_debug_transfer("%s: line: %d transfer incomplete\n", __func__, __LINE__);
    return 1;
}

void
spi_handle_irq(spi_bus_t* spi_bus)
{
#ifdef DEBUG_IRQ
    int int_type = get_spi_interrupt_type(spi_bus);
    if (int_type == RDY_BIT_SET) {
        ZF_LOGE("Interrupt RDY_BIT\n");
    } else {
        ZF_LOGE("Interrupt FIFO ERROR\n");
        print_fifo_error(spi_bus->regs->fifo_status);
    }
#endif
    transfer_data(spi_bus);
    setbits(spi_bus->regs->xfer_status, SPI_XFER_STS_RDY);
}


int
spi_xfer(spi_bus_t* spi_bus, const void* txdata, size_t txcnt,
         void* rxdata, size_t rxcnt, spi_callback_fn cb, void* token)
{
    spi_bus->txbuf = (uint8_t*)txdata;
    spi_bus->rxbuf = (uint8_t*)rxdata;
    spi_bus->rxsize = rxcnt;
    spi_bus->txsize = txcnt;
    spi_bus->txtotal = txcnt;
    spi_bus->rxtotal = rxcnt;
    spi_bus->rxcnt = 0;
    spi_bus->txcnt = 0;
    spi_bus->bytes = 0;

    DSPI("Starting transfer: TX: from 0x%x, %d bytes. RX to 0x%x, %d bytes\n",
         (uint32_t)txdata, txcnt, (uint32_t)rxdata, rxcnt);
    spi_debug(XFER_SPI_FUNC_NAME "line: %d\n", __func__, __LINE__);

    //Moving chip select to calling component so that it can use GPIO

    if (cb == NULL) {
        spi_debug( XFER_SPI_FUNC_NAME "calling with out callback\n");
        while (transfer_data(spi_bus));
    } else {
        uint32_t v;
        spi_bus->cb = cb;
        spi_bus->token = token;
        v = SPI_IE_TX | SPI_IE_RX;
        spi_bus->regs->dma_ctl = v;
        spi_debug( XFER_SPI_FUNC_NAME "line: %d calling with callback\n", __func__, __LINE__);
        transfer_data(spi_bus);
    }
    spi_debug_xfer( XFER_SPI_FUNC_NAME "line: %d\n", __func__, __LINE__);
    return spi_bus->rxcnt;
}
