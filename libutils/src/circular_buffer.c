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

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <utils/zf_log.h>
#include <utils/circular_buffer.h>

static inline off_t _next_pos(circ_buf_t *cb, off_t pos)
{
	return (pos + 1) % cb->size;
}

int circ_buf_new(void *base, size_t size, circ_buf_t *cb)
{
	if (!base || size == 0 || !cb) {
		ZF_LOGE("Invalid arguments\n");
		return EINVAL;
	}

	cb->buf = (uint8_t*)base;
	cb->head = 0;
	cb->tail = 0;
	cb->size = size;

	return 0;
}

void circ_buf_free(circ_buf_t *cb)
{
	if (cb) {
		cb->buf = NULL;
		cb->head = 0;
		cb->tail = 0;
		cb->size = 0;
	} else {
		ZF_LOGW("Freeing NULL pointer\n");
	}
}

bool circ_buf_is_full(circ_buf_t *cb)
{
	return _next_pos(cb, cb->tail) == cb->head;
}

bool circ_buf_is_empty(circ_buf_t *cb)
{
	return cb->tail == cb->head;
}

void circ_buf_put(circ_buf_t *cb, uint8_t c)
{
	cb->buf[cb->tail] = c;
	cb->tail = _next_pos(cb, cb->tail);
}

uint8_t circ_buf_get(circ_buf_t *cb)
{
	uint8_t c = cb->buf[cb->head];
	cb->head = _next_pos(cb, cb->head);

	return c;
}

