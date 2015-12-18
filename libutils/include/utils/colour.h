/*
 * Copyright 2015, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _UTILS_COLOUR_H
#define _UTILS_COLOUR_H

#include <stdint.h>

typedef struct colour_rgb24 {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} colour_rgb24_t;

static inline colour_rgb24_t colour_rgb24_create(uint8_t red, uint8_t green, uint8_t blue) {
    return (colour_rgb24_t) {red, green, blue};
}

static inline colour_rgb24_t colour_rgb24_decode_u32(uint32_t colour) {
    return (colour_rgb24_t) {
        (colour >> 16) & 0xff,
        (colour >> 8) & 0xff,
        colour & 0xff
    };
}

static inline uint32_t colour_rgb24_encode_u32(colour_rgb24_t colour) {
    return (colour.red << 16) | (colour.green << 8) | colour.blue;
}

#endif
