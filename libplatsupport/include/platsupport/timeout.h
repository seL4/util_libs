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
#include <stdbool.h>
#include <stdlib.h>
#include <platsupport/io.h>

/* Multiplex absolute timeouts -> maps ids of a timeout to absolute timeouts and returns the
 * next timeout due */

/* function to call when a timeout comes in */
typedef int (*timeout_cb_fn_t)(uintptr_t token);

typedef struct {
    /* absolute time this timeout should occur */
    uint64_t abs_time;
    /* period this timeout should reoccur (0 if not periodic) */
    uint64_t period;
    /* token to call callback with */
    uintptr_t token;
    /* callback to call */
    timeout_cb_fn_t callback;
} timeout_t;

struct timeout_node {
    /* details of the timeout */
    timeout_t timeout;
    /* is this timeout allocated? */
    bool allocated;
    /* is this timeout in the callback queue? */
    bool active;
    /* unique id of this timeout (for tie-breaking during sorting) */
    int id;
    /* next ptr for queue */
    struct timeout_node *next;
};
typedef struct timeout_node timeout_node_t;

typedef struct {
    /* head of ordered list of timeouts */
    timeout_node_t *queue;
    /* id indexed array of timeouts */
    timeout_node_t *array;
    /* size of timeout array */
    int n;
} timeout_mplex_t;

/*
 * Alloc a new id for registering timeouts with timeout_register.
 *
 * @param id    pointer to store allocated id in.
 * @return      ENOMEM if there are no free ids, EINVAL if tm or id are NULL, 0 on success.
 */
int timeout_alloc_id(timeout_mplex_t *tm, int *id);

/*
 * Free an id. This id can no longer be used to register timeouts with, and
 * can be reallocated by timeout_alloc_id.
 *
 * @param id    an id allocated by timeout_alloc_id
 * @return      EINVAL if tm is NULL or id is invalid (not allocated by timeout_alloc_id), 0 on sucess.
 */
int timeout_free_id(timeout_mplex_t *tm, int id);

/*
 * Register a timeout. The timeout callback will be called when timeout_update is called and
 * the abs_time in the timeout has passed. If the timeout has already passed, the callback will be called when
 * timeout_update is called.
 *
 * This function can be called from callbacks.
 *
 * @param id        id to register timeout with. If the id already has a callback registered, override it.
 * @param timeout   populated details of the timeout.
 * @return          EINVAL if id or tm are invalid, 0 on success.
 *
 */
int timeout_register(timeout_mplex_t *tm, int id, timeout_t *timeout);

/*
 * Cancel a timeout. The id is still valid, but the callback will not be called.
 *
 * @param id    id to cancel timeout for.
 * @return      EINVAL if id or tm are invalid, 0 on success.
 */
int timeout_cancel(timeout_mplex_t *tm, int id);

/*
 * Call any callbacks where abs_time is >= curr_time. Return the next timeout due in next_time. Reenqueue
 * any periodic callbacks.
 *
 * @param curr_time         the time to check abs_time against for all timeouts.
 * @param[out] next_time    field to populate with next lowest time to be set after all callbacks called.
 *                          If NULL, ignore.
 * @return                  EINVAL if tm is invalid, 0 on sucess.
 *
 */
int timeout_update(timeout_mplex_t *tm, uint64_t curr_time, uint64_t *next_time);

/*
 * Get the smallest registered timeout.
 *
 * @param[out] next_time    field to populate with next lowest time to be set.
 * @return                  EINVAL if tm or next_time is NULL, 0 on success.
 */
int timeout_next(timeout_mplex_t *tm, uint64_t *next_time);

/*
 * Initialise a statically sized timeout multiplexer.
 *
 * @param[out] tm   pointer to memory to use to initialise timout mutiplexer.
 * @param mops      malloc ops to allocate timeout nodes with.
 * @param size      maximum number of ids that can be registered.
 * @return          0 on success.
 */
int timeout_init_static(timeout_mplex_t *tm, ps_malloc_ops_t *mops, int size);
