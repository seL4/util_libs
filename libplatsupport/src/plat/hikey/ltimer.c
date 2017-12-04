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

/* This driver is a combinatorial device driver, in that it combines the
 * functionality of 2 devices into a "superdevice" to get a better featureset.
 *
 * Specifically, it combines the 1Hz upcounter of the hikey RTC with one of the
 * 19.2MHz downcounters to get a proper timestamp facility, and presents this
 * combined device as a single logical pseudo-device.
 */
#define DMT_ID DMTIMER0
#define RTC_ID RTC0
#define TIMEOUT_DMT 0
#define TIMESTAMP_DMT 1
#define NUM_DMTS 2

typedef struct {
    rtc_t rtc;
    dmt_t dmts[NUM_DMTS];
    /* Vaddrs of the underlying devices. */
    void *rtc_vaddr;
    /* hikey dualtimers have 2 timers per frame, we just use one */
    void *dmt_vaddr;
    ps_io_ops_t ops;
} hikey_ltimer_t;

static size_t get_num_irqs(void *data)
{
    /* one for the timeout dmt */
    return 1;
}

static int get_nth_irq(void *data, size_t n, ps_irq_t *irq)
{
    assert(n < get_num_irqs(data));
    irq->type = PS_INTERRUPT;
    irq->irq.number = dmt_get_irq(DMT_ID + TIMEOUT_DMT);
    return 0;
}

static size_t get_num_pmems(void *data)
{
    /* 1 for RTC, 1 for DMT */
    return 2;
}

static int get_nth_pmem(void *data, size_t n, pmem_region_t *region)
{
    region->length = PAGE_SIZE_4K;
    if (n == 0) {
        /* dm timer */
        region->base_addr = (uintptr_t) dmt_get_paddr(DMT_ID);
    } else {
        assert(n == 1);
        region->base_addr = (uintptr_t) rtc_get_paddr(RTC_ID);
    }
    return 0;
}

static int get_time(void *data, uint64_t *time)
{
    hikey_ltimer_t *hikey_ltimer = data;
    assert(data != NULL);
    assert(time != NULL);

    rtc_t *rtc = &hikey_ltimer->rtc;
    dmt_t *dualtimer = &hikey_ltimer->dmts[TIMESTAMP_DMT];

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

    uint32_t sec_value0 = rtc_get_time(rtc);
    uint64_t subsec_value = dmt_get_time(dualtimer);

    /* Read the 1Hz RTC again in case the seconds incremented while we
     * read the subsecond value.
     */
    uint32_t sec_value1 = rtc_get_time(rtc);
    if (sec_value1 != sec_value0) {
        /* re-read subsec value. Assumption is that it won't take
         * a whole second to read from the dual-timer's downcounter.
         */
       subsec_value = dmt_get_time(dualtimer);
    }

    /* The subsecond value is from a downcounter, not an upcounter, so
     * we need to subtract to know how many nanoseconds have passed since the
     * last reload.
     */
    subsec_value = NS_IN_S - subsec_value;
    *time = ((uint64_t)sec_value1) * NS_IN_S + subsec_value;
    return 0;
}

int handle_irq(void *data, ps_irq_t *irq)
{
    hikey_ltimer_t *hikey_ltimer = data;
    if (irq->irq.number == dmt_get_irq(DMT_ID + TIMEOUT_DMT)) {
        dmt_handle_irq(&hikey_ltimer->dmts[TIMEOUT_DMT]);
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
    rtc_stop(&hikey_ltimer->rtc);
    rtc_start(&hikey_ltimer->rtc);
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
        error = get_nth_pmem(data, 0,  &region);
        assert(!error);
        ps_pmem_unmap(&hikey_ltimer->ops, region, hikey_ltimer->dmt_vaddr);
    }

    if (hikey_ltimer->rtc_vaddr) {
        rtc_stop(&hikey_ltimer->rtc);
        error = get_nth_pmem(data, 1, &region);
        assert(!error);
        ps_pmem_unmap(&hikey_ltimer->ops, region, hikey_ltimer->rtc_vaddr);
    }
    ps_free(&hikey_ltimer->ops.malloc_ops, sizeof(hikey_ltimer), hikey_ltimer);
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

    int error = ps_calloc(&ops.malloc_ops, 1, sizeof(hikey_ltimer_t), &ltimer->data);
    if (error) {
        return error;
    }
    assert(ltimer->data != NULL);
    hikey_ltimer_t *hikey_ltimer = ltimer->data;
    hikey_ltimer->ops = ops;

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

    /* set up a DMT for subsecond timestamps */
    dmt_config.id++;
    if (!error) {
        error = dmt_init(&hikey_ltimer->dmts[TIMESTAMP_DMT], dmt_config);
    }
    if (!error) {
        dmt_start(&hikey_ltimer->dmts[TIMESTAMP_DMT]);
    }
    if (!error) {
        error = dmt_set_timeout(&hikey_ltimer->dmts[TIMESTAMP_DMT], NS_IN_S, true, false);
    }

    /* map in the frame for the RTC */
    error = get_nth_pmem(NULL, 1, &region);
    hikey_ltimer->rtc_vaddr = ps_pmem_map(&ops, region, false, PS_MEM_NORMAL),
    assert(error == 0);
    if (hikey_ltimer->rtc_vaddr == NULL) {
        error = ENOMEM;
    }

    /* set the rtc to track the timestamp in seconds */
    rtc_config_t rtc_config = {
        .id = RTC_ID,
        .vaddr = hikey_ltimer->rtc_vaddr
    };
    if (!error) {
        error = rtc_init(&hikey_ltimer->rtc, rtc_config);
    }
    if (!error) {
        error = rtc_start(&hikey_ltimer->rtc);
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
