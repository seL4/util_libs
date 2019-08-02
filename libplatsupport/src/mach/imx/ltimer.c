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

/* Implementation of a logical timer for imx platforms
 *
 * Currently all imx platforms use some combination of GPT and EPIT timers to provide ltimer functionality. See platform specific timer.h for details.
 */
#include <platsupport/timer.h>
#include <platsupport/ltimer.h>
#include <platsupport/plat/timer.h>
#include <platsupport/irq.h>

#include <utils/util.h>

#include "../../ltimer.h"

#define TIMESTAMP_IDX 0
#define TIMEOUT_IDX 1

static ps_irq_t imx_ltimer_irqs[] = {
    {
        .type = PS_INTERRUPT,
        .irq.number = TIMESTAMP_INTERRUPT,
    },
    {
        .type = PS_INTERRUPT,
        .irq.number = TIMEOUT_INTERRUPT,
    }
};

static pmem_region_t imx_ltimer_paddrs[] = {
    {
        .type = PMEM_TYPE_DEVICE,
        .base_addr = TIMESTAMP_DEVICE_PADDR,
        .length = PAGE_SIZE_4K
    },
    {
        .type = PMEM_TYPE_DEVICE,
        .base_addr = TIMEOUT_DEVICE_PADDR,
        .length = PAGE_SIZE_4K
    }
};

#define N_IRQS ARRAY_SIZE(imx_ltimer_irqs)
#define N_PADDRS ARRAY_SIZE(imx_ltimer_paddrs)

typedef struct {
    imx_timers_t timers;
    void *timer_vaddrs[N_PADDRS];
    irq_id_t timer_irq_ids[N_IRQS];
    timer_callback_data_t callback_datas[N_IRQS];
    ps_io_ops_t ops;
} imx_ltimer_t;

static size_t get_num_irqs(void *data)
{
    return N_IRQS;
}

static int get_nth_irq(void *data, size_t n, ps_irq_t *irq)
{
    assert(n < N_IRQS);
    *irq = imx_ltimer_irqs[n];
    return 0;
}

static size_t get_num_pmems(void *data)
{
    return N_PADDRS;
}

static int get_nth_pmem(void *data, size_t n, pmem_region_t *paddr)
{
    *paddr = imx_ltimer_paddrs[n];
    return 0;
}

static int handle_irq(void *data, ps_irq_t *irq)
{
    assert(data != NULL);
    imx_ltimer_t *imx_ltimer = data;
    switch (irq->irq.number) {
        case TIMESTAMP_INTERRUPT:
            handle_irq_timestamp(&imx_ltimer->timers);
            break;
        case TIMEOUT_INTERRUPT:
            handle_irq_timeout(&imx_ltimer->timers);
            break;
        default:
            ZF_LOGE("Unknown irq");
            return EINVAL;
    }

    return 0;
}

static int get_time(void *data, uint64_t *time)
{
    assert(data != NULL);
    assert(time != NULL);

    imx_ltimer_t *imx_ltimer = data;
    *time = imx_get_time(&imx_ltimer->timers);
    return 0;
}

static int get_resolution(void *data, uint64_t *resolution)
{
    return ENOSYS;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    assert(data != NULL);
    imx_ltimer_t *imx_ltimer = data;

    if (type == TIMEOUT_ABSOLUTE) {
        uint64_t current_time = imx_get_time(&imx_ltimer->timers);
        ns -= current_time;
    }

    return imx_set_timeout(&imx_ltimer->timers, ns, type == TIMEOUT_PERIODIC);
}

static int reset(void *data)
{
    assert(data != NULL);
    imx_ltimer_t *imx_ltimer = data;

    /* reset the timers */
    imx_stop_timeout(&imx_ltimer->timers);
    imx_stop_timestamp(&imx_ltimer->timers);
    imx_start_timestamp(&imx_ltimer->timers);

    return 0;
}

static void destroy(void *data)
{
    assert(data);

    imx_ltimer_t *imx_ltimer = data;

    int error = 0;

    for (int i = 0; i < N_IRQS; i++) {
        if (imx_ltimer->timer_irq_ids[i] > PS_INVALID_IRQ_ID) {
            error = ps_irq_unregister(&imx_ltimer->ops.irq_ops, imx_ltimer->timer_irq_ids[i]);
            ZF_LOGF_IF(error, "Failed to unregister IRQ!");
        }
    }

    for (int i = 0; i < N_PADDRS; i++) {
        if (imx_ltimer->timer_vaddrs[i]) {
            if (i == TIMESTAMP_IDX) {
                imx_stop_timestamp(&imx_ltimer->timers);
            } else {
                imx_stop_timeout(&imx_ltimer->timers);
            }
            ps_io_unmap(&imx_ltimer->ops.io_mapper, imx_ltimer->timer_vaddrs[i], PAGE_SIZE_4K);
        }
    }

    ps_free(&imx_ltimer->ops.malloc_ops, sizeof(imx_ltimer), imx_ltimer);
}

static int create_ltimer(ltimer_t *ltimer, ps_io_ops_t ops)
{
    assert(ltimer != NULL);
    ltimer->handle_irq = handle_irq;
    ltimer->get_time = get_time;
    ltimer->get_resolution = get_resolution;
    ltimer->set_timeout = set_timeout;
    ltimer->reset = reset;
    ltimer->destroy = destroy;

    int error = ps_calloc(&ops.malloc_ops, 1, sizeof(imx_ltimer_t), &ltimer->data);
    if (error) {
        return error;
    }
    assert(ltimer->data != NULL);

    /* Initialise the IRQ IDs */
    imx_ltimer_t *imx_ltimer = ltimer->data;
    for (int i = 0; i < N_IRQS; i++) {
        imx_ltimer->timer_irq_ids[i] = PS_INVALID_IRQ_ID;
    }

    return 0;
}

static int init_ltimer(ltimer_t *ltimer)
{
    assert(ltimer != NULL);
    imx_ltimer_t *imx_ltimer = ltimer->data;

    int error = imx_init_timestamp(&imx_ltimer->timers, imx_ltimer->timer_vaddrs[TIMESTAMP_IDX]);
    if (error) {
        ZF_LOGE("Failed to init timestamp timer");
        ltimer_destroy(ltimer);
        return error;
    }

    imx_start_timestamp(&imx_ltimer->timers);

    error = imx_init_timeout(&imx_ltimer->timers, imx_ltimer->timer_vaddrs[TIMEOUT_IDX]);
    if (error) {
        ZF_LOGE("Failed to init timeout timer");
        ltimer_destroy(ltimer);
        return error;
    }
    return 0;
}

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops)
{
    int error = ltimer_default_describe(ltimer, ops);
    if (error) {
        return error;
    }

    error = create_ltimer(ltimer, ops);
    if (error) {
        return error;
    }

    imx_ltimer_t *imx_ltimer = ltimer->data;

    imx_ltimer->ops = ops;

    /* register the interrupts that we need */
    for (int i = 0; i < N_IRQS; i++) {
        imx_ltimer->callback_datas[i] = (timer_callback_data_t) { .ltimer = ltimer,
                                                                  .irq = &imx_ltimer_irqs[i]};
        imx_ltimer->timer_irq_ids[i] = ps_irq_register(&ops.irq_ops, imx_ltimer_irqs[i],
                                                       handle_irq_wrapper, &imx_ltimer->callback_datas[i]);
        if (imx_ltimer->timer_irq_ids[i] < 0) {
            ltimer_destroy(ltimer);
            return EIO;
        }
    }

    /* map the frames we need */
    for (int i = 0; i < N_PADDRS; i++) {
        imx_ltimer->timer_vaddrs[i] = ps_pmem_map(&ops, imx_ltimer_paddrs[i], false, PS_MEM_NORMAL);
        if (imx_ltimer->timer_vaddrs[i] == NULL) {
            ltimer_destroy(ltimer);
            return ENOMEM;
        }
    }

    init_ltimer(ltimer);
    if (error) {
        ltimer_destroy(ltimer);
        return error;
    }

    /* success! */
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
