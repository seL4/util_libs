/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * @file circular_buffer.h
 * @brief A circular buffer implementation
 */

#pragma once

#include <stdbool.h>
#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <utils/zf_log.h>



/* Circular Buffer */
typedef struct circ_buf {
    off_t head;
    off_t tail;
    size_t size;
    uint8_t buf[];
} circ_buf_t;


static inline off_t _next_pos(circ_buf_t *cb, off_t pos)
{
    return (pos + 1) % cb->size;
}


/**
 * Initialise a new circular buffer
 *
 * @param size The size of the buffer in bytes.
 * @param[in] cb Circular buffer structure allocated by the user.
 *
 * @return NULL on failure.
 */
static inline int circ_buf_init(size_t size, circ_buf_t *cb)
{
    if (size == 0 || !cb) {
        ZF_LOGE("Invalid arguments\n");
        return EINVAL;
    }

    cb->head = 0;
    cb->tail = 0;
    cb->size = size;

    return 0;
}

/**
 * Check if the circular buffer is full
 *
 * @param cb Circular buffer to check
 *
 * @return true indicates the buffer is full, false otherwise.
 */
static inline bool circ_buf_is_full(circ_buf_t *cb)
{
    return _next_pos(cb, cb->tail) == cb->head;
}

/**
 * Check if the circular buffer is empty
 *
 * @param cb Circular buffer to check
 *
 * @return true indicates the buffer is empty, false otherwise.
 */
static inline bool circ_buf_is_empty(circ_buf_t *cb)
{
    return cb->tail == cb->head;
}

/**
 * Put a byte
 *
 * @param cb Circular buffer to put via.
 * @param c  Byte to send.
 */
static inline void circ_buf_put(circ_buf_t *cb, uint8_t c)
{
    cb->buf[cb->tail] = c;
    cb->tail = _next_pos(cb, cb->tail);
}

/**
 * Get a byte
 *
 * @param cb Circular buffer to get from.
 *
 * @return The byte received.
 */
static inline uint8_t circ_buf_get(circ_buf_t *cb)
{
    uint8_t c = cb->buf[cb->head];
    cb->head = _next_pos(cb, cb->head);

    return c;
}

