/*
 * Copyright 2020, DornerWorks
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DORNERWORKS_BSD)
 */

#pragma once
#include <autoconf.h>

enum chardev_id {
    UART0,
    UART1,
    UART2,
    UART3,
    PS_SERIAL_DEFAULT = UART3
};

/*
 * Depending on whether or not the Divisor Latch Access Bit (DLAB) is set
 * in the line_control (LCR) register (bit 7) the first two registers will
 * have different functions.
 * If DLAB is not set in the LCR:
 *  reg[0]: receive or transmit buffer
 *    If written to: tranmit buffer
 *    If read      : receive buffer
 *  reg[1]: interrupt enable register
 * If the DLAB is set in the LCR:
 *  reg[0]: LSB of the divisor
 *  reg[1]: MSB of the divisor
 * The interrupt id and fifo control registers are related
 *  If the 3rd register is written to, it affects the fifo control
 *  If it is read from, the interrupt id register contents are returned
 */
struct uart {
    union {
        uint32_t rx_buffer;
        uint32_t tx_buffer;
        uint32_t divisor_latch_lsb;
    };
    union {
        uint32_t interrupt_enable;
        uint32_t divisor_latch_msb;
    };
    union {
        uint32_t interrupt_id;
        uint32_t fifo_control;
    };
    uint32_t line_control ;
    uint32_t modem_control ;
    uint32_t line_status;
    uint32_t modem_status;
    uint32_t scratch;
    uint32_t RESERVED ;
    uint32_t multi_mode_interrupt_enable;
    uint32_t multi_mode_interrupt_id;
    uint32_t RESERVED_1 ;
    uint32_t multi_mode_control_0;
    uint32_t multi_mode_control_1;
    uint32_t multi_mode_control_2;
    uint32_t fractional_divisor;
    uint32_t glitch_filter;
    uint32_t transmitter_time_guard;
    uint32_t receiver_time_out;
    uint32_t address;
};
typedef volatile struct uart uart_regs_t;


#define UART0_PADDR 0x20000000
#define UART1_PADDR 0x20100000
#define UART2_PADDR 0x20102000
#define UART3_PADDR 0x20104000

#define UART0_IRQ 90
#define UART1_IRQ 91
#define UART2_IRQ 92
#define UART3_IRQ 93

#define DEFAULT_SERIAL_PADDR UART3_PADDR
#define DEFAULT_SERIAL_INTERRUPT UART3_IRQ

/* Relevant Register Masks *
/
/* Line Control Register */
#define LCR_WORD_LEN_5                  0b00
#define LCR_WORD_LEN_6                  0b01
#define LCR_WORD_LEN_7                  0b10
#define LCR_WORD_LEN_8                  0b11
#define LCR_DIV_LATCH_MASK              (1 << 7)

/* Modem Control Register */
#define MCR_DTR_MASK                    (1 << 0)
#define MCR_RTS_MASK                    (1 << 1)

/* Multi Mode Control Register 0 */
#define MM0_ENABLE_FRAC_MASK            (1 << 7)

/* Line Status Register */
#define LSR_DATA_READY_MASK             (1 << 0)
#define LSR_TX_HOLD_REG_EMPTY_MASK      (1 << 5)
