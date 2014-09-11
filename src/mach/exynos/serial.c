/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/* disabled until someone makes it compile */

#include "serial.h"
#include "mux.h"
#include "platsupport/clock.h"

#include "../../common.h"
#include <platsupport/serial.h>

#define UART_DEBUG

#ifdef UART_DEBUG
#define DUART(...) printf("UART: " __VA_ARGS__)
#else
#define DUART(...) do{}while(0);
#endif

/*****************
 *** functions ***
 *****************/

#define ULCON       0x0000 /* line control */
#define UCON        0x0004 /* control */
#define UFCON       0x0008 /* fifo control */
#define UMCON       0x000C /* modem control */
#define UTRSTAT     0x0010 /* TX/RX status */
#define UERSTAT     0x0014 /* RX error status */
#define UFSTAT      0x0018 /* FIFO status */
#define UMSTAT      0x001C /* modem status */
#define UTXH        0x0020 /* TX buffer */
#define URXH        0x0024 /* RX buffer */
#define UBRDIV      0x0028 /* baud rate divisor */
#define UFRACVAL    0x002C /* divisor fractional value */
#define UINTP       0x0030 /* interrupt pending */
#define UINTSP      0x0034 /* interrupt source pending */
#define UINTM       0x0038 /* interrupt mask */


/* ULCON */
#define WORD_LENGTH_8   (3<<0)

/* UTRSTAT */
#define TX_EMPTY        (1<<2)
#define TXBUF_EMPTY     (1<<1)
#define RXBUF_READY     (1<<0)

/* UCON */
#define UCON_MODE_DISABLE 0x0
#define UCON_MODE_POLL    0x1
#define UCON_MODE_DMA     0x2
#define UCON_MODE_MASK    0x3
#define TXMODE(x)         (UCON_MODE_##x << 2)
#define RXMODE(x)         (UCON_MODE_##x << 0)

#define REG_PTR(base, offset)  ((volatile uint32_t *)((char*)(base) + (offset)))

static clk_t *clk;


static int uart_getchar(ps_chardevice_t *d)
{
    if (*REG_PTR(d->vaddr, UTRSTAT) & RXBUF_READY) {
        return *REG_PTR(d->vaddr, URXH);
    } else {
        return -1;
    }
}

static int uart_putchar(ps_chardevice_t *d, int c)
{
    /* Wait for serial to become ready. */
    while ( !(*REG_PTR(d->vaddr, UTRSTAT) & TXBUF_EMPTY) );

    /* Write out the next character. */
    *REG_PTR(d->vaddr, UTXH) = c;
    if (c == '\n') {
        uart_putchar(d, '\r');
    }

    return c;
}

static void uart_flush(ps_chardevice_t *d)
{
    while ( !(*REG_PTR(d->vaddr, UTRSTAT) & TX_EMPTY) );
}

static void uart_handle_irq(ps_chardevice_t *d UNUSED, int irq UNUSED)
{
    /*TODO*/
}

#define BRDIV_BITS 16
static int uart_set_baud(const ps_chardevice_t *d, long bps)
{
    long div_val, sclk_uart;
    uint32_t brdiv, brfrac;

    sclk_uart = UART_DEFAULT_FIN;/*clk_get_freq(clk)*/;
    div_val  = sclk_uart / bps - 16;
    /* Check if we need to scale down the clock */
    if (div_val / 16 >> BRDIV_BITS > 0) {
        assert(!"Not implemented");
        sclk_uart = 0;//clk_set_div(device_data_clk(d), div_val >> BRDIV_BITS);
        div_val  = sclk_uart / bps;
        /* Make sure that we have fixed the problem */
        if (div_val / 16 >> BRDIV_BITS > 0) {
            return -1;
        }
    }
    brfrac = div_val % 16;
    brdiv = (div_val / 16) & 0xffff;

    *REG_PTR(d->vaddr, UBRDIV) = brdiv;
    *REG_PTR(d->vaddr, UFRACVAL) = brfrac;
    return 0;
}

static int
uart_set_charsize(ps_chardevice_t* d, int char_size)
{
    uint32_t v;
    v = *REG_PTR(d->vaddr, ULCON);
    v &= ~(0x3 << 0);
    switch (char_size) {
    case 5:
        v |= (0x0 << 0);
        break;
    case 6:
        v |= (0x1 << 0);
        break;
    case 7:
        v |= (0x2 << 0);
        break;
    case 8:
        v |= (0x3 << 0);
        break;
    default :
        return -1;
    }
    *REG_PTR(d->vaddr, ULCON) = v;
    return 0;
}

static int
uart_set_stop(ps_chardevice_t *d, int stop_bits)
{
    uint32_t v;
    v = *REG_PTR(d->vaddr, ULCON);
    v &= ~(0x1 << 2);
    switch (stop_bits) {
    case 1:
        v |= (0x0 << 2);
        break;
    case 2:
        v |= (0x1 << 2);
        break;
    default :
        return -1;
    }
    *REG_PTR(d->vaddr, ULCON) = v;
    return 0;
}

static int
uart_set_parity(ps_chardevice_t *d, enum serial_parity parity)
{
    uint32_t v;
    v = *REG_PTR(d->vaddr, ULCON);
    v &= ~(0x7 << 3);
    switch (parity) {
    case PARITY_EVEN:
        v |= (0x5 << 3);
        break;
    case PARITY_ODD:
        v |= (0x4 << 3);
        break;
    case PARITY_NONE:
        v |= (0x0 << 3);
        break;
    default :
        return -1;
    }
    *REG_PTR(d->vaddr, ULCON) = v;
    return 0;
}

int
uart_configure(ps_chardevice_t *d, long bps, int char_size,
               enum serial_parity parity, int stop_bits)
{
    return uart_set_baud(d, bps)
           || uart_set_parity(d, parity)
           || uart_set_charsize(d, char_size)
           || uart_set_stop(d, stop_bits);
}

int uart_init(const struct dev_defn* defn,
              const ps_io_ops_t* ops,
              ps_chardevice_t* dev)
{

    int v;
    void* vaddr = chardev_map(defn, ops);
    if (vaddr == NULL) {
        return -1;
    }
    dev->id         = defn->id;
    dev->vaddr      = vaddr;
    dev->getchar    = &uart_getchar;
    dev->putchar    = &uart_putchar;
    dev->handle_irq = &uart_handle_irq;
    dev->irqs       = defn->irqs;
    dev->rxirqcb    = NULL;
    dev->txirqcb    = NULL;
    dev->ioops      = *ops;

    /* TODO */
    dev->clk        = NULL;

    uart_flush(dev);

    /* reset and initialise hardware */
#ifdef PLAT_EXYNOS5
    if (mux_sys_valid(&ops->mux_sys)) {
        /* The interface declares ops as a const, but this
         * can no longer be true as we may need to modify the
         * MUX/CLOCK. The interface should be updated. Until it
         * is, we have a dirty cast. */
        mux_sys_t* mux_sys = (mux_sys_t*)&ops->mux_sys;
        int err;
        switch (dev->id) {
        case PS_SERIAL0:
            err = mux_feature_enable(mux_sys, MUX_UART0);
            break;
        case PS_SERIAL1:
            err = mux_feature_enable(mux_sys, MUX_UART1);
            break;
        case PS_SERIAL2:
            err = mux_feature_enable(mux_sys, MUX_UART2);
            break;
        case PS_SERIAL3:
            err = mux_feature_enable(mux_sys, MUX_UART3);
            break;
        default:
            printf("Invalid UART ID %d! Could not initialise MUX\n", dev->id);
            err = 0;
        }
        if (err) {
            printf("Failed to initialise MUX for UART %d\n", dev->id);
        }

    } else {
        printf("INFO: Skipping MUX initialisation for UART %d\n", dev->id);
    }
#endif

    /* TODO: Use correct clock source */
    if (clock_sys_valid(&ops->clock_sys)) {
        clk = clk_get_clock(&dev->ioops.clock_sys, CLK_MASTER);
    } else {
        clk = NULL;
    }

    /* Set character encoding */
    assert(!uart_configure(dev, 115200UL, 8, PARITY_NONE, 1));

    /* Enable TX/RX */
    v = *REG_PTR(dev->vaddr, UCON);
    v &= ~(TXMODE(MASK) | RXMODE(MASK));
    v |=  (TXMODE(POLL) | RXMODE(POLL));
    *REG_PTR(dev->vaddr, UCON) = v;

    return 0;
}

