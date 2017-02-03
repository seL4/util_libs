/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(D61_BSD)
 */
#include <stdio.h>
#include <assert.h>

#include <utils/util.h>
#include <utils/time.h>

#include <platsupport/timer.h>
#include <platsupport/plat/timer.h>

#include "timer_priv.h"

/* This driver is a combinatorial device driver, in that it combines the
 * functionality of 2 devices into a "superdevice" to get a better featureset.
 *
 * Specifically, it combines the 1Hz upcounter of the hikey RTC with one of the
 * 19.2MHz downcounters to get a proper timestamp facility, and presents this
 * combined device as a single logical pseudo-device.
 */

typedef struct hikey_vupcounter_privdata {
    /* The platsupport timer IDs for the RTC and dualtimer being combined. */
    int rtc_id, dualtimer_id;
    /* A logical ID created based on the two real hardware IDs. It's created by
     * simply binary ORing them together with the RTC ID making up the high
     * bits.
     */
    uint16_t timer_id;
    /* Vaddrs of the underlying devices. */
    void *rtc_vaddr, *dualtimer_vaddr;
    /* Handles to the underlying devices. */
    pstimer_t *rtc, *dualtimer;
} hikey_vupcounter_privdata_t;

static inline hikey_vupcounter_privdata_t *
hikey_vupcounter_get_privdata(const pstimer_t *vupcounter)
{
    assert(vupcounter != NULL);
    return (hikey_vupcounter_privdata_t *)vupcounter->data;
}

static inline pstimer_t *
hikey_vupcounter_get_rtc_instance(const pstimer_t *vupcounter)
{
    assert(hikey_vupcounter_get_privdata(vupcounter) != NULL);
    return hikey_vupcounter_get_privdata(vupcounter)->rtc;
}

static inline pstimer_t *
hikey_vupcounter_get_dualtimer_instance(const pstimer_t *vupcounter)
{
    assert(hikey_vupcounter_get_privdata(vupcounter) != NULL);
    return hikey_vupcounter_get_privdata(vupcounter)->dualtimer;
}

static uint64_t
hikey_vupcounter_get_timestamp(const pstimer_t *timer)
{
    pstimer_t *rtc, *dualtimer;
    uint32_t sec_value0, sec_value1, subsec_value;

    rtc = hikey_vupcounter_get_rtc_instance(timer);
    assert(rtc != NULL);
    dualtimer = hikey_vupcounter_get_dualtimer_instance(timer);
    assert(dualtimer != NULL);

   /* The problem is that the RTC is a 1Hz upcounter.
    * And the dual-counters (DMtimer) are downcounters.
    *
    * So it's fairly unintuitive to get a decent sub-second
    * offset value to tack on to the end of the RTC's 1Hz
    * value.
    *
    * We can use the second timer of DMtimer0 to provide
    * a subsecond offset from the 1Hz value of the RTC.
    *
    * In order to do this, we can run the RTC timer as a periodic
    * downcounter whose IRQ is disabled. Then we just read from its
    * current-downcount value everytime we want a subsecond timestamp
    * offset.
    */

    sec_value0 = timer_get_time(rtc);
    subsec_value = timer_get_time(dualtimer);

    /* Read the 1Hz RTC again in case the seconds incremented while we
     * read the subsecond value.
     */
    sec_value1 = timer_get_time(rtc);
    if (sec_value1 != sec_value0) {
        /* re-read subsec value. Assumption is that it won't take
         * a whole second to read from the dual-timer's downcounter.
         */
       subsec_value = timer_get_time(dualtimer);
    }

    /* The subsecond value is from a downcounter, not an upcounter, so
     * we need to subtract to know how many nanoseconds have passed since the
     * last reload.
     */
    subsec_value = NS_IN_S - subsec_value;
    return ((uint64_t)sec_value1 << 32) + subsec_value;
}

/* The rest of the functions are wrappers around the underlying devices. */
static int
hikey_vupcounter_start(const pstimer_t* device)
{
    pstimer_t *rtc = hikey_vupcounter_get_rtc_instance(device);
    assert(rtc != NULL);

    return timer_start(rtc);
}

static int
hikey_vupcounter_stop(const pstimer_t* device)
{
    pstimer_t *rtc = hikey_vupcounter_get_rtc_instance(device);
    assert(rtc != NULL);

    return timer_stop(rtc);
}

static int
hikey_vupcounter_oneshot_absolute(const pstimer_t* device, uint64_t ns)
{
    return ENOSYS;
}

static int
hikey_vupcounter_oneshot_relative(const pstimer_t* device, uint64_t ns)
{
    return ENOSYS;
}

static int
hikey_vupcounter_periodic(const pstimer_t* device, uint64_t ns)
{
    return ENOSYS;
}

static void
hikey_vupcounter_handle_irq(const pstimer_t* device, uint32_t irq)
{
}

static uint32_t
hikey_vupcounter_get_nth_irq(const pstimer_t *device, uint32_t n)
{
    return 0;
}

static void
hikey_vupcounter_initialize(const pstimer_t *device)
{
    pstimer_t *dualtimer;

    dualtimer = hikey_vupcounter_get_dualtimer_instance(device);
    assert(dualtimer != NULL);

    /* Put it in periodic mode, without interrupts enabled, and have it wrap
     * around and reload once per second.
     */
    dm_set_timeo(dualtimer, 1 * NS_IN_S, TCLR_AUTORELOAD);
}

static timer_properties_t virtual_upcounter_props = {
    .upcounter = true,
    .timeouts = false,
    .absolute_timeouts = false,
    .relative_timeouts = false,
    .periodic_timeouts = false,

    .bit_width = 32,
    .irqs = 0
};

typedef struct hikey_vupcounter_descriptor {
    pstimer_t apihandle;
    hikey_vupcounter_privdata_t priv;
} hikey_vupcounter_descriptor_t;

static hikey_vupcounter_descriptor_t hikey_vupcounter_descriptor;

pstimer_t *
hikey_vupcounter_get_timer(int rtc_id, int dualtimer_id,
                           hikey_vupcounter_timer_config_t *config)
{
    pstimer_t *ret;

    if (rtc_id < RTC0 || rtc_id > RTC1) {
        ZF_LOGE("Invalid rtc ID %d.", rtc_id);
        return NULL;
    }

    if (dualtimer_id < DMTIMER0 || dualtimer_id > DMTIMER17) {
        ZF_LOGE("Invalid dual timer ID %d.", dualtimer_id);
        return NULL;
    }

    ret = &hikey_vupcounter_descriptor.apihandle;
    ret->properties = virtual_upcounter_props;
    ret->data = &hikey_vupcounter_descriptor.priv;

    hikey_vupcounter_descriptor.priv.rtc_id = rtc_id;
    hikey_vupcounter_descriptor.priv.dualtimer_id = dualtimer_id;
    hikey_vupcounter_descriptor.priv.timer_id = (rtc_id << 8) | dualtimer_id;

    /* Assign the pstimer_t instance pointers for the underlying rtc and
     * dualtimer devices.
     */
    hikey_vupcounter_descriptor.priv.rtc = config->rtc_timer;
    hikey_vupcounter_descriptor.priv.dualtimer = config->dualtimer_timer;

    ret->start = hikey_vupcounter_start;
    ret->stop = hikey_vupcounter_stop;
    ret->get_time = hikey_vupcounter_get_timestamp;
    ret->oneshot_absolute = hikey_vupcounter_oneshot_absolute;
    ret->oneshot_relative = hikey_vupcounter_oneshot_relative;
    ret->periodic = hikey_vupcounter_periodic;
    ret->handle_irq = hikey_vupcounter_handle_irq;
    ret->get_nth_irq = hikey_vupcounter_get_nth_irq;

    hikey_vupcounter_initialize(ret);
    return ret;
}
