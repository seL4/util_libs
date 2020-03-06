/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

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

