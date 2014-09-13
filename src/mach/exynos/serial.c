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
#include <string.h>

#define UART_DEBUG

#ifdef UART_DEBUG
#define DUART(...) printf("UART: " __VA_ARGS__)
#else
#define DUART(...) do{}while(0);
#endif

/* TX FIFO IRQ threshold ratio {0..7}.
 * The absolute value depends on the FIFO size of the individual UART
 * A high value avoids underruns, but increases IRQ overhead */
#define FIFO_TXLVL_VAL   2

/* RX FIFO IRQ threshold ratio {0..7}.
 * The absolute value depends on the FIFO size of the individual UART
 * A low value avoids overruns, but increases IRQ overhead */
#define FIFO_RXLVL_VAL   2

/* Timeout on RX {0..15} */
#define RX_TIMEOUT_VAL   15 /* 8*(N + 1) frames */


#define ULCON       0x0000 /* line control */
#define UCON        0x0004 /* control */
#define UFCON       0x0008 /* fifo control */
#define UMCON       0x000C /* modem control */
#define UTRSTAT     0x0010 /* TX/RX status */
#define UERSTAT     0x0014 /* RX error status */
#define UFRSTAT     0x0018 /* FIFO status */
#define UMSTAT      0x001C /* modem status */
#define UTXH        0x0020 /* TX buffer */
#define URXH        0x0024 /* RX buffer */
#define UBRDIV      0x0028 /* baud rate divisor */
#define UFRACVAL    0x002C /* divisor fractional value */
#define UINTP       0x0030 /* interrupt pending */
#define UINTSP      0x0034 /* interrupt source pending */
#define UINTM       0x0038 /* interrupt mask */


/* UTRSTAT */
#define TRSTAT_RXTIMEOUT      BIT(3)
#define TRSTAT_TX_EMPTY       BIT(2)
#define TRSTAT_TXBUF_EMPTY    BIT(1)
#define TRSTAT_RXBUF_READY    BIT(0)

/* FRSTAT */
#define FRSTAT_TX_FULL        BIT(24)
#define FRSTAT_GET_TXFIFO(x)  (((x) >> 16) & 0xff)
#define FRSTAT_RXFIFO_ERR     BIT(9)
#define FRSTAT_RXFIFO_FULL    BIT(8)
#define FRSTAT_GET_RXFIFO(x)  (((x) >>  0) & 0xff)


/* UCON */
#define CON_MODE_DISABLE      0x0
#define CON_MODE_POLL         0x1
#define CON_MODE_DMA          0x2
#define CON_MODE_MASK         0x3
#define CON_TXMODE(x)         (CON_MODE_##x << 2)
#define CON_RXMODE(x)         (CON_MODE_##x << 0)
#define CON_RX_TIMEOUT(x)     (((x) & 0xf) << 12)
#define CON_RX_TIMEOUT_MASK   CON_RX_TIMEOuT(0xf)
#define CON_RX_TIMEOUT_EMPTY  BIT(11)
#define CON_TXIRQTYPE_LEVEL   BIT(9)
#define CON_RXIRQTYPE_LEVEL   BIT(8)
#define CON_RXTIMEOUT_ENABLE  BIT(7)
#define CON_RXERR_IRQ_EN      BIT(6)
/* FIFO control */
#define FIFO_EN               BIT(0)
#define FIFO_RX_RESET         BIT(1)
#define FIFO_TX_RESET         BIT(2)
#define FIFO_TXLVL(x)         ((x) << 8)
#define FIFO_TXLVL_MASK       FIFO_TXLVL(0x7)
#define FIFO_RXLVL(x)         ((x) << 4)
#define FIFO_RXLVL_MASK       FIFO_RXLVL(0x7)

/* INTP, INTSP, INTM */
#define INT_MODEM BIT(3)
#define INT_TX    BIT(2)
#define INT_ERR   BIT(1)
#define INT_RX    BIT(0)


#define REG_PTR(base, offset)  ((volatile uint32_t *)((char*)(base) + (offset)))

static clk_t *clk;

enum mux_feature uart_mux[] = {
                                  [PS_SERIAL0] = MUX_UART0,
                                  [PS_SERIAL1] = MUX_UART1,
                                  [PS_SERIAL2] = MUX_UART2,
                                  [PS_SERIAL3] = MUX_UART3
                              };

static int
uart_putchar(ps_chardevice_t *d, int c)
{
    if (*REG_PTR(d->vaddr, UFRSTAT) & FRSTAT_TX_FULL) {
        /* abort: no room in FIFO */
        return -1;
    } else {
        /* Write out the next character. */
        *REG_PTR(d->vaddr, UTXH) = c;
        if (c == '\n') {
            /* In this case, We should have checked that we had two free bytes in
             * the FIFO before we submitted the first char, however, the fifo size
             * would need to be considered and this differs between UARTs.
             * To keep things simple, we recognise that it is rare for a '\n' to
             * be sent when there is insufficent FIFO space and accept the
             * inefficiencies of spinning, waiting for space.
             */
            while (uart_putchar(d, '\r') < 0);
        }
        return c;
    }
}

static int
uart_fill_fifo(ps_chardevice_t *d, const char* data, size_t len)
{
    int i;
    for (i = 0; i < len; i++) {
        if (uart_putchar(d, *data++) < 0) {
            return i;
        }
    }
    return len;
}


static ssize_t
uart_write(ps_chardevice_t* d, const void* vdata, size_t count, chardev_callback_t wcb, void* token)
{
    const char* data = (const char*)vdata;
    int sent;
    if (d->write_descriptor.data) {
        /* Transaction is already in progress */
        return -1;
    }
    /* Fill the FIFO */
    sent = uart_fill_fifo(d, data, count);
    if (wcb) {
        /* Register the callback */
        d->write_descriptor.callback = wcb;
        d->write_descriptor.token = token;
        d->write_descriptor.bytes_transfered = sent;
        d->write_descriptor.bytes_requested = count;
        d->write_descriptor.data = (void*)data + sent;
        /* Enable TX IRQ */
        *REG_PTR(d->vaddr, UINTP) = INT_TX;
        *REG_PTR(d->vaddr, UINTM) &= ~INT_TX;
    }
    return sent;
}

static void
uart_handle_tx_irq(ps_chardevice_t* d)
{
    int sent;
    int to_send;
    /* pipe more data onto the fifo */
    to_send = d->write_descriptor.bytes_requested - d->write_descriptor.bytes_transfered;
    sent = uart_fill_fifo(d, d->write_descriptor.data, to_send);
    d->write_descriptor.bytes_transfered += sent;
    d->write_descriptor.data += sent;

    /* Check if this transaction is complete */
    if (d->write_descriptor.bytes_transfered == d->write_descriptor.bytes_requested) {
        /* Shutdown IRQs */
        *REG_PTR(d->vaddr, UINTM) |= INT_TX;
        d->write_descriptor.data = NULL;
        /* Signal completion */
        d->write_descriptor.callback(d, CHARDEV_STAT_COMPLETE,
                                     d->write_descriptor.bytes_transfered,
                                     d->write_descriptor.token);
    }

    /* Clear the pending flag, ready for the next IRQ */
    *REG_PTR(d->vaddr, UINTP) = INT_TX;
}

static int
uart_getchar(ps_chardevice_t *d)
{
    if (*REG_PTR(d->vaddr, UTRSTAT) & TRSTAT_RXBUF_READY) {
        return *REG_PTR(d->vaddr, URXH);
    } else {
        return -1;
    }
}

static int
uart_read_fifo(ps_chardevice_t *d, char* data, size_t len)
{
    int i;
    for (i = 0; i < len; i++) {
        int c;
        c = uart_getchar(d);
        if (c < 0) {
            break;
        }
        *data++ = c;
    }
    /* Clear the pending flag */
    *REG_PTR(d->vaddr, UINTP) = INT_RX;
    return i;
}

static ssize_t
uart_read(ps_chardevice_t* d, void* vdata, size_t count, chardev_callback_t rcb, void* token)
{
    char *data = (char*)vdata;
    int read;
    if (d->read_descriptor.data) {
        /* Transaction is already in progress */
        return -1;
    }
    read = uart_read_fifo(d, data, count);
    if (rcb) {
        /* Register the callback */
        d->read_descriptor.callback = rcb;
        d->read_descriptor.token = token;
        d->read_descriptor.bytes_transfered = read;
        d->read_descriptor.bytes_requested = count;
        d->read_descriptor.data = (void*)data + read;
        /* RX IRQ always enabled */
    }
    return read;
}


static void
uart_handle_rx_irq(ps_chardevice_t* d)
{
    int timeout;
    uint32_t v;
    int read;
    int to_read;
    /* If a callback was not registered, simply return. User is expected
     * to read FIFO data with a call to 'read' */
    if (d->read_descriptor.data == NULL) {
        return;
    }
    /* Check for timeout */
    v = *REG_PTR(d->vaddr, UTRSTAT);
    timeout = v & TRSTAT_RXTIMEOUT;
    /* Clear timeout condition */
    *REG_PTR(d->vaddr, UTRSTAT) = v;

    to_read = d->read_descriptor.bytes_requested - d->read_descriptor.bytes_transfered;
    read = uart_read_fifo(d, d->read_descriptor.data, to_read);
    d->read_descriptor.bytes_transfered += read;
    d->read_descriptor.data += read;

    /* Check if this transaction is complete */
    if (timeout || d->read_descriptor.bytes_transfered == d->read_descriptor.bytes_requested) {
        d->read_descriptor.data = NULL;
        /* Signal completion */
        d->read_descriptor.callback(d, CHARDEV_STAT_COMPLETE,
                                    d->read_descriptor.bytes_transfered,
                                    d->read_descriptor.token);
    }
}

static void uart_flush(ps_chardevice_t *d)
{
    while ( !(*REG_PTR(d->vaddr, UTRSTAT) & TRSTAT_TX_EMPTY) );
}

static void
uart_handle_irq(ps_chardevice_t *d)
{
    uint32_t sts;
    sts = *REG_PTR(d->vaddr, UINTP);
    if (sts & INT_TX) {
        sts &= ~INT_TX;
        uart_handle_tx_irq(d);
    }
    if (sts & INT_RX) {
        sts &= ~INT_RX;
        uart_handle_rx_irq(d);
    }
    if (sts) {
        DUART("Unhandled IRQ (status 0x%x)\n", sts);
    }
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
    memset(dev, 0, sizeof(*dev));
    dev->id         = defn->id;
    dev->vaddr      = vaddr;
    dev->read       = &uart_read;
    dev->write      = &uart_write;
    dev->handle_irq = &uart_handle_irq;
    dev->irqs       = defn->irqs;
    dev->ioops      = *ops;

    /* TODO */
    dev->clk        = NULL;

    uart_flush(dev);

    /* reset and initialise hardware */
    if (mux_sys_valid(&ops->mux_sys)) {
        mux_sys_t* mux_sys = (mux_sys_t*)&ops->mux_sys;
        if (mux_feature_enable(mux_sys, uart_mux[dev->id])) {
            printf("Failed to initialise MUX for UART %d\n", dev->id);
        }

    } else {
        //    printf("INFO: Skipping MUX initialisation for UART %d\n", dev->id);
    }

    /* TODO: Use correct clock source */
    if (clock_sys_valid(&ops->clock_sys)) {
        clk = clk_get_clock(&dev->ioops.clock_sys, CLK_MASTER);
    } else {
        clk = NULL;
    }

    /* Set character encoding */
    assert(!uart_configure(dev, 115200UL, 8, PARITY_NONE, 1));
    /* Set FIFO trigger levels */
    v = FIFO_EN;
    v |= FIFO_RXLVL(FIFO_RXLVL_VAL);
    v |= FIFO_TXLVL(FIFO_TXLVL_VAL);
    *REG_PTR(dev->vaddr, UFCON) = v;
    /* Configure TX/RX modes and enable TX/RX */
    v = CON_RX_TIMEOUT(RX_TIMEOUT_VAL) | CON_RXTIMEOUT_ENABLE;
    v |= (CON_TXMODE(POLL) | CON_RXMODE(POLL));
    v |= CON_TXIRQTYPE_LEVEL | CON_RXIRQTYPE_LEVEL;
    *REG_PTR(dev->vaddr, UCON) = v;
    /* Enable RX IRQ */
    *REG_PTR(dev->vaddr, UINTM) = ~INT_RX;

    return 0;
}

