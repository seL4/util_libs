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

#include <stdlib.h>
#include <platsupport/serial.h>
#include <platsupport/plat/serial.h>
#include <string.h>

#include "../../chardev.h"

#define UART_REF_CLK            50000000

/* CR */
#define UART_CR_RXRES           BIT( 0)
#define UART_CR_TXRES           BIT( 1)
#define UART_CR_RXEN            BIT( 2)
#define UART_CR_RXDIS           BIT( 3)
#define UART_CR_TXEN            BIT( 4)
#define UART_CR_TXDIS           BIT( 5)
#define UART_CR_RSTTO           BIT( 6)
#define UART_CR_STTBRK          BIT( 7)
#define UART_CR_STPBRK          BIT( 8)
/* MR */
#define UART_MR_CLKS            BIT( 0)
#define UART_MR_CHRL(x)         ((x) * BIT(1))
#define UART_MR_CHRL_MASK       UART_MR_CHRL(0x3)
#define UART_MR_PAR(x)          ((x) * BIT(3))
#define UART_MR_PAR_MASK        UART_MR_PAR(0x7)
#define UART_MR_NBSTOP(x)       ((x) * BIT(6))
#define UART_MR_NBSTOP_MASK     UART_MR_NBSTOP(0x3)
#define UART_MR_CHMODE(x)       ((x) * BIT(8))
#define UART_MR_CHMODE_MASK     UART_MR_CHMODE(0x3)
/* IER */
#define UART_IER_RTRIG          BIT( 0)
#define UART_IER_REMPTY         BIT( 1)
#define UART_IER_RFUL           BIT( 2)
#define UART_IER_TEMPTY         BIT( 3)
#define UART_IER_TFUL           BIT( 4)
#define UART_IER_ROVR           BIT( 5)
#define UART_IER_FRAME          BIT( 6)
#define UART_IER_PARE           BIT( 7)
#define UART_IER_TIMEOUT        BIT( 8)
#define UART_IER_DMSI           BIT( 9)
#define UART_IER_TTRIG          BIT(10)
#define UART_IER_TNFUL          BIT(11)
#define UART_IER_TOVR           BIT(12)
/* IDR */
#define UART_IDR_RTRIG          BIT( 0)
#define UART_IDR_REMPTY         BIT( 1)
#define UART_IDR_RFUL           BIT( 2)
#define UART_IDR_TEMPTY         BIT( 3)
#define UART_IDR_TFUL           BIT( 4)
#define UART_IDR_ROVR           BIT( 5)
#define UART_IDR_FRAME          BIT( 6)
#define UART_IDR_PARE           BIT( 7)
#define UART_IDR_TIMEOUT        BIT( 8)
#define UART_IDR_DMSI           BIT( 9)
#define UART_IDR_TTRIG          BIT(10)
#define UART_IDR_TNFUL          BIT(11)
#define UART_IDR_TOVR           BIT(12)
/* IMR */
#define UART_IMR_RTRIG          BIT( 0)
#define UART_IMR_REMPTY         BIT( 1)
#define UART_IMR_RFUL           BIT( 2)
#define UART_IMR_TEMPTY         BIT( 3)
#define UART_IMR_TFUL           BIT( 4)
#define UART_IMR_ROVR           BIT( 5)
#define UART_IMR_FRAME          BIT( 6)
#define UART_IMR_PARE           BIT( 7)
#define UART_IMR_TIMEOUT        BIT( 8)
#define UART_IMR_DMSI           BIT( 9)
#define UART_IMR_TTRIG          BIT(10)
#define UART_IMR_TNFUL          BIT(11)
#define UART_IMR_TOVR           BIT(12)
/* ISR */
#define UART_ISR_RTRIG          BIT( 0)
#define UART_ISR_REMPTY         BIT( 1)
#define UART_ISR_RFUL           BIT( 2)
#define UART_ISR_TEMPTY         BIT( 3)
#define UART_ISR_TFUL           BIT( 4)
#define UART_ISR_ROVR           BIT( 5)
#define UART_ISR_FRAME          BIT( 6)
#define UART_ISR_PARE           BIT( 7)
#define UART_ISR_TIMEOUT        BIT( 8)
#define UART_ISR_DMSI           BIT( 9)
#define UART_ISR_TTRIG          BIT(10)
#define UART_ISR_TNFUL          BIT(11)
#define UART_ISR_TOVR           BIT(12)
/* RXTOUT */
#define UART_RXTOUT_RTO(x)      ((x) * BIT(0))
#define UART_RXTOUT_RTO_MASK    UART_RXTOUT_RTO(0xFF)
/* RXWM */
#define UART_RXWM_RTRIG(x)      ((x) * BIT(0))
#define UART_RXWM_RTRIG_MASK    UART_RXWM_RTRIG(0x3F)
/* MODEMCR */
#define UART_MODEMCR_DTR        BIT( 0)
#define UART_MODEMCR_RTS        BIT( 1)
#define UART_MODEMCR_FCM        BIT( 5)
/* MODEMSR */
#define UART_MODEMSR_DCTS       BIT( 0)
#define UART_MODEMSR_DDSR       BIT( 1)
#define UART_MODEMSR_TERI       BIT( 2)
#define UART_MODEMSR_DDCD       BIT( 3)
#define UART_MODEMSR_CTS        BIT( 4)
#define UART_MODEMSR_DSR        BIT( 5)
#define UART_MODEMSR_RI         BIT( 6)
#define UART_MODEMSR_DCD        BIT( 7)
#define UART_MODEMSR_FCMS       BIT( 8)
/* SR */
#define UART_SR_RTRIG           BIT( 0)
#define UART_SR_REMPTY          BIT( 1)
#define UART_SR_RFUL            BIT( 2)
#define UART_SR_TEMPTY          BIT( 3)
#define UART_SR_TFUL            BIT( 4)
#define UART_SR_RACTIVE         BIT(10)
#define UART_SR_TACTIVE         BIT(11)
#define UART_SR_FDELT           BIT(12)
#define UART_SR_TTRIG           BIT(13)
#define UART_SR_TNFUL           BIT(14)
/* FLOWDIV */
#define UART_FLOWDIV_FDEL(x)    ((x) * BIT(0))
#define UART_FLOWDIV_FDEL_MASK  UART_FLOWDIV_FDEL(0x3F)
/* TXWM */
#define UART_TXWM_TTRIG(x)      ((x) * BIT(0))
#define UART_TXWM_TTRIG_MASK    UART_TXWM_TTRIG(0x3F)

/* Baud rate dividers */
#define UART_BAUDDIV_BDIV_MIN   4
#define UART_BAUDDIV_BDIV_MAX   255
#define UART_BAUDGEN_CD_MIN     1
#define UART_BAUDGEN_CD_MAX     65535

/* Fifo size */
#define UART_TX_FIFO_SIZE 64
#define UART_RX_FIFO_SIZE 64

struct zynq7000_uart_regs {
    uint32_t cr;            /* 0x00 Control Register */
    uint32_t mr;            /* 0x04 Mode Register */
    uint32_t ier;           /* 0x08 Interrupt Enable Register */
    uint32_t idr;           /* 0x0C Interrupt Disable Register */
    uint32_t imr;           /* 0x10 Interrupt Mask Register */
    uint32_t isr;           /* 0x14 Channel Interrupt Status Register */
    uint32_t baudgen;       /* 0x18 Baud Rate Generator Register */
    uint32_t rxtout;        /* 0x1C Receiver Timeout Register */
    uint32_t rxwm;          /* 0x20 Receiver FIFO Trigger Level Register */
    uint32_t modemcr;       /* 0x24 Modem Control Register */
    uint32_t modemsr;       /* 0x28 Modem Status Register */
    uint32_t sr;            /* 0x2C Channel Status Register */
    uint32_t fifo;          /* 0x30 Transmit and Receive FIFO */
    uint32_t bauddiv;       /* 0x34 Baud Rate Divider Register */
    uint32_t flowdel;       /* 0x38 Flow Control Delay Register */
    uint32_t pad[2];
    uint32_t txwm;          /* 0x44 Transmitter FIFO Trigger Level Register */
};
typedef volatile struct zynq7000_uart_regs zynq7000_uart_regs_t;

static inline zynq7000_uart_regs_t*
zynq7000_uart_get_priv(ps_chardevice_t *d)
{
    return (zynq7000_uart_regs_t*)d->vaddr;
}

static inline void
zynq7000_uart_enable_tx(zynq7000_uart_regs_t* regs)
{
    regs->cr &= ~UART_CR_TXDIS;
    regs->cr |= UART_CR_TXEN;
}

static inline void
zynq7000_uart_enable_rx(zynq7000_uart_regs_t* regs)
{
    regs->cr &= ~UART_CR_RXDIS;
    regs->cr |= UART_CR_RXEN;
}

int uart_getchar(ps_chardevice_t *d)
{
    zynq7000_uart_regs_t* regs = zynq7000_uart_get_priv(d);
    int c = -1;

    if (!(regs->sr & UART_SR_REMPTY)) {
        c = regs->fifo;

        /* Clear the Rx timeout interrupt status bit if set */
        if (regs->isr & UART_ISR_TIMEOUT) {
            regs->isr &= ~UART_ISR_TIMEOUT;
        }
    }
    return c;
}

int uart_putchar(ps_chardevice_t* d, int c)
{
    zynq7000_uart_regs_t* regs = zynq7000_uart_get_priv(d);

    if (c == '\n' && (d->flags & SERIAL_AUTO_CR)) {
        /* check if 2 bytes are free - tx trigger level is 63 and
         * this bit is set if the fifo level is >= the trigger level
         */
        if (!(regs->sr & UART_SR_TTRIG)) {

            regs->fifo = '\r';
            regs->fifo = '\n';

            return '\n';
        }
    } else if (!(regs->sr & UART_SR_TFUL)) {
        regs->fifo = c;
        return c;
    }

    return -1;
}

static void
uart_handle_irq(ps_chardevice_t* d UNUSED)
{
    /* TODO */
}

/*
 * Calculate the baud rate divisors
 * @param    clk: UART module input clock
 * @param   baud: Desired baud rate
 * @param  rdiv8: Calculated MR[CLKS] clock source select value (returned)
 * @param    rcd: Calculated BAUDGEN[CD] baud rate clock divisor value (returned)
 * @param  rbdiv: Calculated BAUDDIV[BDIV] baud rate divider value (returned)
 * @return      : The actual baud rate based on the calculated values
 */
static long
zynq7000_uart_calc_baud_divs(long clk, long baud, unsigned int* rdiv8, uint32_t* rcd, uint32_t* rbdiv)
{
    /* Safety checks */
    assert(rdiv8 != NULL);
    assert(rcd != NULL);
    assert(rbdiv != NULL);

    uint32_t cd, bdiv;
    long rbaud;
    unsigned int calc_baud, baud_error, best_baud_error = ~0;

    /* Calculate the UART clock divisor */
    if (baud < clk / ((UART_BAUDDIV_BDIV_MAX + 1) * UART_BAUDGEN_CD_MAX)) {
        *rdiv8 = 1;
        clk /= 8;
    } else {
        *rdiv8 = 0;
    }

    /* Calculate values for CD and BDIV based on the desired baud rate */
    for (bdiv = UART_BAUDDIV_BDIV_MIN; bdiv <= UART_BAUDDIV_BDIV_MAX; bdiv++) {
        cd = clk / (baud * (bdiv + 1));
        if (cd < UART_BAUDGEN_CD_MIN || cd > UART_BAUDGEN_CD_MAX) {
            continue;
        }

        calc_baud = clk / (cd * (bdiv + 1));

        if (baud > calc_baud) {
            baud_error = baud - calc_baud;
        } else {
            baud_error = calc_baud - baud;
        }

        if (baud_error < best_baud_error) {
            best_baud_error = baud_error;
            *rcd = cd;
            *rbdiv = bdiv;
            rbaud = calc_baud;

            /* Short-circuit */
            if (baud_error == 0) {
                break;
            }
        }
    }

    return rbaud;
}

/*
 * baud rate = clk / BAUDGEN.CD * (BAUDDIV.BDIV + 1)
 * BAUDGEN.CD is 16 bit, BAUDDIV.BDIV is 8 bit
 */
static void
zynq7000_uart_set_baud(ps_chardevice_t* d, long bps)
{
    zynq7000_uart_regs_t* regs = zynq7000_uart_get_priv(d);
    uint32_t cd = 0;
    uint32_t bdiv = 0;
    unsigned int div8;

    zynq7000_uart_calc_baud_divs(UART_REF_CLK, bps, &div8, &cd, &bdiv);

    /* Disable the Rx path */
    regs->cr &= ~UART_CR_RXEN;

    /* Disable the Tx path */
    regs->cr &= ~UART_CR_TXEN;

    /* Apply the calculated values */
    if (div8) {
        regs->mr |= UART_MR_CLKS;
    } else {
        regs->mr &= ~UART_MR_CLKS;
    }

    regs->baudgen = cd;
    regs->bauddiv = bdiv;

    /* Reset the Tx and Rx paths */
    regs->cr |= UART_CR_TXRES | UART_CR_RXRES;
    while (regs->cr & (UART_CR_TXRES | UART_CR_RXRES));

    /* Enable the Rx path */
    zynq7000_uart_enable_rx(regs);

    /* Enable the Tx path */
    zynq7000_uart_enable_tx(regs);
}

int
serial_configure(ps_chardevice_t* d, long bps, int char_size, enum serial_parity parity, int stop_bits)
{
    zynq7000_uart_regs_t* regs = zynq7000_uart_get_priv(d);
    uint32_t mr;

    /* Character size */
    mr = regs->mr;
    if (char_size == 6) {
        mr &= ~UART_MR_CHRL_MASK;
        mr |= UART_MR_CHRL(0x3);
    } else if (char_size == 7) {
        mr &= ~UART_MR_CHRL_MASK;
        mr |= UART_MR_CHRL(0x2);
    } else if (char_size == 8) {
        mr &= ~UART_MR_CHRL_MASK;
    } else {
        return -1;
    }
    /* Stop bits */
    if (stop_bits == 1) {
        mr &= ~UART_MR_NBSTOP_MASK;
    } else if (stop_bits == 2) {
        mr &= ~UART_MR_NBSTOP_MASK;
        mr |= UART_MR_NBSTOP(0x2);
        /* Do not handle 1.5 stop bits for now */
    } else {
        return -1;
    }
    /* Parity */
    if (parity == PARITY_EVEN) {
        mr &= ~UART_MR_PAR_MASK;
    } else if (parity == PARITY_ODD) {
        mr &= ~UART_MR_PAR_MASK;
        mr |= UART_MR_PAR(0x1);
        /* Do not handle forced to 0 parity (space) for now */
        /* Do not handle forced to 1 parity (mark) for now */
    } else if (parity == PARITY_NONE) {
        mr &= ~UART_MR_PAR_MASK;
        mr |= UART_MR_PAR(0x7);
    } else {
        return -1;
    }
    /* Apply the changes */
    regs->mr = mr;
    /* Now set the board rate */
    zynq7000_uart_set_baud(d, bps);
    return 0;
}

int uart_init(const struct dev_defn* defn,
              const ps_io_ops_t* ops,
              ps_chardevice_t* dev)
{
    zynq7000_uart_regs_t* regs;

    /* Attempt to map the virtual address, assure this works */
    void* vaddr = chardev_map(defn, ops);
    if (vaddr == NULL) {
        return -1;
    }

    memset(dev, 0, sizeof(*dev));

    /* Set up all the  device properties. */
    dev->id         = defn->id;
    dev->vaddr      = (void*)vaddr;
    dev->read       = &uart_read;
    dev->write      = &uart_write;
    dev->handle_irq = &uart_handle_irq;
    dev->irqs       = defn->irqs;
    dev->ioops      = *ops;
    dev->flags      = SERIAL_AUTO_CR;

    regs = zynq7000_uart_get_priv(dev);

    /* Software reset */
    // TODO - UART software reset is done through a different register (UART_RST_CTRL)
    // TODO - does the I/O signal routing have to be configured here too?

    /* Configure UART character frame */
    serial_configure(dev, 115200, 8, PARITY_NONE, 1);

    /* Set the level of the RxFIFO trigger level */
    regs->rxwm &= ~UART_RXWM_RTRIG_MASK;    /* Clear the Rx trigger level */
    regs->rxwm |= UART_RXWM_RTRIG(1);       /* Set the Rx trigger level to 1 */

    /* Enable the RTRIG interrupt */
    regs->ier |= UART_IER_RTRIG;            /* Set the interrupt enable bit */
    regs->idr &= ~UART_IDR_RTRIG;           /* Clear the interrupt disable bit */
    if (!(regs->imr & UART_IMR_RTRIG)) {    /* Verify the interrupt mask value */
        return -1;
    }

    /* Enable the controller */
    regs->cr |= UART_CR_TXRES;              /* Reset Tx path */
    regs->cr |= UART_CR_RXRES;              /* Reset Rx path */
    zynq7000_uart_enable_rx(regs);          /* Enable the Rx path */
    zynq7000_uart_enable_tx(regs);          /* Enable the Tx path */
    regs->cr |= UART_CR_RSTTO;              /* Restart the receiver timeout counter */
    regs->cr &= ~UART_CR_STTBRK;            /* Do not start to transmit a break */
    regs->cr |= UART_CR_STPBRK;             /* Stop break transmitter */

    /* Program the receiver timeout mechanism */
    regs->rxtout &= ~UART_RXTOUT_RTO_MASK;  /* Disable the timeout mechanism */

    /* set the tx trigger to one less than the fifo size so it's possible to check
     * if there are 2 bytes free
     */
    regs->txwm = UART_TXWM_TTRIG(UART_TX_FIFO_SIZE - 1) & UART_TXWM_TTRIG_MASK;

    return 0;
}
