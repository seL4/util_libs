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

/* Minimal implementation of a logical timer for zynq
 *
 * Does not implement some functions yet.
 */
#include <platsupport/timer.h>
#include <platsupport/ltimer.h>
#include <platsupport/plat/timer.h>

#include <utils/util.h>

#include "../../ltimer.h"

/* Use ttc0_timer1 for timeouts/sleep */
#define TTC_TIMEOUT   TTC0_TIMER1
/* Use ttc1_timer1 to keep running for timestamp/gettime */
#define TTC_TIMESTAMP TTC1_TIMER1

#define N_IRQS 2
#define N_PADDRS 2

#define TIMEOUT_IDX 0
#define TIMESTAMP_IDX 1

typedef struct {
    ttc_t ttcs[N_PADDRS];
    void *timer_vaddrs[N_PADDRS];

    irq_id_t timer_irq_ids[N_IRQS];
    timer_callback_data_t callback_datas[N_IRQS];

    ltimer_callback_fn_t user_callback;
    void *user_callback_token;

    ps_io_ops_t ops;
    /* ns that have passed on the timestamp counter */
    uint64_t time;
} ttc_ltimer_t;

static size_t get_num_irqs(void *data)
{
    return N_IRQS;
}

static int get_nth_irq(void *data, size_t n, ps_irq_t *irq)
{
    assert(n < get_num_irqs(data));

    irq->type = PS_INTERRUPT;

    switch (n) {
    case TIMEOUT_IDX:
        irq->irq.number = ttc_irq(TTC_TIMEOUT);
        break;
    case TIMESTAMP_IDX:
        irq->irq.number = ttc_irq(TTC_TIMESTAMP);
        break;
    default:
        ZF_LOGE("Invalid irq\n");
        return EINVAL;
    }

    return 0;
}

static size_t get_num_pmems(void *data)
{
    return N_PADDRS;
}

static int get_nth_pmem(void *data, size_t n, pmem_region_t *region)
{
    assert(n < get_num_pmems(data));
    region->length = PAGE_SIZE_4K;

    switch(n) {
    case TIMEOUT_IDX:
        region->base_addr = ttc_paddr(TTC_TIMEOUT);
        break;
    case TIMESTAMP_IDX:
        region->base_addr = ttc_paddr(TTC_TIMESTAMP);
        break;
    default:
        ZF_LOGE("Invalid timer number");
    }
    return 0;
}

/* increment the high bits of the counter if an irq has occured */
static void update_timestamp(ttc_ltimer_t *ttc_ltimer)
{
    if (ttc_handle_irq(&ttc_ltimer->ttcs[TIMESTAMP_IDX])) {
#ifdef CONFIG_PLAT_ZYNQMP
        /* if handle irq returns a high value, we have an overflow irq unhandled */
        ttc_ltimer->time += ttc_ticks_to_ns(&ttc_ltimer->ttcs[TIMESTAMP_IDX], UINT32_MAX);
#else
        /* if handle irq returns a high value, we have an overflow irq unhandled,
         * the only other platform, zynq7000 doesn't support ticks > 2^16 */
        ttc_ltimer->time += ttc_ticks_to_ns(&ttc_ltimer->ttcs[TIMESTAMP_IDX], UINT16_MAX);
#endif
    }
}

static int handle_irq(void *data, ps_irq_t *irq)
{
    assert(data != NULL);
    ttc_ltimer_t *ttc_ltimer = data;
    ltimer_event_t event;

    if (irq->irq.number == ttc_irq(TTC_TIMEOUT)) {
        ttc_handle_irq(&ttc_ltimer->ttcs[TIMEOUT_IDX]);
        event = LTIMER_TIMEOUT_EVENT;
    } else if (irq->irq.number == ttc_irq(TTC_TIMESTAMP)) {
        update_timestamp(ttc_ltimer);
        event = LTIMER_OVERFLOW_EVENT;
    } else {
        return EINVAL;
    }

    if (ttc_ltimer->user_callback) {
        ttc_ltimer->user_callback(ttc_ltimer->user_callback_token, event);
    }

    return 0;
}

static uint64_t read_time(ttc_ltimer_t *ttc_ltimer)
{
    uint64_t ticks = ttc_get_time(&ttc_ltimer->ttcs[TIMESTAMP_IDX]);
    update_timestamp(ttc_ltimer);
    uint64_t ticks2 = ttc_get_time(&ttc_ltimer->ttcs[TIMESTAMP_IDX]);
    if (ticks2 < ticks) {
        /* overflow occured  */
        update_timestamp(ttc_ltimer);
        ticks = ticks2;
    }

    return ticks + ttc_ltimer->time;
}

static int get_time(void *data, uint64_t *time)
{
    assert(data != NULL);
    assert(time != NULL);
    *time = read_time(data);
    return 0;
}

static int get_resolution(void *data, uint64_t *resolution)
{
    return ENOSYS;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    assert(data != NULL);
    ttc_ltimer_t *ttc_ltimer = data;

    if (type == TIMEOUT_ABSOLUTE) {
        uint64_t time = read_time(data);
        if (ns <= time) {
            return ETIME;
        } else {
            ns -= time;
        }
    }

    return ttc_set_timeout(&ttc_ltimer->ttcs[TIMEOUT_IDX], ns, type == TIMEOUT_PERIODIC);
}

static int reset(void *data)
{
    assert(data != NULL);
    ttc_ltimer_t *ttc_ltimer = data;

    /* reset the timers */
    ttc_stop(&ttc_ltimer->ttcs[TIMEOUT_IDX]);
    ttc_start(&ttc_ltimer->ttcs[TIMEOUT_IDX]);
    ttc_stop(&ttc_ltimer->ttcs[TIMESTAMP_IDX]);
    ttc_start(&ttc_ltimer->ttcs[TIMESTAMP_IDX]);

    return 0;
}

static void destroy(void *data)
{
    assert(data);

    ttc_ltimer_t *ttc_ltimer = data;

    int error;

    for (int i = 0; i < N_IRQS; i++) {
        if (ttc_ltimer->callback_datas[i].irq) {
            ps_free(&ttc_ltimer->ops.malloc_ops, sizeof(ps_irq_t), ttc_ltimer->callback_datas[i].irq);
        }

        if (ttc_ltimer->timer_irq_ids[i] > PS_INVALID_IRQ_ID) {
            error = ps_irq_unregister(&ttc_ltimer->ops.irq_ops, ttc_ltimer->timer_irq_ids[i]);
            ZF_LOGF_IF(error, "Failed to unregister timer IRQ");
        }
    }

    for (int i = 0; i < N_PADDRS; i++) {
        ttc_stop(&ttc_ltimer->ttcs[i]);
        pmem_region_t region;
        UNUSED int error = get_nth_pmem(data, i, &region);
        assert(!error);
        ps_pmem_unmap(&ttc_ltimer->ops, region, ttc_ltimer->timer_vaddrs[i]);
    }

    ps_free(&ttc_ltimer->ops.malloc_ops, sizeof(ttc_ltimer), ttc_ltimer);
}

static int create_ltimer(ltimer_t *ltimer, ps_io_ops_t ops)
{
    ltimer->get_time = get_time;
    ltimer->get_resolution = get_resolution;
    ltimer->set_timeout = set_timeout;
    ltimer->reset = reset;
    ltimer->destroy = destroy;

    int error = ps_calloc(&ops.malloc_ops, 1, sizeof(ttc_ltimer_t), &ltimer->data);
    if (error) {
        return error;
    }
    assert(ltimer->data != NULL);

    /* initialise the IRQ IDs */
    ttc_ltimer_t *ttc_ltimer = ltimer->data;
    for (int i = 0; i < N_IRQS; i++) {
        ttc_ltimer->timer_irq_ids[i] = PS_INVALID_IRQ_ID;
    }

    return 0;
}

static int init_ltimer(ltimer_t *ltimer)
{
    int error;

    assert(ltimer != NULL);
    ttc_ltimer_t *ttc_ltimer = ltimer->data;

    ttc_config_t config = {
        .vaddr = ttc_ltimer->timer_vaddrs[TIMEOUT_IDX],
        .id = TTC_TIMEOUT,
    };

    ttc_config_t config_timestamp = {
        .vaddr = ttc_ltimer->timer_vaddrs[TIMESTAMP_IDX],
        .id = TTC_TIMESTAMP,
    };

    error = ttc_init(&ttc_ltimer->ttcs[TIMEOUT_IDX] ,config);
    if (error) {
        return error;
    }

    error = ttc_start(&ttc_ltimer->ttcs[TIMEOUT_IDX]);
    if (error) {
        return error;
    }

    /* set the second ttc to be a timestamp counter */
    error = ttc_init(&ttc_ltimer->ttcs[TIMESTAMP_IDX],config_timestamp);
    if (error) {
        return error;
    }

    ttc_freerun(&ttc_ltimer->ttcs[TIMESTAMP_IDX]);
    error = ttc_start(&ttc_ltimer->ttcs[TIMESTAMP_IDX]);
    if (error) {
        return error;
    }

    return 0;
}

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops, ltimer_callback_fn_t callback, void *callback_token)
{
    int error = ltimer_default_describe(ltimer, ops);
    if (error) {
        return error;
    }

    error = create_ltimer(ltimer, ops);
    if (error) {
        return error;
    }

    ttc_ltimer_t *ttc_ltimer = ltimer->data;
    ttc_ltimer->ops = ops;
    ttc_ltimer->user_callback = callback;
    ttc_ltimer->user_callback_token = callback_token;

    /* Map the registers */
    for (int i = 0; i < N_PADDRS; i++) {
        pmem_region_t region;
        error = get_nth_pmem(ltimer->data, i, &region);
        assert(error == 0);
        ttc_ltimer->timer_vaddrs[i] = ps_pmem_map(&ops, region, false, PS_MEM_NORMAL);
        if (ttc_ltimer->timer_vaddrs[i] == NULL) {
            destroy(ttc_ltimer);
            return ENOMEM;
        }
    }

    /* Register the IRQs */
    for (int i = 0; i < N_IRQS; i++) {
        ps_irq_t irq = {0};
        if (i == TIMEOUT_IDX) {
            irq = (ps_irq_t) { .type = PS_INTERRUPT, .irq = { .number = ttc_irq(TTC_TIMEOUT) }};
        } else {
            irq = (ps_irq_t) { .type = PS_INTERRUPT, .irq = { .number = ttc_irq(TTC_TIMESTAMP) }};
        }

        error = ps_calloc(&ops.malloc_ops, 1, sizeof(ps_irq_t),
                          (void **) &ttc_ltimer->callback_datas[i].irq);
        if (error) {
            destroy(ttc_ltimer);
            return error;
        }
        ttc_ltimer->callback_datas[i].ltimer = ltimer;
        *ttc_ltimer->callback_datas[i].irq = irq;
        ttc_ltimer->callback_datas[i].irq_handler = handle_irq;

        ttc_ltimer->timer_irq_ids[i] = ps_irq_register(&ops.irq_ops, irq, handle_irq_wrapper,
                                                       &ttc_ltimer->callback_datas[i]);
        if (ttc_ltimer->timer_irq_ids[i] < 0) {
            destroy(ttc_ltimer);
            return EIO;
        }
    }

    error = init_ltimer(ltimer);
    if (error) {
        destroy(ttc_ltimer);
        return error;
    }

    return 0;
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
