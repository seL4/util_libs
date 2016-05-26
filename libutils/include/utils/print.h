/*
 * Copyright 2015, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#pragma once

#include <utils/attribute.h>

#if (defined(__clang__) && __has_feature(c_generic_selections)) || \
    (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)) && __STDC_VERSION__ >= 201112L)

    /* On a fully-compliant C11 compiler, we provide a generic mechanism for printing a variable.
     *
     * Example usage:
     *
     *     int i = 42;
     *     printf(FORMAT_STRING(i), i);
     *
     * Some known gotchas:
     *  - Character literals are ints, so you will need to cast them to get sensible results:
     *      FORMAT_STRING((char)'a')
     *  - String literals are character arrays, so you will need to also cast them:
     *      FORMAT_STRING((char*)"hello world")
     */

    #define FORMAT_STRING(x) _Generic((x), \
        char:               "%c",   \
        signed char:        "%hhd", \
        unsigned char:      "%hhu", \
        short:              "%hd",  \
        unsigned short:     "%hu",  \
        int:                "%d",   \
        unsigned int:       "%u",   \
        long:               "%ld",  \
        unsigned long:      "%lu",  \
        long long:          "%lld", \
        unsigned long long: "%llu", \
        double:             "%f",   \
        long double:        "%Lf",  \
        float:              "%f",   \
        char*:              "%s",   \
        void*:              "%p",   \
                                    \
        /* The remaining cases are not necessary on GCC, where lvalue conversion is performed, */ \
        /* but are needed for Clang, which does not do this conversion.                        */ \
                                    \
        const char:               "%c",   \
        const signed char:        "%hhd", \
        const unsigned char:      "%hhu", \
        const short:              "%hd",  \
        const unsigned short:     "%hu",  \
        const int:                "%d",   \
        const unsigned int:       "%u",   \
        const long:               "%ld",  \
        const unsigned long:      "%lu",  \
        const long long:          "%lld", \
        const unsigned long long: "%llu", \
        const double:             "%f",   \
        const long double:        "%Lf",  \
        const float:              "%f",   \
        const char*:              "%s",   \
        const void*:              "%p",   \
        volatile char:               "%c",   \
        volatile signed char:        "%hhd", \
        volatile unsigned char:      "%hhu", \
        volatile short:              "%hd",  \
        volatile unsigned short:     "%hu",  \
        volatile int:                "%d",   \
        volatile unsigned int:       "%u",   \
        volatile long:               "%ld",  \
        volatile unsigned long:      "%lu",  \
        volatile long long:          "%lld", \
        volatile unsigned long long: "%llu", \
        volatile double:             "%f",   \
        volatile long double:        "%Lf",  \
        volatile float:              "%f",   \
        volatile char*:              "%s",   \
        volatile void*:              "%p"    \
                                             \
        /* We assume that no one needs to print a const- *and* volatile-qualified variable. */ \
                                             )

#else

    /* In an unsupported compiler configuration, trigger a compile-time error if the user has
     * attempted to use FORMAT_STRING.
     */

    extern const char *FORMAT_STRING(void *dummy, ...)
        ERROR("FORMAT_STRING requires full C11 compiler support");

#endif
