/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include "string.h"

int strcmp(const char *a, const char *b)
{
    while (1) {
        if (*a != * b) {
            return ((unsigned char) * a) - ((unsigned char) * b);
        }
        if (*a == 0) {
            return 0;
        }
        a++;
        b++;
    }
}

void *memset(void *s, int c, size_t n)
{
    char *mem = (char *)s;

    size_t i;
    for (i = 0; i < n; i++) {
        mem[i] = c;
    }

    return s;
}

void *memcpy(void *dest, void *src, size_t n)
{
    char *d = (char *)dest;
    char *s = (char *)src;

    size_t i;
    for (i = 0; i < n; i++) {
        d[i] = s[i];
    }

    return dest;
}

