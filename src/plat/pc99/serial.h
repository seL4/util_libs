/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef _PLATSUPPORT_PLAT_SERIAL_H
#define _PLATSUPPORT_PLAT_SERIAL_H

#include "../../chardev.h"

#define SERIAL_CONSOLE_COM1_PORT 0x3f8
#define SERIAL_CONSOLE_COM2_PORT 0x2f8
#define SERIAL_CONSOLE_COM3_PORT 0x3e8
#define SERIAL_CONSOLE_COM4_PORT 0x2e8

#define SERIAL_CONSOLE_COM1_IRQ 4
#define SERIAL_CONSOLE_COM2_IRQ 3
#define SERIAL_CONSOLE_COM3_IRQ 4
#define SERIAL_CONSOLE_COM4_IRQ 3

/*
 * Port offsets
 * W    - write
 * R    - read 
 * RW   - read and write
 * DLAB - Alternate register function bit
 */

#define SERIAL_THR  0 /* Transmitter Holding Buffer (W ) DLAB = 0 */
#define SERIAL_RBR  0 /* Receiver Buffer            (R ) DLAB = 0 */ 
#define SERIAL_DLL  0 /* Divisor Latch Low Byte     (RW) DLAB = 1 */
#define SERIAL_IER  1 /* Interrupt Enable Register  (RW) DLAB = 0 */
#define SERIAL_DLH  1 /* Divisor Latch High Byte    (RW) DLAB = 1 */
#define SERIAL_IIR  2 /* Interrupt Identification   (R ) */ 
#define SERIAL_FCR  2 /* FIFO Control Register      (W ) */
#define SERIAL_LCR  3 /* Line Control Register      (RW) */
#define SERIAL_MCR  4 /* Modem Control Register     (RW) */
#define SERIAL_LSR  5 /* Line Status Register       (R ) */
#define SERIAL_MSR  6 /* Modem Status Register      (R ) */
#define SERIAL_SR   7 /* Scratch Register           (RW) */
#define CONSOLE(port, label) ((port) + (SERIAL_##label))
#define SERIAL_DLAB BIT(7)
#define SERIAL_LSR_DATA_READY BIT(0)
#define SERIAL_LSR_TRANSMITTER_EMPTY BIT(5)


struct ps_chardevice*
serial_init(const struct dev_defn* defn, const ps_io_ops_t* ops, struct ps_chardevice* dev);

#endif /* _PLATSUPPORT_PLAT_SERIAL_H */