/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include <utils/util.h>
#include <utils/time.h>

#include <platsupport/plat/rtc.h>

#define HIKEY_RTCCR_ENABLE_SHIFT    (0)

typedef volatile struct rtc_regs {
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
} rtc_regs_t;

static inline rtc_regs_t *
rtc_get_regs(rtc_t *rtc)
{
    assert(rtc != NULL);
    assert(rtc->vaddr != NULL);
    return rtc->vaddr;
}

int rtc_start(rtc_t *rtc)
{
    rtc_regs_t *regs = rtc_get_regs(rtc);

    /* We set the initial value of the upcounter to 0 when
     * initializing, because we have no reason to use any
     * other value, and because we have no guarantee that
     * it starts at a sensibly low value if not explicitly set.
     */
    regs->rtclr = 0;
    regs->rtccr = BIT(HIKEY_RTCCR_ENABLE_SHIFT);
    return 0;
}

int rtc_stop(rtc_t *rtc)
{
    rtc_regs_t *regs = rtc_get_regs(rtc);
    regs->rtccr = 0;
    return 0;
}

uint32_t rtc_get_time(rtc_t *rtc)
{
    rtc_regs_t *regs = rtc_get_regs(rtc);
    return regs->rtcdr;
}

int rtc_init(rtc_t *rtc, rtc_config_t config)
{
    if (config.id < RTC0 || config.id > RTC1) {
        ZF_LOGE("Invalid timer device ID for a hikey RTC timer.");
        return EINVAL;
    }

    if (config.vaddr == NULL) {
        ZF_LOGE("Vaddr of the mapped RTC device register frame is required.");
        return EINVAL;
    }

    rtc->vaddr = config.vaddr;
    return 0;
}
