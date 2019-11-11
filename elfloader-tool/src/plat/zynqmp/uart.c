/*
 * Copyright 2017, DornerWorks
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_DORNERWORKS_GPL)
 */
/*
 * This data was produced by DornerWorks, Ltd. of Grand Rapids, MI, USA under
 * a DARPA SBIR, Contract Number D16PC00107.
 *
 * Approved for Public Release, Distribution Unlimited.
 */

#include <elfloader_common.h>
#include <platform.h>

/*
 * UART Hardware Constants
 */
#define XUARTPS_CR             0x00
#define XUARTPS_SR             0x2C
#define XUARTPS_FIFO           0x30

#define XUARTPS_SR_TXEMPTY     (1U << 3)

#define XUARTPS_CR_TX_EN       (1U << 4)
#define XUARTPS_CR_TX_DIS      (1U << 5)


#define UART_REG(x) ((volatile uint32_t *)(UART_PPTR + (x)))

int plat_console_putchar(unsigned int c)
{
    /* Wait to be able to transmit. */
    while (!(*UART_REG(XUARTPS_SR) & XUARTPS_SR_TXEMPTY));

    /* Transmit. */
    *UART_REG(XUARTPS_FIFO) = c;

    return 0;
}

void enable_uart()
{
    uint32_t v = *UART_REG(XUARTPS_CR);
    v |= XUARTPS_CR_TX_EN;
    v &= ~XUARTPS_CR_TX_DIS;
    *UART_REG(XUARTPS_CR) = v;
}
