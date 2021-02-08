/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <utils/arith.h>

/* A streaming base64 encoder */

typedef struct {
    FILE *output;
    uint16_t buffer;
    size_t bits;
} base64_t;

#define BASE64_LOOKUP (\
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ" \
    "abcdefghijklmnopqrstuvwxyz" \
    "0123456789+/" \
)

/* Create a new base64 streamer that streams to the given output */
static inline base64_t base64_new(FILE *output)
{
    return (base64_t) {
        .output = output,
        .buffer = 0,
        .bits = 0,
    };
}

/* Lookup the character for a given bit pattern */
static inline uint8_t base64_lookup(uint8_t bit)
{
    return BASE64_LOOKUP[bit & MASK(6)];
}

/* Write a byte to a base64 stream */
static inline int base64_putbyte(base64_t *streamer, uint8_t byte)
{
    /* Buffer the byte */
    streamer->buffer <<= 8;
    streamer->buffer |= byte;
    streamer->bits += 8;

    /* Write any bits to the output */
    while (streamer->bits >= 6) {
        streamer->bits -= 6;
        uint8_t part = streamer->buffer >> streamer->bits;
        fputc(base64_lookup(part), streamer->output);
    }

    return 0;
}

/* Write any remaining data to the output */
static inline int base64_terminate(base64_t *streamer)
{
    if (streamer->bits > 0) {
        size_t padding = 6 - streamer->bits;
        streamer->buffer <<= padding;
        fputc(base64_lookup(streamer->buffer), streamer->output);
        while (padding > 0) {
            fputc('=', streamer->output);
            padding -= 2;
        }
    }

    /* Reset the streamer */
    streamer->bits = 0;

    return 0;
}
