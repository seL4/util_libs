/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#pragma once

#include <stdint.h>
#include <byteswap.h>
#include <endian.h>

typedef union colour_rgb24 {
    struct {
        uint8_t __padding;
        uint8_t red;
        uint8_t green;
        uint8_t blue;
    } as_rgb;
    uint32_t as_u32;
} colour_rgb24_t;

static inline colour_rgb24_t colour_rgb24_create(uint8_t red, uint8_t green, uint8_t blue) {
    return (colour_rgb24_t) { .as_rgb = {
        .__padding = 0,
        .red = red,
        .green = green,
        .blue = blue
    }};
}

static inline colour_rgb24_t colour_rgb24_decode_u32(uint32_t colour) {
#if __BYTE_ORDER == __BIG_ENDIAN
    return (colour_rgb24_t) { .as_u32 = colour };
#else
    return (colour_rgb24_t) { .as_u32 = __bswap_32(colour) };
#endif
}

static inline uint32_t colour_rgb24_encode_u32(colour_rgb24_t colour) {
#if __BYTE_ORDER == __BIG_ENDIAN
    return colour.as_u32;
#else
    return __bswap_32(colour.as_u32);
#endif
}
