/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <utils/sglib.h>
#include <platsupport/tqueue.h>

static int cmp(uint64_t a, uint64_t b)
{
    if (a > b) {
        return 1;
    } else if (a < b) {
        return -1;
    } else {
        return 0;
    }
}

#define TIMEOUT_CMP(t1, t2) (cmp(t1->timeout.abs_time, t2->timeout.abs_time))

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
SGLIB_DEFINE_SORTED_LIST_PROTOTYPES(tqueue_node_t, TIMEOUT_CMP, next)
SGLIB_DEFINE_SORTED_LIST_FUNCTIONS(tqueue_node_t, TIMEOUT_CMP, next)
#pragma GCC diagnostic pop

static tqueue_node_t *head(tqueue_node_t *list)
{
    struct sglib_tqueue_node_t_iterator it;
    return sglib_tqueue_node_t_it_init(&it, list);
}

int tqueue_alloc_id(tqueue_t *tq, unsigned int *id)
{
    if (!tq || !id) {
        return EINVAL;
    }

    for (int i = 0; i < tq->n; i++) {
        if (!tq->array[i].allocated) {
            tq->array[i].allocated = true;
            *id = i;
            return 0;
        }
    }

    ZF_LOGE("Out of timer client ids\n");
    return ENOMEM;
}

int tqueue_alloc_id_at(tqueue_t *tq, unsigned int id)
{
    if (!tq || id >= tq->n) {
        return  EINVAL;
    }

    if (tq->array[id].allocated) {
        return EADDRINUSE;
    }

    tq->array[id].allocated = true;
    return 0;
}

int tqueue_free_id(tqueue_t *tq, unsigned int id)
{
    if (!tq) {
        return EINVAL;
    }

    if (id < 0 || id >= tq->n) {
        ZF_LOGE("Invalid id");
        return EINVAL;
    }

    if (!tq->array[id].allocated) {
        ZF_LOGW("Freeing unallocated id");
        return EINVAL;
    }

    /* remove from queue */
    if (tq->array[id].active) {
        sglib_tqueue_node_t_delete(&tq->queue, &tq->array[id]);
        tq->array[id].active = false;
    }

    tq->array[id].allocated = false;
    return 0;
}

int tqueue_register(tqueue_t *tq, unsigned int id, timeout_t *timeout)
{
    if (!tq) {
        return EINVAL;
    }

    if (id < 0 || id > tq->n || !tq->array[id].allocated) {
        ZF_LOGE("invalid id");
        return EINVAL;
    }

    /* delete the callback from the queue if its present */
    if (tq->array[id].active) {
        sglib_tqueue_node_t_delete(&tq->queue, &tq->array[id]);
    }

    /* update node */
    tq->array[id].active = true;
    tq->array[id].timeout = *timeout;

    /* add to data structure */
    sglib_tqueue_node_t_add(&tq->queue, &tq->array[id]);
    return 0;
}

int tqueue_cancel(tqueue_t *tq, unsigned int id)
{

    if (!tq) {
        return EINVAL;
    }

    /* iterate through the list until we find that id */
    if (id < 0 || id > tq->n) {
        ZF_LOGE("Invalid id");
        return EINVAL;
    }

    /* delete the callback from the queue if its present */
    if (tq->array[id].active) {
        sglib_tqueue_node_t_delete(&tq->queue, &tq->array[id]);
    }

    tq->array[id].active = false;
    return 0;
}

int tqueue_update(tqueue_t *tq, uint64_t curr_time, uint64_t *next_time)
{
    if (!tq) {
        return EINVAL;
    }

    /* keep checking the head of this queue */
    tqueue_node_t *t = head(tq->queue);
    while (t != NULL && t->timeout.abs_time <= curr_time) {
        if (t->active) {
            t->timeout.callback(t->timeout.token);
        }

        /* check if it is active again, as callback may have deactivated the timeout */
        if (t->active) {
            sglib_tqueue_node_t_delete(&tq->queue, t);
            if (t->timeout.period > 0) {
                t->timeout.abs_time += t->timeout.period;
                sglib_tqueue_node_t_add(&tq->queue, t);
            } else {
                t->active = false;
            }
        }
        t = head(tq->queue);
    }

    if (next_time) {
        if (t) {
            *next_time = t->timeout.abs_time;
        } else {
            *next_time = 0;
        }
    }
    return 0;
}

int tqueue_init_static(tqueue_t *tq, ps_malloc_ops_t *mops, int size)
{
    if (!tq || !mops) {
        return EINVAL;
    }

    if (size <= 0) {
        return EINVAL;
    }

    /* initialise the list */
    tq->n = size;
    int error = ps_calloc(mops, size, sizeof(tqueue_node_t), (void **) &tq->array);
    if (error) {
        return ENOMEM;
    }

    assert(tq->array != NULL);

    /* noone currently in the queue */
    tq->queue = NULL;

    return 0;
}
