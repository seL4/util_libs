/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/*
 * Contains definitions for all character devices on this
 * platform
 */

#include "../../chardev.h"
#include "../../common.h"

#include "vga.h"
#include "keyboard_chardev.h"
#include <utils/arith.h>

static const int com1_irqs[] = {SERIAL_CONSOLE_COM1_IRQ, -1};
static const int com2_irqs[] = {SERIAL_CONSOLE_COM2_IRQ, -1};
static const int com3_irqs[] = {SERIAL_CONSOLE_COM3_IRQ, -1};
static const int com4_irqs[] = {SERIAL_CONSOLE_COM4_IRQ, -1};

static const int vga_irqs[] = { -1};

#define PC99_SERIAL_DEFN(devid) {          \
    .id      = PC99_SERIAL_COM##devid,    \
    .paddr   = SERIAL_CONSOLE_COM##devid##_PORT, \
    .size    = 0,             \
    .irqs    = com##devid##_irqs,  \
    .init_fn = &uart_init           \
}

#define PC99_TEXT_VGA_DEFN() {      \
        .id = PC99_TEXT_VGA,        \
        .paddr = VGA_TEXT_FB_BASE,  \
        .size = BIT(12),            \
        .irqs = vga_irqs,           \
        .init_fn = text_vga_init    \
    }

static const int keyboard_irqs[] = {KEYBOARD_PS2_IRQ, -1};

#define PC99_KEYBOARD_DEFN() {        \
    .id      = PC99_KEYBOARD_PS2,     \
    .paddr   = 0,                     \
    .size    = 0,                     \
    .irqs    = keyboard_irqs,         \
    .init_fn = &keyboard_cdev_init    \
}

static const struct dev_defn dev_defn[] = {
    PC99_SERIAL_DEFN(1),
    PC99_SERIAL_DEFN(2),
    PC99_SERIAL_DEFN(3),
    PC99_SERIAL_DEFN(4),
    PC99_TEXT_VGA_DEFN(),
    PC99_KEYBOARD_DEFN()
};

struct ps_chardevice*
ps_cdev_init(enum chardev_id id, const ps_io_ops_t* io_ops, struct ps_chardevice* dev) {
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(dev_defn); i++) {
        if (dev_defn[i].id == id) {
            return (dev_defn[i].init_fn(dev_defn + i, io_ops, dev)) ? NULL : dev;
        }
    }
    return NULL;
}
