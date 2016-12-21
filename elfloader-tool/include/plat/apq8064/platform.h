/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#define SPS_UART1_DM_PADDR        0x12450000 /* Daytona_SPS */
#define SPS_UART2_DM_PADDR        0x12490000 /* Daytona_SPS */
#define GSBI3_UART_DM_PADDR       0x16240000 /* GSBIs */
#define GSBI4_UART_DM_PADDR       0x16340000 /* GSBIs */
#define GSBI6_UART_DM_PADDR       0x16540000 /* GSBIs */
#define GSBI7_UART_DM_PADDR       0x16640000 /* GSBIs */
#define GSBI5_UART_DM_PADDR       0x1A240000 /* GSBIs */
#define UART_PPTR                 (GSBI7_UART_DM_PADDR)

#endif /* _PLATFORM_H_ */

