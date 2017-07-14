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
#include <utils/fence.h>

#include "../../chardev.h"

#define UART_BYTE_MASK  0xff

#define NV_UART_INPUT_CLOCK_FREQ_HZ (408000000)

#define LCR_DLAB        BIT(7)
#define LCR_SET_BREAK   BIT(6)
#define LCR_SET_PARITY  BIT(5)  /* Force parity to value of BIT(4) */
#define LCR_EVEN        BIT(4)  /* even parity? */
#define LCR_PAR         BIT(3)  /* send parity? */
#define LCR_STOP        BIT(2)  /* transmit 1 (0) or 2 (1) stop bits */
#define LCR_WD_SIZE_5   0
#define LCR_WD_SIZE_6   1
#define LCR_WD_SIZE_7   2
#define LCR_WD_SIZE_8   3

#define LSR_THRE_EMPTY  (BIT(5))
#define LSR_RDR_READY   BIT(0)

#define IER_RHR_ENABLE          BIT(0)
#define IER_THR_EMPTY_ENABLE    BIT(1)
#define IER_RX_FIFO_TIMEDOUT    BIT(4)
#define IER_EO_RECEIVE_DATA     BIT(5)

/* IIR is read-only, FCR is write-only. Both share the same address mapping.
 * IIR is automatically selected when you do a read-access.
 * FCR is automatically selected when you do a write-access
 */
#define FCR_FIFO_ENABLE                BIT(0)
#define FCR_DMA_MODE0                  (0)
#define FCR_DMA_MODE1                  BIT(3)
#define FCR_RX_TRIG_SHIFT              (6)
#define FCR_RX_TRIG_MASK               (0x3)
#define FCR_RX_TRIG_FIFO_GT_1          (0)
#define FCR_RX_TRIG_FIFO_GT_4          (1)
#define FCR_RX_TRIG_FIFO_GT_8          (2)
#define FCR_RX_TRIG_FIFO_GT_16         (3)

#define FCR_TX_TRIG_SHIFT              (4)
#define FCR_TX_TRIG_MASK               (0x3)
#define FCR_TX_TRIG_FIFO_GT_1          (3)
#define FCR_TX_TRIG_FIFO_GT_4          (2)
#define FCR_TX_TRIG_FIFO_GT_8          (1)
#define FCR_TX_TRIG_FIFO_GT_16         (0)

#define FCR_TX_FIFO_CLEAR_SHIFT        (2)
#define FCR_RX_FIFO_CLEAR_SHIFT        (1)

#define IIR_FIFO_MODE_STATUS_SHIFT     (6)
#define IIR_FIFO_MODE_STATUS_MASK      (0x3)

#define IIR_INT_PENDING                 BIT(0)
#define IIR_INT_SOURCE_SHIFT            (0)
#define IIR_INT_SOURCE_MASK             (0xF)

enum iir_int_source {
    IIR_INT_SOURCE_NO_INTERRUPT             = 0x1,
    IIR_INT_SOURCE_DATA_ERROR               = 0x6,
    IIR_INT_SOURCE_RECEIVED_DATA_AVAILABLE  = 0x4,
    IIR_INT_SOURCE_RECEIVED_DATA_TIMEOUT    = 0xC,
    IIR_INT_SOURCE_EO_RECEIVED_DATA         = 0x8,
    IIR_INT_SOURCE_THR_TXRDY                = 0x2,
    IIR_INT_SOURCE_MODEM_STATUS             = 0x0
};

#define EXTRACT_BITS(bf,shift,mask)         (((bf) >> (shift)) & (mask))
#define ENCODE_BITS(bf,shift,mask,v)        (((bf) & ~(((mask) << (shift)))) \
                                                | (((v) & (mask)) << (shift)))
struct tk1_uart_regs {
    uint32_t    thr_dlab;   /* 0x0: tx holding register                     */
    uint32_t    ier_dlab;   /* 0x4: IER and DLH registers                   */
    uint32_t    iir_fcr;    /* 0x8: FIFO control; interrupt identification  */
    uint32_t    lcr;        /* 0xc: line control                            */
    uint32_t    mcr;        /* 0x10: modem control                          */
    uint32_t    lsr;        /* 0x14: line status                            */
    uint32_t    msr;        /* 0x18: modem status                           */
    uint32_t    spr;        /* 0x1c: scratchpad                             */
    uint32_t    csr;        /* 0x20: IrDA pulse coding                      */
    uint32_t    rx_fifo_cfg;/* 0x24:                                        */
    uint32_t    mie;        /* 0x28: modem interrupt enable                 */
    uint32_t    asr;        /* 0x3c: auto sense baud                        */
};
typedef volatile struct tk1_uart_regs tk1_uart_regs_t;

static inline tk1_uart_regs_t*
tk1_uart_get_priv(ps_chardevice_t *d)
{
    return (tk1_uart_regs_t*)d->vaddr;
}

static inline bool
tk1_uart_is_async(ps_chardevice_t *d)
{
    /* NV_UART[ABCD] are polling, while NV_UART[ABCD]_ASYNC are asynchronous
     * versions of the first 4 devices.
     */
    return (d->id >= NV_UARTA_ASYNC);
}

static inline void
tk1_uart_set_thr_irq(tk1_uart_regs_t *regs, bool enable)
{
    uint32_t ier;

    ier = regs->ier_dlab;
    if (enable) {
        ier |= IER_THR_EMPTY_ENABLE;
    } else {
        ier &= ~IER_THR_EMPTY_ENABLE;
    }
    regs->ier_dlab = ier;
}

static inline void
tk1_uart_set_rbr_irq(tk1_uart_regs_t *regs, bool enable)
{
    uint32_t ier;

    ier = regs->ier_dlab;
    if (enable) {
        ier |= IER_RHR_ENABLE;
        ier |= IER_RX_FIFO_TIMEDOUT;
    } else {
        ier &= ~IER_RHR_ENABLE;
        ier &= ~IER_RX_FIFO_TIMEDOUT;
    }
    regs->ier_dlab = ier;
}

int uart_getchar(ps_chardevice_t *d)
{
    tk1_uart_regs_t* regs = tk1_uart_get_priv(d);
    uint32_t reg = 0;
    int c = -1;

    if (regs->lsr & LSR_RDR_READY) {
        reg = regs->thr_dlab;
        c = reg & UART_BYTE_MASK;
    }
    return c;
}

int uart_putchar(ps_chardevice_t* d, int c)
{
    tk1_uart_regs_t* regs = tk1_uart_get_priv(d);
    uint32_t lsr = regs->lsr;

    if (((lsr & LSR_THRE_EMPTY) == LSR_THRE_EMPTY)) {
        if (c == '\n' && (d->flags & SERIAL_AUTO_CR)) {
            uart_putchar(d, '\r');
        }

        regs->thr_dlab = (uint8_t) c;

        return c;
    } else {
        return -1;
    }
}

static void
uart_handle_irq(ps_chardevice_t* d)
{
    tk1_uart_regs_t *regs = tk1_uart_get_priv(d);
    uintptr_t irq_ident_val;

    /* Determine the cause of the IRQ. */
    irq_ident_val = regs->iir_fcr;
    irq_ident_val &= IIR_INT_SOURCE_MASK;

    switch (irq_ident_val) {
    case IIR_INT_SOURCE_NO_INTERRUPT:
        break;
    case IIR_INT_SOURCE_DATA_ERROR:

        ZF_LOGE("Parity, overrun, or framing error.");
        if (d->read_descriptor.data != NULL) {
            struct chardev_xmit_descriptor *rd = &d->read_descriptor;

            if (rd->callback != NULL) {
                rd->callback(d, CHARDEV_STAT_ERROR,
                             0,
                             rd->token);
            }
        }
        break;

    case IIR_INT_SOURCE_RECEIVED_DATA_TIMEOUT:
    case IIR_INT_SOURCE_EO_RECEIVED_DATA:
    case IIR_INT_SOURCE_RECEIVED_DATA_AVAILABLE: {
        uint8_t *client_buff;
        int c;

        struct chardev_xmit_descriptor *rd = &d->read_descriptor;

        if (irq_ident_val == IIR_INT_SOURCE_EO_RECEIVED_DATA) {

            /* Tegra K1 Mobile Processor TRM, section 23.4.2:
             *
             * "EORD (End of Receive Data) Interrupt occurs when the receiver
             * detects that data stops coming in for more than 4 character
             * times. This interrupt is useful for determining that the sending
             * device has completed sending all its data. EORD timeout will not
             * occur if the receiving data stream is stopped because of hardware
             * handshaking."
             *
             * "To clear the EORD timeout interrupt you must DISABLE the EORD
             * interrupt enable (IE_EORD)."
             *
             * But I assume I have to re-enable it too, because otherwise I
             * won't get them anymore.
             */
            ZF_LOGV("Int reason EO received data.");
            regs->ier_dlab &= ~IER_EO_RECEIVE_DATA;
            regs->ier_dlab |= IER_EO_RECEIVE_DATA;
        }

        if (rd->data == NULL || rd->bytes_requested == 0) {
            /* Even if there is no rx buffer, or no bytes have been requested,
             * or some other unusual case has been triggered, we should read the
             * RBR register and consume the bytes in it.
             */
            ZF_LOGW("Draining.");
            while (uart_getchar(d) != -1) {
                /* Just read the bytes out to clear the FIFO */
            }
            break;
        }

        /* So everytime a new RX data IRQ comes in, the buffer cursor gets
         * reset to the beginning of the client-supplied buffer.
         *
         * Therefore the client should try to service IRQs as quickly as possible.
         */

        client_buff = rd->data;

        c = uart_getchar(d);
        while (c != -1) {
            /* Don't overrun the client-supplied buffer */
            if (rd->bytes_transfered >= rd->bytes_requested) {
                ZF_LOGV("Buffer of %dB will be overrun.", rd->bytes_requested);

                /* We use bytes_requested as a flag to indicate to the this IRQ
                 * handler that it shouldn't call the callback again.
                 * If bytes_requested is 0, we won't get here, so this callback
                 * won't be called repeatedly if there's more data than the
                 * caller's buffer can hold.
                 */
                rd->bytes_requested = 0;

                if (rd->callback != NULL) {
                    rd->callback(d, CHARDEV_STAT_INCOMPLETE,
                                 rd->bytes_transfered,
                                 rd->token);
                }
                break;
            }

            client_buff[rd->bytes_transfered] = (char)c;
            rd->bytes_transfered++;
            c = uart_getchar(d);
        }

        /* If the loop exits early because the buffer was overrun, "c" will
         * not be -1, because the UART would have returned a character. We
         * just didn't have any buffer memory remaining.
         *
         * I.e, c will be -1 if the loop exited normally.
         */
        if (c == -1 && rd->bytes_transfered > 0) {
            if (rd->callback != NULL) {
                /* Only send COMPLETE status if we didn't overrun. */
                rd->callback(d, CHARDEV_STAT_COMPLETE,
                             rd->bytes_transfered,
                             rd->token);
            }
        }

        break;
    }

    case IIR_INT_SOURCE_THR_TXRDY:
        ZF_LOGV("Int reason THR ready: %d of %d bytes transferred.",
                d->write_descriptor.bytes_transfered,
                d->write_descriptor.bytes_requested);

        if (d->write_descriptor.data != NULL) {
            struct chardev_xmit_descriptor *wd = &d->write_descriptor;
            uint8_t *client_data;
            int status;

            client_data = wd->data;
            while (wd->bytes_transfered < wd->bytes_requested) {
                status = uart_putchar(d, client_data[wd->bytes_transfered]);
                if (status == -1) {
                    ZF_LOGV("One DMA pass finished: written %d of %dB!",
                           wd->bytes_transfered, wd->bytes_requested);
                    break;
                }

                wd->bytes_transfered++;
            }

            if (wd->bytes_transfered >= wd->bytes_requested) {
                /* Disable the THR_EMPTY IRQ until a new write request is made. */
                tk1_uart_set_thr_irq(regs, false);

                if (wd->callback != NULL) {
                    wd->callback(d, CHARDEV_STAT_COMPLETE, wd->bytes_transfered,
                                 wd->token);
                }
            }
        } else {
            /* If there's no input data buffer to read from, disable the
             * THR_EMPTY IRQ, because it was triggered, for one reason or
             * another.
             */
            tk1_uart_set_thr_irq(regs, false);
        }
        break;

    case IIR_INT_SOURCE_MODEM_STATUS:
        ZF_LOGV("Modem status changed.");
        break;
    default:
        ZF_LOGW("Unknown interrupt reason %d.", irq_ident_val);
    };
}

static ssize_t
tk1_uart_write(ps_chardevice_t* d, const void* vdata,
                           size_t count, chardev_callback_t rcb,
                           void* token)
{
    struct chardev_xmit_descriptor wd = {
        .callback = rcb,
        .token = token,
        .bytes_transfered = 0,
        .bytes_requested = count,
        .data = (void *)vdata
    };

    d->write_descriptor = wd;

    if (count == 0) {
        /* Call the callback immediately */
        if (rcb != NULL) {
            rcb(d, CHARDEV_STAT_COMPLETE, count, token);
        }
        return 0;
    }

    if (!tk1_uart_is_async(d)) {
        /* Write the data out over the line synchronously. */
        for (int i = 0; i < count; i++) {
            while (uart_putchar(d, ((uint8_t *)vdata)[i]) == -1) {
            }

            d->write_descriptor.bytes_transfered++;
        }

        if (rcb != NULL) {
            rcb(d, CHARDEV_STAT_COMPLETE, d->write_descriptor.bytes_transfered,
                token);
        }
    } else {
        /* Else enable the THRE IRQ and return. */
        tk1_uart_set_thr_irq(tk1_uart_get_priv(d), true);
        THREAD_MEMORY_RELEASE();
    }

    return d->write_descriptor.bytes_transfered;
}

static ssize_t
tk1_uart_read(ps_chardevice_t* d, void* vdata,
                          size_t count, chardev_callback_t rcb,
                          void* token)
{
    if (count < 1) {
        count = 0;
    }

    struct chardev_xmit_descriptor rd = {
        .callback = rcb,
        .token = token,
        .bytes_transfered = 0,
        .bytes_requested = count,
        .data = vdata
    };

    d->read_descriptor = rd;

    if (count == 0 && rcb != NULL) {
        ZF_LOGV("read call with 0 count.");
        rcb(d, CHARDEV_STAT_COMPLETE, count, token);
    }

    if (!tk1_uart_is_async(d)) {
        int n_chars_read = 0;
        int c;

        while ((c = uart_getchar(d)) == -1) {
        }

        /* Read the data synchronously. */
        while (c != -1) {
            ((uint8_t *)vdata)[n_chars_read] = c;

            c = uart_getchar(d);
            n_chars_read++;
        }

        d->read_descriptor.bytes_transfered = n_chars_read;

        if (rcb != NULL) {
            rcb(d, CHARDEV_STAT_COMPLETE, d->read_descriptor.bytes_transfered,
                token);
        }
    }

    return d->read_descriptor.bytes_transfered;
}

/** Used for debugging. Concats the dlab hi and lo bytes into a 16-bit int.
 * @param d Pointer to the device whose divisor you want to get.
 * @return 16-bit divisor.
 */
UNUSED static uint16_t
tk1_uart_get_dlab_divisor(ps_chardevice_t *d)
{
    uint16_t ret=0;
    tk1_uart_regs_t* regs = tk1_uart_get_priv(d);

    regs->lcr |= LCR_DLAB;
    THREAD_MEMORY_RELEASE();
    ret = regs->thr_dlab & 0xFF;
    ret |= (regs->ier_dlab & 0xFF) << 8;
    THREAD_MEMORY_RELEASE();
    regs->lcr &= ~LCR_DLAB;

    return ret;
}

static void
tk1_uart_set_dlab_divisor(ps_chardevice_t *d, uint16_t divisor)
{
    tk1_uart_regs_t* regs = tk1_uart_get_priv(d);

    regs->lcr |= LCR_DLAB;
    THREAD_MEMORY_RELEASE();
    regs->thr_dlab = divisor & 0xFF;
    regs->ier_dlab = (divisor >> 8) & 0xFF;
    THREAD_MEMORY_RELEASE();
    regs->lcr &= ~LCR_DLAB;
}

static int
tk1_uart_get_divisor_for(int baud)
{
    int ret;

    switch (baud) {
    case 115200:
    case 57600:
    case 38400:
    case 19200:
    case 9600:
    case 4800:
    case 2400:
    case 1200:
    case 300:
        /* Do nothing; This switch is for input validation. */
        break;
    default:
        ZF_LOGE("TK1-uart: Unsupported baud rate %d.",
                baud);
        return -1;
    }

    /* Both we an u-boot program the UARTs to use PllP_out as their input clock,
     * which is fixed at 408MHz:
     *
     *  TegraK1 TRM, Section 5.22, Table 14:
     *  "pllP_out: This is the PLLP’s output clock which is set to 408 MHz."
     *
     * This 408MHz output, is then channeled into the UART-controllers after
     * being passed through a divider. The divider's default divisor is 17
     * on hardware #RESET, but u-boot sets the divider's divisor to 0, and so
     * the UART controller gets the full 408 MHz as its input.
     *
     * From there, we just calculate a divisor to put into the UART controller's
     * DLAB (divisor latch) registers, based on the caller's requested baud
     * rate, according to the formula:
     *  Divisor = input_clock_freq / (16 * desired_baud)
     *
     * The number "16" comes from the fact that the UART controller takes 16
     * clock phases to generate one bit of output on the line (TK1 TRM, section
     * 34.1.1.)
     */
    ret = NV_UART_INPUT_CLOCK_FREQ_HZ / (16 * baud);
    return ret;
}

int
serial_configure(ps_chardevice_t* d, long bps, int char_size, enum serial_parity parity, int stop_bits)
{
    tk1_uart_regs_t* regs = tk1_uart_get_priv(d);
    int divisor;
    /* line control register */
    uint32_t lcr = 0;

    /* Disable the receive IRQ while changing line configuration. */
    tk1_uart_set_rbr_irq(regs, false);

    switch (char_size) {
        case 5:
            lcr |= LCR_WD_SIZE_5;
            break;
        case 6:
            lcr |= LCR_WD_SIZE_6;
            break;
        case 7:
            lcr |= LCR_WD_SIZE_7;
            break;
        case 8:
            lcr |= LCR_WD_SIZE_8;
            break;
        default:
            return -1;
    }

    switch (parity) {
        case PARITY_NONE:
            break;
        case PARITY_EVEN:
            lcr |= LCR_EVEN | LCR_PAR;
            break;
        case PARITY_ODD:
            lcr |= LCR_PAR;
            break;
/*
 * Uncomment if we ever need fixed values for parity bits
 *
        case PARITY_ONE:
            lcr |= LCR_SET_PARITY | LCR_EVEN;
            break;
        case PARITY_ZERO:
            lcr |= LCR_SET_PARITY;
            break
*/
        default:
            return -1;
    }

    /* one stop bit */
    regs->lcr = lcr;

    divisor = tk1_uart_get_divisor_for(bps);
    if (divisor < 1) {
        /* Unsupported baud rate. */
        return -1;
    }

    tk1_uart_set_dlab_divisor(d, divisor);

    /* Disable hardware flow control for all UARTs other than UARTD,
     * because UARTD actually has an RS232 pinout port.
     */
    if (d->id != NV_UARTD && d->id != NV_UARTD_ASYNC) {
        uint32_t mcr = regs->mcr;

        /* Clear RTS_EN and CTS_EN. */
        mcr &= ~(BIT(6) | BIT(5));
        /* Force RTS and DTR to low (active) */
        mcr |= (BIT(1) | BIT(0));
        regs->mcr = mcr;
    }

    /* Drain the RX buffer when configuring a new set of line options.
     * By implication, the caller should wait for previous transmissions to be
     * completed before reconfiguring.
     */
    while (uart_getchar(d) != -1) {
    }

    if (tk1_uart_is_async(d)) {
        /* Re-enable receive IRQ. */
        tk1_uart_set_rbr_irq(regs, true);
    }

    return 0;
}

/** Initialize a ps_chardevice_t instance.
 *
 * Expects an already valid mapping to the TK1 UART MMIO vaddr range.
 */
int
tk1_uart_init_common(const struct dev_defn *defn, void *const uart_mmio_vaddr,
                     ps_chardevice_t *dev)
{
    volatile void *uart_vaddr = 0;
    tk1_uart_regs_t* regs;
    uint32_t iir_fcr;
    struct chardev_xmit_descriptor cxd_zero = {0};
    ps_io_ops_t ioops_zero = {{0}};

    /* add offsets properly */
    switch (defn->id) {
        case NV_UARTA:
        case NV_UARTA_ASYNC:
            uart_vaddr = uart_mmio_vaddr;
            break;
        case NV_UARTB:
        case NV_UARTB_ASYNC:
            uart_vaddr = uart_mmio_vaddr + UARTB_OFFSET;
            break;
        case NV_UARTC:
        case NV_UARTC_ASYNC:
            uart_vaddr = uart_mmio_vaddr + UARTC_OFFSET;
            break;
        case NV_UARTD:
        case NV_UARTD_ASYNC:
            uart_vaddr = uart_mmio_vaddr + UARTD_OFFSET;
            /* The kernel uses UART-D. Recommend not conflicting with it. */
            break;
        default:
            return -1;
    }

    memset(dev, 0, sizeof(*dev));

    /* Set up all the  device properties. */
    dev->id         = defn->id;
    dev->vaddr      = (void*)uart_vaddr;
    dev->read       = &tk1_uart_read;
    dev->write      = &tk1_uart_write;
    dev->handle_irq = &uart_handle_irq;
    dev->irqs       = defn->irqs;
    dev->ioops      = ioops_zero;
    dev->flags      = SERIAL_AUTO_CR;

    /* Zero out the client state. */
    dev->write_descriptor = cxd_zero;
    dev->read_descriptor = cxd_zero;

    regs = tk1_uart_get_priv(dev);

    /* Disable IRQs. */
    tk1_uart_set_rbr_irq(regs, false);
    tk1_uart_set_thr_irq(regs, false);

    /* Line configuration */
    serial_configure(dev, 115200, 8, PARITY_NONE, 1);

    /* Set FCR[0] to 1 to enable FIFO mode, and enable DMA mode 1 which will
     * generate an interrupt only when the buffer has.
     *
     * There's no point in doing an R-M-W sequence because reading the FCR
     * actually returns the values from the IIR, so you can't actually read FCR.
     */
    iir_fcr = 0
        | FCR_FIFO_ENABLE | FCR_DMA_MODE0
        | ENCODE_BITS(0, FCR_RX_TRIG_SHIFT, FCR_RX_TRIG_MASK, FCR_RX_TRIG_FIFO_GT_16)
        | ENCODE_BITS(0, FCR_TX_TRIG_SHIFT, FCR_TX_TRIG_MASK, FCR_TX_TRIG_FIFO_GT_16)
        | BIT(FCR_TX_FIFO_CLEAR_SHIFT)
        | BIT(FCR_RX_FIFO_CLEAR_SHIFT);

    regs->iir_fcr = iir_fcr;

    /* Read the status bit to ensure the FIFO was enabled. */
    iir_fcr = regs->iir_fcr;
    if (EXTRACT_BITS(iir_fcr,
                     IIR_FIFO_MODE_STATUS_SHIFT,
                     IIR_FIFO_MODE_STATUS_MASK) != 3) {
        ZF_LOGE("FIFO mode wasn't enabled.\n");
        return -1;
    }

    return 0;
}

/** Initializes a ps_chardevice_t.
 *
 * Expects a viable ps_io_ops_t for mapping the registers.
 */
int
uart_init(const struct dev_defn* defn,
              const ps_io_ops_t* ops,
              ps_chardevice_t* dev)
{
    static void *vaddr = 0;
    int ret;

    /* Attempt to map the virtual address, assure this works */
    if (vaddr == 0) {
        vaddr = chardev_map(defn, ops);

        if (vaddr == NULL) {
            return -1;
        }
    }

    ret = tk1_uart_init_common(defn, vaddr, dev);
    dev->ioops = *ops;
    return ret;
}
