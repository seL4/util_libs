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

#ifndef _TYPES_H
#define _TYPES_H

typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

#if defined(__KERNEL_32__)
typedef uint32_t uintptr_t;
typedef uint32_t size_t;
typedef uint32_t word_t;
#define BYTE_PER_WORD   4
#elif defined(__KERNEL_64__)
typedef uint64_t uintptr_t;
typedef uint64_t size_t;
typedef uint64_t word_t;
#define BYTE_PER_WORD   8
#endif

#define UINT32_MAX    (0xffffffff)
#define UINT64_MAX    (0xffffffffffffffffull)

#endif /* _TYPES_H */
