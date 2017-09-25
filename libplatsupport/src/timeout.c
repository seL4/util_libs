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

#include <utils/sglib.h>
#include <platsupport/timeout.h>

#define TIMEOUT_CMP(t1, t2) (10 * (t1->timeout.abs_time - t2->timeout.abs_time) + (t1->id - t2->id))

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
SGLIB_DEFINE_SORTED_LIST_PROTOTYPES(timeout_node_t, TIMEOUT_CMP, next)
SGLIB_DEFINE_SORTED_LIST_FUNCTIONS(timeout_node_t, TIMEOUT_CMP, next)
#pragma GCC diagnostic pop

static timeout_node_t *head(timeout_node_t *list)
{
    struct sglib_timeout_node_t_iterator it;
    return sglib_timeout_node_t_it_init(&it, list);
}

int timeout_next(timeout_mplex_t *mplex, uint64_t *next) {
    if (!mplex || !next) {
        return EINVAL;
    }
    /* can't be NULL, we just put something in the list */
    timeout_node_t *head_node = head(mplex->queue);
    *next = head_node == NULL ? 0 : head_node->timeout.abs_time;
    return 0;
}

int timeout_alloc_id(timeout_mplex_t *mplex, int *id)
{
    if (!mplex || !id) {
        return EINVAL;
    }

    for (int i = 0; i < mplex->n; i++) {
        if (!mplex->array[i].allocated) {
            mplex->array[i].allocated = true;
            mplex->array[i].id = i;
            *id = i;
            return 0;
        }
    }

    ZF_LOGE("Out of timer client ids\n");
    return ENOMEM;
}

int timeout_free_id(timeout_mplex_t *mplex, int id)
{
    if (!mplex) {
        return EINVAL;
    }

    if (id < 0 || id >= mplex->n) {
        ZF_LOGE("Invalid id");
        return EINVAL;
    }

    if (!mplex->array[id].allocated) {
        ZF_LOGW("Freeing unallocated id");
        return EINVAL;
    }

    /* remove from queue */
    if (mplex->array[id].active) {
        sglib_timeout_node_t_delete(&mplex->queue, &mplex->array[id]);
        mplex->array[id].active = false;
    }

    mplex->array[id].allocated = false;
    return 0;
}

int timeout_register(timeout_mplex_t *mplex, int id, timeout_t *timeout)
{
    if (!mplex) {
        return EINVAL;
    }

    if (id < 0 || id > mplex->n || !mplex->array[id].allocated) {
        ZF_LOGE("invalid id");
        return EINVAL;
    }

    /* delete the callback from the queue if its present */
    if (mplex->array[id].active) {
        sglib_timeout_node_t_delete(&mplex->queue, &mplex->array[id]);
    }

    /* update node */
    mplex->array[id].active = true;
    mplex->array[id].timeout = *timeout;

    /* add to data structure */
    sglib_timeout_node_t_add(&mplex->queue, &mplex->array[id]);
    return 0;
}

int timeout_cancel(timeout_mplex_t *mplex, int id) {

    if (!mplex) {
        return EINVAL;
    }

    /* iterate through the list until we find that id */
    if (id < 0 || id > mplex->n) {
        ZF_LOGE("Invalid id");
        return EINVAL;
    }

    /* delete the callback from the queue if its present */
    if (mplex->array[id].active) {
        sglib_timeout_node_t_delete(&mplex->queue, &mplex->array[id]);
    }

    mplex->array[id].active = false;
    return 0;
}

int timeout_update(timeout_mplex_t *mplex, uint64_t curr_time, uint64_t *next_time) {
    if (!mplex) {
        return EINVAL;
    }

    /* keep checking the head of this queue */
    timeout_node_t *t = head(mplex->queue);
    while (t != NULL && t->timeout.abs_time <= curr_time) {
        if (t->active) {
            t->timeout.callback(t->timeout.token);
        }

        /* check if it is active again, as callback may have deactivated the timeout */
        if (t->active) {
            sglib_timeout_node_t_delete(&mplex->queue, t);
            if (t->timeout.period > 0) {
                t->timeout.abs_time += t->timeout.period;
                sglib_timeout_node_t_add(&mplex->queue, t);
            } else {
                t->active = false;
            }
        }
        t = head(mplex->queue);
    }

    if (next_time) {
        return timeout_next(mplex, next_time);
    }
    return 0;
}

int timeout_init_static(timeout_mplex_t *mplex, ps_malloc_ops_t *mops, int size)
{
    if (!mplex || !mops) {
        return EINVAL;
    }

    if (size <= 0) {
        return EINVAL;
    }

    /* initialise the list */
    mplex->n = size;
    int error = ps_calloc(mops, size, sizeof(timeout_node_t), (void **) &mplex->array);
    if (error) {
        return ENOMEM;
    }

    assert(mplex->array != NULL);

    /* noone currently in the queue */
    mplex->queue = NULL;

    return 0;
}
