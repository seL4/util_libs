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

/* Both memset and memcpy need a custom type that allows us to use a word
 * that has the aliasing properties of a char.
 */
#ifdef __GNUC__
  #define HAS_MAY_ALIAS
#elif defined(__clang__)
  #if __has_attribute(may_alias)
    #define HAS_MAY_ALIAS
  #endif
#endif

#ifdef HAS_MAY_ALIAS
typedef uint32_t __attribute__((__may_alias__)) u32_alias;
#endif

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

#ifdef HAS_MAY_ALIAS
    /* fill byte by byte until 32-bit aligned */
    for (; (uintptr_t)mem % 4 != 0 && n > 0; mem++, n--) {
        *mem = c;
    }
    /* construct 32-bit filler */
    u32_alias fill = ((u32_alias)-1 / 255) * (unsigned char)c;
    /* do as many word writes as we can */
    for (; n > 3; n-=4, mem+=4) {
        *(u32_alias*)mem = fill;
    }
    /* fill byte by byte for any remainder */
    for (; n > 0; n--, mem++) {
        *mem = c;
    }
#else
    /* Without the __may__alias__ attribute we cannot safely do word writes
     * so fallback to bytes */
    size_t i;
    for (i = 0; i < n; i++) {
        mem[i] = c;
    }
#endif

    return s;
}

void *memcpy(void *restrict dest, const void *restrict src, size_t n)
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

#ifdef HAS_MAY_ALIAS
    /* copy byte by byte until 32-bit aligned */
    for (; (uintptr_t)d % 4 != 0 && n > 0; d++, s++, n--) {
        *d = *s;
    }
    /* copy word by word as long as we can */
    for (; n > 3; n-=4, s+=4, d+=4) {
        *(u32_alias*)d = *(const u32_alias*)s;
    }
    /* copy any remainder byte by byte */
    for (; n > 0; d++, s++, n--) {
        *d = *s;
    }
#else
    size_t i;
    for (i = 0; i < n; i++) {
        d[i] = s[i];
    }
#endif

    return dest;
}

