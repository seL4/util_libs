/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

/*
 * Platform-specific putchar implementation.
 */

#include "../stdint.h"
#include "../stdio.h"
/*
 * Place a character to the given stream, which we always assume to be
 * 'stdout'.
 */
extern int
__fputc(int c, FILE *stream);

#define SPS_UART1_DM_PADDR        0x12450000 /* Daytona_SPS */
#define SPS_UART2_DM_PADDR        0x12490000 /* Daytona_SPS */
#define GSBI3_UART_DM_PADDR       0x16240000 /* GSBIs */
#define GSBI4_UART_DM_PADDR       0x16340000 /* GSBIs */
#define GSBI6_UART_DM_PADDR       0x16540000 /* GSBIs */
#define GSBI7_UART_DM_PADDR       0x16640000 /* GSBIs */
#define GSBI5_UART_DM_PADDR       0x1A240000 /* GSBIs */
#define UART_PADDR  (GSBI7_UART_DM_PADDR)

#define UART_REG(x) ((volatile uint32_t *)(UART_PADDR + (x)))

#define USR                   0x08
#define UTF                   0x70
#define UNTX                  0x40

#define USR_TXRDY             (1U << 2)
#define USR_TXEMP             (1U << 3)


volatile uint32_t* uart = (volatile uint32_t*)UART_PADDR;


int
__fputc(int c, FILE *stream)
{
    (void)stream;
    /* Wait for TX fifo to be empty */
    while ( !(*UART_REG(USR) & USR_TXEMP) );
    /* Tell the peripheral how many characters to send */
    *UART_REG(UNTX) = 1;
    /* Write the character into the FIFO */
    *UART_REG(UTF) = c & 0xff;

    /* Send '\r' after every '\n'. */
    if (c == '\n') {
        (void)__fputc('\r', stream);
    }

    return 0;
}

