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
#include <stdio.h>
#include <assert.h>

#include <utils/util.h>
#include <utils/time.h>

#include <platsupport/ltimer.h>
#include <platsupport/plat/rtc.h>
#include <platsupport/plat/dmt.h>
#include <platsupport/io.h>

#include "../../ltimer.h"

/*
 * We use two dm timers: one to keep track of an absolute time, the other for timeouts.
 */
#define DMT_ID DMTIMER0
#define TIMEOUT_DMT 0
#define TIMESTAMP_DMT 1
#define NUM_DMTS 2

typedef struct {
    dmt_t dmts[NUM_DMTS];
    /* hikey dualtimers have 2 timers per frame, we just use one */
    void *dmt_vaddr;
    irq_id_t timer_irq_ids[NUM_DMTS];
    timer_callback_data_t callback_datas[NUM_DMTS];
    ltimer_callback_fn_t user_callback;
    void *user_callback_token;
    ps_io_ops_t ops;
    uint32_t high_bits;
} hikey_ltimer_t;

static size_t get_num_irqs(void *data)
{
    /* one for each dmt */
    return 2;
}

static int get_nth_irq(void *data, size_t n, ps_irq_t *irq)
{
    assert(n < get_num_irqs(data));
    irq->type = PS_INTERRUPT;
    irq->irq.number = dmt_get_irq(DMT_ID + n);
    return 0;
}

static size_t get_num_pmems(void *data)
{
    /* 1 - both dmts are on the same page */
    return 1;
}

static int get_nth_pmem(void *data, size_t n, pmem_region_t *region)
{
    region->length = PAGE_SIZE_4K;
    /* dm timer */
    region->base_addr = (uintptr_t) dmt_get_paddr(DMT_ID);
    return 0;
}

static int get_time(void *data, uint64_t *time)
{
    hikey_ltimer_t *hikey_ltimer = data;
    assert(data != NULL);
    assert(time != NULL);

    dmt_t *dualtimer = &hikey_ltimer->dmts[TIMESTAMP_DMT];
    uint64_t low_ticks = UINT32_MAX - dmt_get_ticks(dualtimer); /* dmt is a down counter, invert the result */
    uint64_t ticks = hikey_ltimer->high_bits + !!dmt_is_irq_pending(dualtimer);
    ticks = (ticks << 32llu) + low_ticks;
    *time = dmt_ticks_to_ns(ticks);
    return 0;
}

int handle_irq(void *data, ps_irq_t *irq)
{
    hikey_ltimer_t *hikey_ltimer = data;
    if (irq->irq.number == dmt_get_irq(DMT_ID + TIMEOUT_DMT)) {
        dmt_handle_irq(&hikey_ltimer->dmts[TIMEOUT_DMT]);
    } else if (irq->irq.number == dmt_get_irq(DMT_ID + TIMESTAMP_DMT)) {
        dmt_handle_irq(&hikey_ltimer->dmts[TIMESTAMP_DMT]);
        hikey_ltimer->high_bits++;
    } else {
        ZF_LOGE("unknown irq");
        return EINVAL;
    }
    return 0;
}

int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    if (type == TIMEOUT_ABSOLUTE) {
        uint64_t time;
        int error = get_time(data, &time);
        if (error) {
            return error;
        }
        if (time > ns) {
            return ETIME;
        }
        ns -= time;
    }

    hikey_ltimer_t *hikey_ltimer = data;
    return dmt_set_timeout(&hikey_ltimer->dmts[TIMEOUT_DMT], ns,
            type == TIMEOUT_PERIODIC, true);
}

static int get_resolution(void *data, uint64_t *resolution)
{
    return ENOSYS;
}

static int reset(void *data)
{
    hikey_ltimer_t *hikey_ltimer = data;
    /* restart the rtc */
    dmt_stop(&hikey_ltimer->dmts[TIMEOUT_DMT]);
    dmt_start(&hikey_ltimer->dmts[TIMEOUT_DMT]);
    return 0;
}

static void destroy(void *data)
{
    pmem_region_t region;
    UNUSED int error;
    hikey_ltimer_t *hikey_ltimer = data;

    if (hikey_ltimer->dmt_vaddr) {
        dmt_stop(&hikey_ltimer->dmts[TIMEOUT_DMT]);
        dmt_stop(&hikey_ltimer->dmts[TIMESTAMP_DMT]);
        error = get_nth_pmem(data, 0, &region);
        assert(!error);
        ps_pmem_unmap(&hikey_ltimer->ops, region, hikey_ltimer->dmt_vaddr);
    }

    for (int i = 0; i < NUM_DMTS; i++) {
        if (hikey_ltimer->callback_datas[i].irq) {
            ps_free(&hikey_ltimer->ops.malloc_ops, sizeof(ps_irq_t), hikey_ltimer->callback_datas[i].irq);
        }

        if (hikey_ltimer->timer_irq_ids[i] > PS_INVALID_IRQ_ID) {
            error = ps_irq_unregister(&hikey_ltimer->ops.irq_ops, hikey_ltimer->timer_irq_ids[i]);
            ZF_LOGF_IF(error, "Failed to unregister IRQ");
        }
    }

    ps_free(&hikey_ltimer->ops.malloc_ops, sizeof(hikey_ltimer), hikey_ltimer);
}

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops, ltimer_callback_fn_t callback, void *callback_token)
{
    if (ltimer == NULL) {
        ZF_LOGE("ltimer cannot be NULL");
        return EINVAL;
    }

    ltimer_default_describe(ltimer, ops);
    ltimer->get_time = get_time;
    ltimer->get_resolution = get_resolution;
    ltimer->set_timeout = set_timeout;
    ltimer->reset = reset;
    ltimer->destroy = destroy;

    int error = ps_calloc(&ops.malloc_ops, 1, sizeof(hikey_ltimer_t), &ltimer->data);
    if (error) {
        return error;
    }
    assert(ltimer->data != NULL);
    hikey_ltimer_t *hikey_ltimer = ltimer->data;
    hikey_ltimer->ops = ops;
    hikey_ltimer->user_callback = callback;
    hikey_ltimer->user_callback_token = callback_token;
    for (int i = 0; i < NUM_DMTS; i++) {
        hikey_ltimer->timer_irq_ids[i] = PS_INVALID_IRQ_ID;
    }

    /* map the frame for the dm timers */
    pmem_region_t region;
    error = get_nth_pmem(NULL, 0, &region);
    assert(error == 0);
    hikey_ltimer->dmt_vaddr = ps_pmem_map(&ops, region, false, PS_MEM_NORMAL);
    if (hikey_ltimer->dmt_vaddr == NULL) {
        error = ENOMEM;
    }

    /* set up a DMT for timeouts */
    dmt_config_t dmt_config = {
        .vaddr = hikey_ltimer->dmt_vaddr,
        .id = DMT_ID
    };
    if (!error) {
        error = dmt_init(&hikey_ltimer->dmts[TIMEOUT_DMT], dmt_config);
    }
    if (!error) {
        dmt_start(&hikey_ltimer->dmts[TIMEOUT_DMT]);
    }

    /* set up a DMT for timestamps */
    dmt_config.id++;
    if (!error) {
        error = dmt_init(&hikey_ltimer->dmts[TIMESTAMP_DMT], dmt_config);
    }
    if (!error) {
        dmt_start(&hikey_ltimer->dmts[TIMESTAMP_DMT]);
    }
    if (!error) {
        error = dmt_set_timeout_ticks(&hikey_ltimer->dmts[TIMESTAMP_DMT], UINT32_MAX, true, true);
    }

    /* register IRQs for the timers */
    for (int i = 0; i < NUM_DMTS; i++) {
        error = ps_calloc(&ops.malloc_ops, 1, sizeof(ps_irq_t), (void **) &hikey_ltimer->callback_datas[i].irq);
        if (error) {
            break;
        }
        hikey_ltimer->callback_datas[i].ltimer = ltimer;
        error = get_nth_irq(hikey_ltimer, i, hikey_ltimer->callback_datas[i].irq);
        if (error) {
            break;
        }
        hikey_ltimer->timer_irq_ids[i] = ps_irq_register(&ops.irq_ops, *hikey_ltimer->callback_datas[i].irq,
                                                         handle_irq_wrapper, &hikey_ltimer->callback_datas[i]);
        if (hikey_ltimer->timer_irq_ids[i] < 0) {
            error = EIO;
            break;
        }
    }

    /* if there was an error, clean up */
    if (error) {
        destroy(ltimer);
    }
    return error;
}

int ltimer_default_describe(ltimer_t *ltimer, ps_io_ops_t ops)
{
    if (ltimer == NULL) {
        ZF_LOGE("Timer is NULL!");
        return EINVAL;
    }

    ltimer->get_num_irqs = get_num_irqs;
    ltimer->get_nth_irq = get_nth_irq;
    ltimer->get_num_pmems = get_num_pmems;
    ltimer->get_nth_pmem = get_nth_pmem;
    return 0;
}
