/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */

#pragma once

#define NULL ((void *)0)
#define FILE void

/* Architecture-specific putchar implementation. */
int __fputc(int c, FILE *data);

int printf(const char *format, ...);
int sprintf(char *buff, const char *format, ...);

