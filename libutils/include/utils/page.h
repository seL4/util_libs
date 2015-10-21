/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef _UTILS_PAGE_H
#define _UTILS_PAGE_H
/* macros on page sizes */

#include <assert.h>

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <utils/arch/mapping.h>
#include <utils/arith.h>

#define SIZE_BITS_TO_BYTES(size_bits) (BIT(size_bits))
#define BYTES_TO_SIZE_BITS(bytes) (LOG_BASE_2(bytes))

#define PAGE_BITS_4K 12
#define PAGE_SIZE_4K (SIZE_BITS_TO_BYTES(PAGE_BITS_4K))
#define PAGE_MASK_4K (PAGE_SIZE_4K - 1)
#define PAGE_ALIGN_4K(addr) ((addr) & ~PAGE_MASK_4K)
#define IS_ALIGNED_4K(addr) IS_ALIGNED(addr, PAGE_BITS_4K)

/* convert b bytes to the number of pages of size size_bits required for that many bytes */
#define BYTES_TO_SIZE_BITS_PAGES(b, size_bits) (((b) / (BIT(size_bits))) + ((((b) % (BIT(size_bits))) > 0) ? 1 : 0))
#define BYTES_TO_4K_PAGES(b) BYTES_TO_SIZE_BITS_PAGES(b, PAGE_BITS_4K)

#define PAGE_ALIGN(addr, size) ((addr) & ~(size-1))

#define SAME_PAGE_4K(a, b) \
    ((((uintptr_t)(a)) & ~PAGE_MASK_4K) == (((uintptr_t)(b)) & ~PAGE_MASK_4K))

#define UTILS_NUM_PAGE_SIZES ((int) ARRAY_SIZE(utils_page_sizes))

static inline bool
utils_valid_size_bits(size_t size_bits) 
{
    for (int i = 0; i < UTILS_NUM_PAGE_SIZES && size_bits >= utils_page_sizes[i]; i++) {
        /* sanity check, utils_page_sizes should be ordered */
        if (i > 0) {
            assert(utils_page_sizes[i] < utils_page_sizes[i+1]);
        }
        if (size_bits == utils_page_sizes[i]) {
            return true;
        }
    }

    return false;
}

#endif /* _UTILS_PAGE_H */
