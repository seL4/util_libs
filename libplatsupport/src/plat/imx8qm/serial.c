/*
 * Copyright 2017, DornerWorks
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_DORNERWORKS_BSD)
 */

#include <autoconf.h>
#include <stdlib.h>
#include <platsupport/serial.h>
#include <platsupport/plat/serial.h>
#include <string.h>

#include "../../chardev.h"

#define UART_REF_CLK 80000000

typedef volatile struct imx8_uart_regs imx8_uart_regs_t;


static inline imx8_uart_regs_t*
imx8_uart_get_priv(ps_chardevice_t *d)
{
    return (imx8_uart_regs_t*)d->vaddr;
}

int uart_getchar(ps_chardevice_t *d)
{
    imx8_uart_regs_t* regs = imx8_uart_get_priv(d);
    uint32_t reg = 0;
    int c = -1;

    if (regs->stat & LPUART_STAT_RDRF){
        reg = regs->data;
        c = reg & LPUART_DATA_RT8;

    }

    return c;
}

int uart_putchar(ps_chardevice_t* d, int c)
{
    imx8_uart_regs_t* regs = imx8_uart_get_priv(d);
    if (regs->stat & LPUART_STAT_TDRE) {
        if (c == '\n' && (d->flags & SERIAL_AUTO_CR)) {
            uart_putchar(d, '\r');
        }
        regs->data = regs->data | c;
        return c;
    } else {
        return -1;
    }
}

static void
uart_handle_irq(ps_chardevice_t* d UNUSED)
{
    /* TODO */
}


/**
 *  \brief Check if LPUART transmitter is enabled and disable
 *
 *  A number of settings for the i.MX8 LPUART require the
 *  receiver and transmitter to be enabled. Before making those
 *  changes this function can be used to check if the transmitter is
 *  enabled and disable it. It returns a 1 if it was enabled and
 *  0 if it was disabled. This way the imx8_uart_reenable_transmitter()
 *  can be used to put the transmitter to its previous state.
 *
 *  \param regs
 *  \return uint8_t
 */
static uint8_t imx8_uart_check_and_disable_transmitter(imx8_uart_regs_t* regs)
{
    uint8_t tx_e;
    // check the state of the rx and tx enable
    tx_e = (regs->ctrl & LPUART_CTRL_TE) ? 1 : 0;
    if (tx_e){
        regs->ctrl = (regs->ctrl) & ~LPUART_CTRL_TE;
    }
    // return if transmitter was enabled
    return(tx_e);
}

/**
 *  \brief Check if LPUART receiver is enabled and disable
 *
 *  A number of settings for the i.MX8 LPUART require the
 *  receiver and transmitter to be enabled. Before making those
 *  changes this function can be used to check if the receiver is
 *  enabled and disable it. It returns a 1 if it was enabled and
 *  0 if it was disabled. This way the imx8_uart_reenable_receiver()
 *  can be used to put the receiver to its previous state.
 *
 *  \param regs
 *  \return uint8_t
 */
static uint8_t imx8_uart_check_and_disable_receiver(imx8_uart_regs_t* regs)
{
    uint8_t rx_e;
    // check the state of the rx and tx enable
    rx_e = (regs->ctrl & LPUART_CTRL_RE) ? 1 : 0;
    if (rx_e){
        regs->ctrl = (regs->ctrl) & ~LPUART_CTRL_RE;
    }
    // return if receiver was enabled
    return(rx_e);
}

/**
 *  \brief Re-enable i.MX8 LPUART transmitter
 *
 *  To be used in conjuntion with imx8_uart_check_and_disable_transmitter()
 *  to put the UART transmitter in it's previous state
 *
 *  \param regs, prev_state
 *  \return void
 */
static void imx8_uart_reenable_transmitter(imx8_uart_regs_t* regs, uint8_t prev_state)
{
    if (prev_state)
    {
        regs->ctrl = regs->ctrl | LPUART_CTRL_TE;
    }
}

/**
 *  \brief Re-enable i.MX8 LPUART reciever
 *
 *  To be used in conjuntion with imx8_uart_check_and_disable_reciever()
 *  to put the UART reciever in it's previous state
 *
 *  \param regs, prev_state
 *  \return void
 */
static void imx8_uart_reenable_receiver(imx8_uart_regs_t* regs, uint8_t prev_state)
{
    if (prev_state)
    {
        regs->ctrl = regs->ctrl | LPUART_CTRL_RE;
    }

}

/**
 *  \brief Set the baud rate of the i.MX8 LPUART device
 *
 *  Given the ps_chardevice_t and bps, performs the calculation
 *  for the SBR bits of the BAUD register and sets them.
 *  BaudRate = LPUART Module Clock / (SBR[12:0] * OSR+1)
 *  The 13 bits in SBR[12:0] set the modulo divide rate for the baud rate generator. When SBR is 1 - 8191,
 *  the baud rate equals "baud clock / ((OSR+1) × SBR)". The 13-bit baud rate setting [SBR12:SBR0] must
 *  only be updated when the transmitter and receiver are both disabled (LPUART_CTRL[RE] and
 *  LPUART_CTRL[TE] are both 0).
 *
 *  \param d, bps
 *  \return void
 */
static void
imx8_uart_set_baud(ps_chardevice_t* d, long bps)
{
    imx8_uart_regs_t* regs = imx8_uart_get_priv(d);
    uint32_t sbr, osr, osr_val;
    uint8_t rx_e, tx_e;
    // check the state of the rx and tx enable, disable if enabled
    tx_e = imx8_uart_check_and_disable_transmitter(regs);
    rx_e = imx8_uart_check_and_disable_receiver(regs);

    osr_val = ((regs->baud & LPUART_BAUD_OSR_MASK)>>LPUART_BAUD_OSR_OFFSET);
    osr = (osr_val > 0) ? (osr_val - 1) : 0;
    sbr = UART_REF_CLK / (bps * (osr + 1));
    regs->baud = regs->baud | sbr; // oring here because there other other flags in this register
    // re-enable the tx or rx before exiting if they were before callin this function
    imx8_uart_reenable_transmitter(regs, tx_e);
    imx8_uart_reenable_receiver(regs, rx_e);
}

/**
 *  \brief Configure the i.MX8 UART
 *
 *  Sets the bps, character size, parity and stop_bits of the LPUART
 *
 *  \param d, bps, char_size, parity, stop_bits
 *  \return int
 */
int
serial_configure(ps_chardevice_t* d, long bps, int char_size, enum serial_parity parity, int stop_bits)
{
    imx8_uart_regs_t* regs = imx8_uart_get_priv(d);
    uint8_t  te_flag, re_flag;
    uint32_t ctrl;

    // Receiver and Transmitter need to be disabled to change the ctrl M7 bit
    te_flag = imx8_uart_check_and_disable_transmitter(regs);
    re_flag = imx8_uart_check_and_disable_transmitter(regs);
    ctrl = regs->ctrl;
    /* Character size */
    switch (char_size){
    case 7:
        ctrl |= LPUART_CTRL_M7;
        break;
    case 8:
        ctrl &= ~LPUART_CTRL_M7;
        ctrl &= ~LPUART_CTRL_M;
        break;
    case 9:
        ctrl &= ~LPUART_CTRL_M7;
        ctrl |= LPUART_CTRL_M;
        break;
    default:
        return -1;
    }
    /* Stop bits */
    switch (stop_bits){
    case 2:
        regs->baud |= LPUART_BAUD_SBNS;
        break;
    case 1:
        regs->baud &= ~LPUART_BAUD_SBNS;
        break;
    default:
        return -1;
    }

    /* Parity */
    if (parity == PARITY_NONE) {
        ctrl &= ~LPUART_CTRL_PE;
    } else if (parity == PARITY_ODD) {
        /* ODD */
        ctrl |= LPUART_CTRL_PE;
        ctrl |= LPUART_CTRL_PT;
    } else if (parity == PARITY_EVEN) {
        /* Even */
        ctrl |=  LPUART_CTRL_PE;
        ctrl &= ~LPUART_CTRL_PT;
    } else {
        return -1;
    }
    /* Apply the changes */
    regs->ctrl = ctrl;
    /* Now set the board rate */
    imx8_uart_set_baud(d, bps);
    /* Re-enable the transmitter and receiver if they were before */
    imx8_uart_reenable_transmitter(regs, te_flag);
    imx8_uart_reenable_receiver(regs, re_flag);
    return 0;
}

/**
 *  \brief Initialize the i.MX8 LPUART
 *
 *  Initialize the transmitter, receiver, and thier interrupts
 *
 *  \param defn, ops, dev
 *  \return int
 */
int uart_init(const struct dev_defn* defn,
              const ps_io_ops_t* ops,
              ps_chardevice_t* dev)
{
    imx8_uart_regs_t* regs;

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

    regs = imx8_uart_get_priv(dev);

    /* Software reset */
    regs->global |= LPUART_GLOBAL_RST;
    while (!(regs->global & LPUART_GLOBAL_RST));

    /* Line configuration */
    serial_configure(dev, 115200, 8, PARITY_NONE, 1);

    /* Enable the UART */
    regs->ctrl |= LPUART_CTRL_TE | LPUART_CTRL_RE; /* RX/TX enable                      */
    /* Initialise the receiver interrupt.                                             */
    regs->ctrl |= LPUART_CTRL_RIE;                /* Enable recv interrupt.            */

    return 0;
}
