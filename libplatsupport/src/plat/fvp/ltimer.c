/*
 * Copyright 2019, Data61
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
#include <platsupport/plat/sp804.h>
#include <platsupport/io.h>

/*
 * We use two sp804 timers: one to keep track of an absolute time, the other for timeouts.
 */
#define SP804_ID            SP804_TIMER0
#define TIMEOUT_SP804       0
#define TIMESTAMP_SP804     1
#define NUM_SP804_TIMERS    2

typedef struct {
    sp804_t sp804s[NUM_SP804_TIMERS];
    /* fvp sp804 have 2 timers per frame, we just use one */
    void *sp804_timer0_vaddr;
    void *sp804_timer1_vaddr;
    ps_io_ops_t ops;
    uint32_t high_bits;
} fvp_ltimer_t;

static size_t get_num_irqs(void *data)
{
    /* one for each sp804 */
    return 2;
}

static int get_nth_irq(void *data, size_t n, ps_irq_t *irq)
{
    assert(n < get_num_irqs(data));
    irq->type = PS_INTERRUPT;
    irq->irq.number = sp804_get_irq(SP804_ID + n);
    return 0;
}

static size_t get_num_pmems(void *data)
{
    /* 1 - both sp804s are on the same page */
    return 2;
}

static int get_nth_pmem(void *data, size_t n, pmem_region_t *region)
{
    region->length = PAGE_SIZE_4K;
    region->base_addr = (uintptr_t) sp804_get_paddr(n);
    return 0;
}

static int get_time(void *data, uint64_t *time)
{
    fvp_ltimer_t *fvp_ltimer = data;
    assert(data != NULL);
    assert(time != NULL);

    sp804_t *dualtimer = &fvp_ltimer->sp804s[TIMESTAMP_SP804];
    uint64_t low_ticks = UINT32_MAX - sp804_get_ticks(dualtimer); /* sp804 is a down counter, invert the result */
    uint64_t ticks = fvp_ltimer->high_bits + !!sp804_is_irq_pending(dualtimer);
    ticks = (ticks << 32llu) + low_ticks;
    *time = sp804_ticks_to_ns(ticks);
    return 0;
}

int handle_irq(void *data, ps_irq_t *irq)
{
    fvp_ltimer_t *fvp_ltimer = data;
    if (irq->irq.number == sp804_get_irq(SP804_ID + TIMEOUT_SP804)) {
        sp804_handle_irq(&fvp_ltimer->sp804s[TIMEOUT_SP804]);
    } else if (irq->irq.number == sp804_get_irq(SP804_ID + TIMESTAMP_SP804)) {
        sp804_handle_irq(&fvp_ltimer->sp804s[TIMESTAMP_SP804]);
        fvp_ltimer->high_bits++;
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

    fvp_ltimer_t *fvp_ltimer = data;
    return sp804_set_timeout(&fvp_ltimer->sp804s[TIMEOUT_SP804], ns,
            type == TIMEOUT_PERIODIC, true);
}

static int get_resolution(void *data, uint64_t *resolution)
{
    return ENOSYS;
}

static int reset(void *data)
{
    fvp_ltimer_t *fvp_ltimer = data;
    /* restart the rtc */
    sp804_stop(&fvp_ltimer->sp804s[TIMEOUT_SP804]);
    sp804_start(&fvp_ltimer->sp804s[TIMEOUT_SP804]);
    return 0;
}

static void destroy(void *data)
{
    pmem_region_t region;
    UNUSED int error;
    fvp_ltimer_t *fvp_ltimer = data;

    if (fvp_ltimer->sp804_timer0_vaddr) {
        sp804_stop(&fvp_ltimer->sp804s[TIMEOUT_SP804]);
        error = get_nth_pmem(data, 0,  &region);
        assert(!error);
        ps_pmem_unmap(&fvp_ltimer->ops, region, fvp_ltimer->sp804_timer0_vaddr);
    }

    if (fvp_ltimer->sp804_timer1_vaddr) {
        sp804_stop(&fvp_ltimer->sp804s[TIMESTAMP_SP804]);
        error = get_nth_pmem(data, 1,  &region);
        assert(!error);
        ps_pmem_unmap(&fvp_ltimer->ops, region, fvp_ltimer->sp804_timer1_vaddr);
    }
    ps_free(&fvp_ltimer->ops.malloc_ops, sizeof(fvp_ltimer), fvp_ltimer);
}

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops)
{
    if (ltimer == NULL) {
        ZF_LOGE("ltimer cannot be NULL");
        return EINVAL;
    }

    ltimer_default_describe(ltimer, ops);
    ltimer->handle_irq = handle_irq;
    ltimer->get_time = get_time;
    ltimer->get_resolution = get_resolution;
    ltimer->set_timeout = set_timeout;
    ltimer->reset = reset;
    ltimer->destroy = destroy;

    int error = ps_calloc(&ops.malloc_ops, 1, sizeof(fvp_ltimer_t), &ltimer->data);
    if (error) {
        return error;
    }
    assert(ltimer->data != NULL);
    fvp_ltimer_t *fvp_ltimer = ltimer->data;
    fvp_ltimer->ops = ops;

    /* map the frame for the sp804 timers */
    pmem_region_t region;
    error = get_nth_pmem(NULL, 0, &region);
    assert(error == 0);
    fvp_ltimer->sp804_timer0_vaddr = ps_pmem_map(&ops, region, false, PS_MEM_NORMAL);
    if (fvp_ltimer->sp804_timer0_vaddr == NULL) {
        error = ENOMEM;
    }

    /* set up an SP804 for timeouts */
    sp804_config_t sp804_config = {
        .vaddr = fvp_ltimer->sp804_timer0_vaddr,
        .id = SP804_ID
    };

    if (!error) {
        error = sp804_init(&fvp_ltimer->sp804s[TIMEOUT_SP804], sp804_config);
    }
    if (!error) {
        sp804_start(&fvp_ltimer->sp804s[TIMEOUT_SP804]);
    }

    /* set up a SP804_TIMER for timestamps */
    sp804_config.id++;
    error = get_nth_pmem(NULL, 1, &region);
    assert(error == 0);

    fvp_ltimer->sp804_timer1_vaddr = ps_pmem_map(&ops, region, false, PS_MEM_NORMAL);
    if (fvp_ltimer->sp804_timer1_vaddr == NULL) {
        error = ENOMEM;
    }

    sp804_config.vaddr = fvp_ltimer->sp804_timer1_vaddr;

    if (!error) {
        error = sp804_init(&fvp_ltimer->sp804s[TIMESTAMP_SP804], sp804_config);
    }
    if (!error) {
        sp804_start(&fvp_ltimer->sp804s[TIMESTAMP_SP804]);
    }
    if (!error) {
        error = sp804_set_timeout_ticks(&fvp_ltimer->sp804s[TIMESTAMP_SP804], UINT32_MAX, true, true);
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
