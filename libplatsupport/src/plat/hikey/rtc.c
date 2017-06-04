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

#include <platsupport/timer.h>
#include <platsupport/plat/timer.h>

#include "timer_priv.h"

#define HIKEY_RTCCR_ENABLE_SHIFT    (0)

typedef volatile struct hikey_rtc_regs {
    uint32_t rtcdr;     /* Data reg: RO: returns current value of 1Hz upcounter */
    uint32_t rtcmr;     /* Comparison reg: RW: for the alarm (irq) feature.
                         * Generates an IRQ when rtcmr == rtcdr.
                         * Don't care: we don't use the alarm feature.
                         */
    uint32_t rtclr;     /* Load reg: RW: the initial value of the upcounter. */
    uint32_t rtccr;     /* Control reg: literally only has an enable bit (bit 0)
                         * and that's it. Absolute control.
                         */
    uint32_t rtcimsc;   /* Interrupt mask reg: Don't care. we don't use the alarm
                         * feature.
                         */
    uint32_t rtcris;    /* Raw Interrupt status reg: don't care. */
    uint32_t rtcmis;    /* Masked interrupt status reg: don't care. */
    uint32_t rtcicr;    /* interrupt clear reg: don't care. */
} hikey_rtc_regs_t;

typedef struct hikey_rtc_privdata {
    int timer_id;
    hikey_rtc_regs_t *vaddr;
    uint32_t irq;
} hikey_rtc_privdata_t;

static inline hikey_rtc_privdata_t *
hikey_rtc_get_privdata(const pstimer_t *timer)
{
    assert(timer != NULL);
    return (hikey_rtc_privdata_t *)timer->data;
}

static inline hikey_rtc_regs_t *
hikey_rtc_get_regs(const pstimer_t *timer)
{
    hikey_rtc_privdata_t *priv = hikey_rtc_get_privdata(timer);

    assert(priv != NULL);
    assert(priv->vaddr != NULL);
    return priv->vaddr;
}

static void
hikey_rtc_initialize(const pstimer_t *timer)
{
    hikey_rtc_regs_t *regs = hikey_rtc_get_regs(timer);

    /* We set the initial value of the upcounter to 0 when
     * initializing, because we have no reason to use any
     * other value, and because we have no guarantee that
     * it starts at a sensibly low value if not explicitly set.
     */
    regs->rtclr = 0;
    regs->rtccr = BIT(HIKEY_RTCCR_ENABLE_SHIFT);
}

static int
hikey_rtc_start(const pstimer_t* device)
{
    return 0;
}

static int
hikey_rtc_stop(const pstimer_t* device)
{
    hikey_rtc_regs_t *regs = hikey_rtc_get_regs(device);

    regs->rtccr = 0;
    return 0;
}

static uint32_t
hikey_rtc_get_nth_irq(const pstimer_t *device, UNUSED uint32_t n)
{
    return hikey_rtc_get_privdata(device)->irq;
}

/* The rest are all just dummy functions. */
static int
hikey_rtc_oneshot_absolute(UNUSED const pstimer_t* device, UNUSED uint64_t ns)
{
    return ENOSYS;
}

static int
hikey_rtc_oneshot_relative(UNUSED const pstimer_t* device, UNUSED uint64_t ns)
{
    return ENOSYS;
}

static int
hikey_rtc_periodic(UNUSED const pstimer_t* device, UNUSED uint64_t ns)
{
    return ENOSYS;
}

static void
hikey_rtc_handle_irq(UNUSED const pstimer_t* device, UNUSED uint32_t irq)
{
}

static uint64_t
hikey_rtc_get_timestamp(const pstimer_t *timer)
{
    hikey_rtc_regs_t *regs = hikey_rtc_get_regs(timer);

    return regs->rtcdr;
}

static timer_properties_t hikey_rtc_props = {
    .upcounter = true,
    .timeouts = false,
    .absolute_timeouts = false,
    .relative_timeouts = false,
    .periodic_timeouts = false,

    .bit_width = 32,
    .irqs = 0
};

typedef struct hikey_rtc_descriptor {
    pstimer_t apihandle;
    hikey_rtc_privdata_t priv;
} hikey_rtc_descriptor_t;

static hikey_rtc_descriptor_t hikey_rtc_descriptors[NUM_RTCS];

pstimer_t *
hikey_rtc_get_timer(int timer_id, timer_config_t *config)
{
    pstimer_t *ret;

    if (timer_id < RTC0 || timer_id > RTC1) {
        ZF_LOGE("Invalid timer device ID for a hikey RTC timer.");
        return NULL;
    } else {
        timer_id -= RTC0;
    }

    if (config == NULL || config->vaddr == NULL) {
        ZF_LOGE("Vaddr of the mapped RTC device register frame is required.");
        return NULL;
    }

    ret = &hikey_rtc_descriptors[timer_id].apihandle;
    ret->data = &hikey_rtc_descriptors[timer_id].priv;
    ret->properties = hikey_rtc_props;

    hikey_rtc_descriptors[timer_id].priv.timer_id = timer_id + RTC0;
    hikey_rtc_descriptors[timer_id].priv.vaddr = (hikey_rtc_regs_t *)config->vaddr;
    hikey_rtc_descriptors[timer_id].priv.irq = config->irq;

    ret->start = hikey_rtc_start;
    ret->stop = hikey_rtc_stop;
    ret->get_time = hikey_rtc_get_timestamp;
    ret->oneshot_absolute = hikey_rtc_oneshot_absolute;
    ret->oneshot_relative = hikey_rtc_oneshot_relative;
    ret->periodic = hikey_rtc_periodic;
    ret->handle_irq = hikey_rtc_handle_irq;
    ret->get_nth_irq = hikey_rtc_get_nth_irq;

    hikey_rtc_initialize(ret);
    return ret;
}
