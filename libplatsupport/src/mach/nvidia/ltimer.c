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

/*
 * Implementation of a logical timer for NVIDIA platforms.
 * NVIDIA has their own local timer implementations,
 * that vary slightly from platform to platform.  TK1 and TX1 are similar;
 * TX2 spreads them out over multiple 64k blocks.  TX2 also requires specific
 * interrupt routing from the NVidia timer to the shared interrupt controller.
 * Refer to the respective reference manual for more specific platform differences.
 */
#include <stdbool.h>
#include <platsupport/timer.h>
#include <platsupport/ltimer.h>
#include <platsupport/plat/timer.h>
#include <utils/util.h>

#include "../../ltimer.h"

typedef struct {
    bool started;
    nv_tmr_t nv_tmr;
    ps_io_ops_t ops;
} nv_tmr_ltimer_t;

static int get_time(void *data, uint64_t *time)
{
    assert(data != NULL);
    assert(time != NULL);

    nv_tmr_ltimer_t *nv_tmr_ltimer = data;
    *time = nv_tmr_get_time(&nv_tmr_ltimer->nv_tmr);
    return 0;
}

static int get_resolution(void *data, uint64_t *resolution)
{
    return ENOSYS;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    assert(data != NULL);
    nv_tmr_ltimer_t *nv_tmr_ltimer = data;

    switch (type) {
    case TIMEOUT_ABSOLUTE: {
        uint64_t time = nv_tmr_get_time(&nv_tmr_ltimer->nv_tmr);
        if (time >= ns) {
            return ETIME;
        }
        return nv_tmr_set_timeout(&nv_tmr_ltimer->nv_tmr, false, ns - time);
    }
    case TIMEOUT_PERIODIC:
    case TIMEOUT_RELATIVE:
        return nv_tmr_set_timeout(&nv_tmr_ltimer->nv_tmr, (type == TIMEOUT_PERIODIC), ns);
    }

    return EINVAL;
}

static int reset(void *data)
{
    assert(data != NULL);
    nv_tmr_ltimer_t *nv_tmr_ltimer = data;
    nv_tmr_start(&nv_tmr_ltimer->nv_tmr);
    nv_tmr_stop(&nv_tmr_ltimer->nv_tmr);
    return 0;
}

static void destroy(void *data)
{
    assert(data);
    nv_tmr_ltimer_t *nv_tmr_ltimer = data;
    if (nv_tmr_ltimer->started) {
        nv_tmr_stop(&nv_tmr_ltimer->nv_tmr);
        nv_tmr_destroy(&nv_tmr_ltimer->nv_tmr);
    }
    ps_free(&nv_tmr_ltimer->ops.malloc_ops, sizeof(nv_tmr_ltimer), nv_tmr_ltimer);
}

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops, ltimer_callback_fn_t callback, void *callback_token)
{
    if (!ltimer) {
        return EINVAL;
    }

    ltimer->get_time = get_time;
    ltimer->get_resolution = get_resolution;
    ltimer->set_timeout = set_timeout;
    ltimer->reset = reset;
    ltimer->destroy = destroy;

    int error = ps_calloc(&ops.malloc_ops, 1, sizeof(nv_tmr_ltimer_t), &ltimer->data);
    if (error) {
        return error;
    }
    assert(ltimer->data != NULL);
    nv_tmr_ltimer_t *nv_tmr_ltimer = ltimer->data;
    nv_tmr_ltimer->ops = ops;

    error = nv_tmr_init(&nv_tmr_ltimer->nv_tmr, ops, NV_TMR_PATH, callback, callback_token);
    if (error) {
        destroy(ltimer->data);
        return error;
    }

    nv_tmr_start(&nv_tmr_ltimer->nv_tmr);
    nv_tmr_ltimer->started = true;
    /* success! */
    return 0;
}

/* This function is intended to be deleted,
 * this is just left here for now so that stuff can compile */
int ltimer_default_describe(ltimer_t *ltimer, ps_io_ops_t ops)
{
    ZF_LOGE("get_(nth/num)_(irqs/pmems) are not valid");
    return EINVAL;
}
