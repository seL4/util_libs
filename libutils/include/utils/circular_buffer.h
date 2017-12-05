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

/**
 * @file circular_buffer.h
 * @brief A circular buffer implementation
 */

#pragma once

#include <stdbool.h>
#include <sys/types.h>
#include <stdint.h>

/* Circular Buffer */
typedef struct circ_buf {
	off_t head;
	off_t tail;
	size_t size;
	uint8_t buf[];
} circ_buf_t;

/**
 * Create a new circular buffer
 *
 * @param size The size of the buffer in bytes.
 * @param[in] cb Circular buffer structure allocated by the user.
 *
 * @return NULL on failure.
 */
int circ_buf_new(size_t size, circ_buf_t *cb);

/**
 * Destroy a circular buffer
 *
 * @param cb Circular buffer to Destroy.
 */
void circ_buf_free(circ_buf_t *cb);

/**
 * Check if the circular buffer is full
 *
 * @param cb Circular buffer to check
 *
 * @return true indicates the buffer is full, false otherwise.
 */
bool circ_buf_is_full(circ_buf_t *cb);

/**
 * Check if the circular buffer is empty
 *
 * @param cb Circular buffer to check
 *
 * @return true indicates the buffer is empty, false otherwise.
 */
bool circ_buf_is_empty(circ_buf_t *cb);

/**
 * Put a byte
 *
 * @param cb Circular buffer to put via.
 * @param c  Byte to send.
 */
void circ_buf_put(circ_buf_t *cb, uint8_t c);

/**
 * Get a byte
 *
 * @param cb Circular buffer to get from.
 *
 * @return The byte received.
 */
uint8_t circ_buf_get(circ_buf_t *cb);

