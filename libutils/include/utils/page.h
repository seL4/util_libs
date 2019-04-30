/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

/* macros on page sizes */

#include <stdint.h>

#include <utils/arith.h>

#define SIZE_BITS_TO_BYTES(size_bits) (BIT(size_bits))
#define BYTES_TO_SIZE_BITS(bytes) (LOG_BASE_2(bytes))

#define PAGE_BITS_1G 30
#define PAGE_BITS_4M 22
#define PAGE_BITS_2M 21
#define PAGE_BITS_4K 12
#define PAGE_SIZE_4K (SIZE_BITS_TO_BYTES(PAGE_BITS_4K))
#define PAGE_MASK_4K (PAGE_SIZE_4K - 1)
#define PAGE_ALIGN_4K(addr) ((addr) & ~PAGE_MASK_4K)
#define IS_ALIGNED_4K(addr) IS_ALIGNED(addr, PAGE_BITS_4K)

#define MiB_TO_BYTES(x) (1024 * 1024 * ((size_t)x))

/* convert b bytes to the number of pages of size size_bits required for that many bytes */
#define BYTES_TO_SIZE_BITS_PAGES(b, size_bits) (((b) / (BIT(size_bits))) + ((((b) % (BIT(size_bits))) > 0) ? 1 : 0))
#define BYTES_TO_4K_PAGES(b) BYTES_TO_SIZE_BITS_PAGES(b, PAGE_BITS_4K)

#define PAGE_ALIGN(addr, size) ((addr) & ~(size-1))

#define SAME_PAGE_4K(a, b) \
    ((((uintptr_t)(a)) & ~PAGE_MASK_4K) == (((uintptr_t)(b)) & ~PAGE_MASK_4K))
