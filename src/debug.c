/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include "debug.h"

struct ps_chardevice* debug_con = (void*)0;

void register_debug_device(struct ps_chardevice* d)
{
    debug_con = d;
}

static void _printhex_nibble(char c)
{
    c &= 0xf;
    if ( c > 9) {
        ps_putchar(c + 'A' - 10);
    } else {
        ps_putchar(c + '0');
    }
}

int ps_putchar(int c)
{
    if (debug_con) {
        ps_cdev_putchar(debug_con, c);
    }
    return c;
}

int ps_getchar(void)
{
    if (debug_con) {
        return ps_cdev_getchar(debug_con);
    } else {
        return -1;
    }
}

void ps_printhex(uint32_t h)
{
    ps_putchar('0');
    ps_putchar('x');
    int i;
    for (i = 0; i < 8; i++) {
        _printhex_nibble(h >> (28 - (i * 4)));
    }
    ps_putchar('\r');
    ps_putchar('\n');
}


void ps_print(const char* str)
{
    for ( ; *str != '\0' ; str++) {
        ps_putchar(*str);
    }
}
