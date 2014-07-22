/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <platsupport/chardev.h>

void register_debug_device(struct ps_chardevice* d);

void ps_printhex(uint32_t h);
int ps_putchar(int c);
int ps_getchar(void);
void ps_print(const char* str);
