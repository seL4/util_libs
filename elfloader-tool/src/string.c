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

#include <strops.h>

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
typedef word_t __attribute__((__may_alias__)) u_alias;
#endif

size_t strlen(const char *str)
{
	const char *s;
	for (s = str; *s; ++s);
	return (s - str);
}


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

int strncmp(const char* s1, const char* s2, size_t n)
{
    word_t i;
    int diff;

    for (i = 0; i < n; i++) {
        diff = ((unsigned char*)s1)[i] - ((unsigned char*)s2)[i];
        if (diff != 0 || s1[i] == '\0') {
            return diff;
        }
    }

    return 0;
}

void *memset(void *s, int c, size_t n)
{
    char *mem = (char *)s;

#ifdef HAS_MAY_ALIAS
    /* fill byte by byte until word aligned */
    for (; (uintptr_t)mem % BYTE_PER_WORD != 0 && n > 0; mem++, n--) {
        *mem = c;
    }
    /* construct word filler */
    u_alias fill = ((u_alias)-1 / 255) * (unsigned char)c;
    /* do as many word writes as we can */
    for (; n > BYTE_PER_WORD - 1; n -= BYTE_PER_WORD, mem += BYTE_PER_WORD) {
        *(u_alias *)mem = fill;
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
    /* copy byte by byte until word aligned */
    for (; (uintptr_t)d % BYTE_PER_WORD != 0 && n > 0; d++, s++, n--) {
        *d = *s;
    }
    /* copy word by word as long as we can */
    for (; n > BYTE_PER_WORD - 1; n -= BYTE_PER_WORD, s += BYTE_PER_WORD, d += BYTE_PER_WORD) {
        *(u_alias *)d = *(const u_alias *)s;
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
