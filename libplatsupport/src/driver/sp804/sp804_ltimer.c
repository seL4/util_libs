/*
 * Copyright 2022, HENSOLDT Cyber GmbH
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <assert.h>

#include <utils/util.h>
#include <utils/time.h>

#include <platsupport/io.h>
#include <platsupport/ltimer.h>

#include <platsupport/driver/sp804/sp804.h>
#include <platsupport/driver/sp804/sp804_ltimer.h>

#include "../../ltimer.h"

/*
 * A each sp804 timer peripheral has two timers. Timer #1 keeps track of the
 * timeouts and timer #2 tracks the absolute time. It depends on the platform
 * if there is one interrupt for both timers or separate interrupts.
 */

typedef struct {
    ps_io_ops_t ops;
    ltimer_callback_fn_t callback_func;
    void *callback_token;
    uint32_t time_h;
    pmem_region_t pmem;
    void *mapping;
    uint64_t freq_100KHz;
    irq_id_t irq_id;
    irq_id_t irq2_id;
} ltimer_sp804_t;

static sp804_regs_t *ltimer_sp804_get_regs_timeout(ltimer_sp804_t *ltimer)
{
    return SP804_TIMER1_REGS(ltimer->mapping);
}

static sp804_regs_t *ltimer_sp804_get_regs_timestamp(ltimer_sp804_t *ltimer)
{
    return SP804_TIMER2_REGS(ltimer->mapping);
}

static uint64_t ltimer_sp804_get_time(ltimer_sp804_t *ltimer)
{
    sp804_regs_t *regs = ltimer_sp804_get_regs_timestamp(ltimer);

    /* sp804 is a down counter, invert the result */
    uint32_t high = ltimer->time_h;
    uint32_t low = UINT32_MAX - sp804_get_ticks(regs);

    /* check after fetching low to see if we've missed a high bit */
    if (sp804_is_irq_pending(regs)) {
        high += 1;
        assert(high != 0);
    }

    uint64_t ticks = ((uint64_t)high << 32) | low;
    uint64_t ns = (ticks * 10000) / ltimer->freq_100KHz;
    return ns;
}

int ltimer_sp804_simple_set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    assert(data != NULL);
    ltimer_sp804_t *ltimer = data;

    /* calculate relative timeout for absolute timeout */
    if (TIMEOUT_ABSOLUTE == type) {
        uint64_t time_ns_now = ltimer_sp804_get_time(ltimer);
        /* The timeout can't be in the past */
        if (time_ns_now > ns) {
            return ETIME;
        }
        ns -= time_ns_now;
    }

    uint64_t ticks64 = (ns * ltimer->freq_100KHz) / 10000;
    /* There is no trivial way to set values greater 32-bit. */
    if (ticks64 > UINT32_MAX) {
        ZF_LOGE("timout of %"PRIu64" ns (%"PRIu64" ticks) exceeds 32-bit range",
                ns, ticks64);
        return ETIME;
    }

    /* The value 0 makes the interrupt trigger immediately.
     * ToDo: might be better to reject a periodic timer with a timeout of 0.
     */
    sp804_set_timeout(ltimer_sp804_get_regs_timeout(ltimer),
                      (uint32_t)ticks64, (type == TIMEOUT_PERIODIC), true);
    return 0;
}

static int ltimer_sp804_simple_get_time(void *data, uint64_t *time)
{
    assert(data != NULL);
    assert(time != NULL);
    ltimer_sp804_t *ltimer = data;
    *time = ltimer_sp804_get_time(ltimer);
    return 0;
}

static int ltimer_sp804_simple_reset(void *data)
{
    ltimer_sp804_t *ltimer = data;

    sp804_regs_t *regs_timeout = ltimer_sp804_get_regs_timeout(ltimer);
    sp804_reset(regs_timeout);

    sp804_regs_t *regs_timestamp = ltimer_sp804_get_regs_timestamp(ltimer);
    sp804_reset(regs_timestamp);
    sp804_start(regs_timestamp);

    return 0;
}

static void ltimer_sp804_simple_destroy(void *data)
{
    int error;

    assert(data != NULL);
    ltimer_sp804_t *ltimer = data;

    if (ltimer->irq_id != PS_INVALID_IRQ_ID) {
        error = ps_irq_unregister(&ltimer->ops.irq_ops, ltimer->irq_id);
        if (error) {
            /* This is considered as a fatal error */
            ZF_LOGF("Failed to unregister IRQ (%d)", error);
            UNREACHABLE();
        }
    }

    if (ltimer->irq2_id != PS_INVALID_IRQ_ID) {
        error = ps_irq_unregister(&ltimer->ops.irq_ops, ltimer->irq2_id);
        if (error) {
            /* This is considered as a fatal error */
            ZF_LOGF("Failed to unregister IRQ (%d)", error);
            UNREACHABLE();
        }
    }

    ps_pmem_unmap(&ltimer->ops, ltimer->pmem, (void *)ltimer->mapping);

    ps_free(&ltimer->ops.malloc_ops, sizeof(ltimer_sp804_t), ltimer);
}

static void ltimer_sp804_handle_irq(void *data, ps_irq_acknowledge_fn_t ack_fn,
                                    void *ack_data)
{
    assert(data != NULL);
    ltimer_sp804_t *ltimer = data;

    sp804_regs_t *regs_timestamp = ltimer_sp804_get_regs_timestamp(ltimer);
    bool is_overflow = false;
    if (sp804_is_irq_pending(regs_timestamp)) {
        is_overflow = true;
        sp804_clear_intr(regs_timestamp);
        ltimer->time_h++;
    }

    sp804_regs_t *regs_timeout = ltimer_sp804_get_regs_timeout(ltimer);
    bool is_timeout = false;
    if (sp804_is_irq_pending(regs_timeout)) {
        is_timeout = true;
        sp804_clear_intr(regs_timeout);
    }

    if (ack_fn) {
        int error = ack_fn(ack_data);
        if (error) {
            /* This is considered as a fatal error */
            ZF_LOGF("Failed to acknowledge the timer's interrupts (%d)", error);
            UNREACHABLE();
        }
    }

    if (ltimer->callback_func) {
        if (is_timeout) {
            ltimer->callback_func(ltimer->callback_token, LTIMER_TIMEOUT_EVENT);
        }
        if (is_overflow) {
            ltimer->callback_func(ltimer->callback_token, LTIMER_OVERFLOW_EVENT);
        }
    }
}

int ltimer_sp804_init(ltimer_t *ltimer, const char *ftd_path,
                      uint64_t freq, ps_io_ops_t ops,
                      ltimer_callback_fn_t callback, void *callback_token)
{
    int error;

    if (ltimer == NULL) {
        ZF_LOGE("ltimer cannot be NULL");
        return EINVAL;
    }

    error = create_ltimer_simple(ltimer, ops, sizeof(ltimer_sp804_t),
                                 ltimer_sp804_simple_get_time,
                                 ltimer_sp804_simple_set_timeout,
                                 ltimer_sp804_simple_reset,
                                 ltimer_sp804_simple_destroy);
    if (error) {
        ZF_LOGE("Failed to create ltimer simple (%d)", error);
        return error;
    }

    ltimer_sp804_t *ltimer_sp804 = ltimer->data;

    ltimer_sp804->ops = ops;
    ltimer_sp804->callback_func = callback,
    ltimer_sp804->callback_token = callback_token,

    /* Assume the frequency is a multiple of 100 KHz so the numbers do not get
     * too big during calculations and there are less rounding errors.
     */
    assert(0 == (freq % 100000));
    ltimer_sp804->freq_100KHz = freq / 100000;

    /* Set up timer1 for timeouts and timer2 for timestamps. We can't use
     * helper_fdt_alloc_simple() here, because there are platforms where the
     * timers have independent interrupts.
     * Set safe default to allow cleanup if init fails
     */
    ltimer_sp804->mapping = NULL;
    ltimer_sp804->irq_id = PS_INVALID_IRQ_ID;
    ltimer_sp804->irq2_id = PS_INVALID_IRQ_ID;

    /* Gather FDT info */
    ps_fdt_cookie_t *cookie = NULL;
    error = ps_fdt_read_path(&ops.io_fdt, &ops.malloc_ops, ftd_path, &cookie);
    if (error) {
        ZF_LOGE("failed to read path (%d, %s)", error, ftd_path);
        goto init_cleanup;
    }

    ltimer_sp804->mapping = ps_fdt_index_map_register(&ops, cookie, 0,
                                                      &ltimer_sp804->pmem);
    if (ltimer_sp804->mapping == NULL) {
        ZF_LOGE("failed to map registers");
        goto init_cleanup;
    }

    ltimer_sp804->irq_id = ps_fdt_index_register_irq(&ops, cookie, 0,
                                                     ltimer_sp804_handle_irq,
                                                     ltimer_sp804);
    if (ltimer_sp804->irq_id <= PS_INVALID_IRQ_ID) {
        ZF_LOGE("failed to register irq (%d)", ltimer_sp804->irq_id);
        goto init_cleanup;
    }

#ifdef SP804_LTIMER_MULTIPLE_INTR
    ltimer_sp804->irq2_id = ps_fdt_index_register_irq(&ops, cookie, 1,
                                                      ltimer_sp804_handle_irq,
                                                      ltimer_sp804);
    if (ltimer_sp804->irq2_id <= PS_INVALID_IRQ_ID) {
        ZF_LOGE("failed to register irq 2 (%d)", ltimer_sp804->irq2_id);
        goto init_cleanup;
    }
#endif /* SP804_LTIMER_MULTIPLE_INTR */

init_cleanup:
    if (cookie) {
        int error2 = ps_fdt_cleanup_cookie(&ops.malloc_ops, cookie);
        if (error2) {
            ZF_LOGE("failed to clean up FTD cookie (%d)", error2);
            error = error2;
        }
    }
    if (error) {
        ltimer_sp804_simple_destroy(ltimer_sp804);
        ZF_LOGE("sp804 ltimer init failed (%d)", error);
        return error;
    }

    sp804_regs_t *regs_timeout = ltimer_sp804_get_regs_timeout(ltimer_sp804);
    sp804_regs_t *regs_timestamp = ltimer_sp804_get_regs_timestamp(ltimer_sp804);

    sp804_reset(regs_timeout);
    sp804_reset(regs_timestamp);

    /* Start the timestamp counter (setting a value implies starting). */
    sp804_set_timeout(regs_timestamp, UINT32_MAX, true, true);

    return 0;
}
