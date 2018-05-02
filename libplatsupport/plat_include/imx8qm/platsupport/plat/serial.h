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

#pragma once

#include <autoconf.h>
#include <platsupport/mach/serial.h>

#define UART1_PADDR  0x5A060000
#define UART2_PADDR  0x5A070000
#define UART3_PADDR  0x5A080000
#define UART4_PADDR  0x5A090000
#define UART5_PADDR  0x5A0A0000

#define UART1_IRQ    377
#define UART2_IRQ    378
#define UART3_IRQ    379
#define UART4_IRQ    380
#define UART5_IRQ    381

#define DEFAULT_SERIAL_PADDR        UART2_PADDR
#define DEFAULT_SERIAL_INTERRUPT    UART2_IRQ

#define LPUART_VERID    0x00
#define LPUART_PARAM    0x04
#define LPUART_GLOBAL   0x08
#define LPUART_PINCFG   0x0C
#define LPUART_BAUD     0x10
#define LPUART_STAT     0x14
#define LPUART_CTRL     0x18
#define LPUART_DATA     0x1C
#define LPUART_MATCH    0x20
#define LPUART_FIFO     0x28
#define LPUART_WATER    0x2C

struct imx8_uart_regs {
    uint32_t verid;    /* 0x000 Version ID */
    uint32_t param;    /* 0x004 Parameter Register */
    uint32_t global;   /* 0x008 Global Register */
    uint32_t pincfg;   /* 0x00C Pin Configuration Register */
    uint32_t baud;     /* 0x010 Baud Rate Register */
    uint32_t stat;     /* 0x014 Status Register */
    uint32_t ctrl;     /* 0x018 Control Register */
    uint32_t data;     /* 0x01C Data Register */
    uint32_t match;    /* Match Address Register */
    uint32_t fifo;     /* FIFO Register */
    uint32_t water;    /* Watermark Register */
};

#define LPUART_VERID_FEATURE_MASK  (0xFFFF * BIT(0))
#define LPUART_VERID_MINOR_MASK    (0xFF   * BIT(16))
#define LPUART_VERID_MAJOR_MASK    (0xFF   * BIT(24))

#define LPUART_PARAM_RXFIFO_MASK   (0xFF   * BIT(0))
#define LPUART_PARAM_TXFIFO_MASK   (0xFF   * BIT(8))

#define LPUART_GLOBAL_RST          BIT(1)

#define LPUART_PINCFG_TRGSEL_MASK  (0x03   * BIT(0))

#define LPUART_BAUD_MAEN1          BIT(31)
#define LPUART_BAUD_MAEN2          BIT(30)
#define LPUART_BAUD_M10            BIT(29)
#define LPUART_BAUD_OSR_OFFSET     24
#define LPUART_BAUD_OSR_MASK       (0x1F   * BIT(LPUART_BAUD_OSR_OFFSET))
#define LPUART_BAUD_TDMAE          BIT(23)
#define LPUART_BAUD_RDMAE          BIT(21)
#define LPUART_BAUD_RIDMAE         BIT(20)
#define LPUART_BAUD_MATCFG_MASK    (0x3 * BIT(18))
#define LPUART_BAUD_BOTHEDGE       BIT(17)
#define LPUART_BAUD_RESYNCDIS      BIT(16)
#define LPUART_BAUD_LBKDIE         BIT(15)
#define LPUART_BAUD_RXEDGIE        BIT(14)
#define LPUART_BAUD_SBNS           BIT(13)
#define LPUART_BAUD_SBR_MASK       (0x1FFFFF * BIT(0))

#define LPUART_STAT_LBKDIF               BIT(31)
#define LPUART_STAT_RXEDGIF              BIT(30)
#define LPUART_STAT_MSBF                 BIT(29)
#define LPUART_STAT_RXINV                BIT(28)
#define LPUART_STAT_RWUID                BIT(27)
#define LPUART_STAT_BRK13                BIT(26)
#define LPUART_STAT_LBKDE                BIT(25)
#define LPUART_STAT_RAF                  BIT(24)
#define LPUART_STAT_TDRE                 BIT(23)
#define LPUART_STAT_TC                   BIT(22)
#define LPUART_STAT_RDRF                 BIT(21)
#define LPUART_STAT_IDLE                 BIT(20)
#define LPUART_STAT_OR                   BIT(19)
#define LPUART_STAT_NF                   BIT(18)
#define LPUART_STAT_FE                   BIT(17)
#define LPUART_STAT_PF                   BIT(16)
#define LPUART_STAT_MA1F                 BIT(15)
#define LPUART_STAT_MA2F                 BIT(14)

#define LPUART_CTRL_R8T9                BIT(31)
#define LPUART_CTRL_R9T8                BIT(30)
#define LPUART_CTRL_TXDIR               BIT(29)
#define LPUART_CTRL_TXINV               BIT(28)
#define LPUART_CTRL_ORIE                BIT(27)
#define LPUART_CTRL_NEIE                BIT(26)
#define LPUART_CTRL_FEIE                BIT(25)
#define LPUART_CTRL_PEIE                BIT(24)
#define LPUART_CTRL_TIE                 BIT(23)
#define LPUART_CTRL_TCIE                BIT(22)
#define LPUART_CTRL_RIE                 BIT(21)
#define LPUART_CTRL_ILIE                BIT(20)
#define LPUART_CTRL_TE                  BIT(19)
#define LPUART_CTRL_RE                  BIT(18)
#define LPUART_CTRL_RWU                 BIT(17)
#define LPUART_CTRL_SBK                 BIT(16)
#define LPUART_CTRL_MA1IE               BIT(15)
#define LPUART_CTRL_MA2IE               BIT(14)
#define LPUART_CTRL_M7                  BIT(11)
#define LPUART_CTRL_IDLECFG_MASK        (7 * BIT(8))
#define LPUART_CTRL_LOOPS               BIT(7)
#define LPUART_CTRL_DOZEEN              BIT(6)
#define LPUART_CTRL_RSRC                BIT(5)
#define LPUART_CTRL_M                   BIT(4)
#define LPUART_CTRL_WAKE                BIT(3)
#define LPUART_CTRL_ILT                 BIT(2)
#define LPUART_CTRL_PE                  BIT(1)
#define LPUART_CTRL_PT                  BIT(0)

#define LPUART_DATA_NOISY       BIT(15)
#define LPUART_DATA_PARITYE     BIT(14)
#define LPUART_DATA_FRETSC      BIT(13)
#define LPUART_DATA_RXEMPT      BIT(12)
#define LPUART_DATA_IDLINE      BIT(11)
#define LPUART_DATA_R9T9        BIT(9)
#define LPUART_DATA_R8T8        BIT(8)
#define LPUART_DATA_R7T7        BIT(7)
#define LPUART_DATA_R6T6        BIT(6)
#define LPUART_DATA_R5T5        BIT(5)
#define LPUART_DATA_R4T4        BIT(4)
#define LPUART_DATA_R3T3        BIT(3)
#define LPUART_DATA_R2T2        BIT(2)
#define LPUART_DATA_R1T1        BIT(1)
#define LPUART_DATA_R0T0        BIT(0)
#define LPUART_DATA_RT8         (0xFF  * BIT(0))
#define LPUART_DATA_RT9         (0x1FF * BIT(0))


#define LPUART_MATCH_MA2_MASK (0x3FF * BIT(16))
#define LPUART_MATCH_MA1_MASK (0x3FF * BIT(0))

#define FIFO_TXEMPT             BIT(23)
#define FIFO_RXEMPT             BIT(22)
#define FIFO_TXOF               BIT(17)
#define FIFO_RXUF               BIT(16)
#define FIFO_TXFLUSH            BIT(15)
#define FIFO_RXFLUSH            BIT(14)
#define FIFO_RXIDEN_MASK        (0x7 * BIT(10))
#define FIFO_TXOFE              BIT(9)
#define FIFO_RXUFE              BIT(8)
#define FIFO_TXFE               BIT(7)
#define FIFO_TXFIFOSIZE_MASK    (0x7 * BIT(4))
#define FIFO_RXFE               BIT(3)
#define FIFO_RXFIFOSIZE_MASK    (0x7 * BIT(0))

#define WATERMARK_RXCOUNT_MASK (0x7F * BIT(24))
#define WATERMARK_RXWATER_MASK (0x3F * BIT(16))
#define WATERMARK_TXCOUNT_MASK (0x7F * BIT(8))
#define WATERMARK_TXWATER_MASK (0x3F * BIT(0))

#define WATERMARK_SET_RXCOUNT(x) (x << 24) & WATERMARK_RXCOUNT_MASK
#define WATERMARK_SET_RXWATER(x) (x << 16) & WATERMARK_RXWATER_MASK
