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
#include <autoconf.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include <utils/util.h>

#include <platsupport/arch/generic_timer.h>

#ifdef CONFIG_EXPORT_PCNT_USER

/*
 * This timer will only work if the kernel has configured
 * CNTPCT to be read from user-level. This is done by writing 1 to CNTKCTL.
 * If CONFIG_DANGEROUS_CODE_INJECTION is available, we'll try that,
 * otherwise we will use a default frequency.
 *
 * If all else fails the timer will fail to initialise.
 *
 */
#define MCR(cpreg, v)                               \
    do {                                            \
        uint32_t _v = v;                            \
        asm volatile("mcr  " cpreg :: "r" (_v));    \
    }while(0)
#define MRRC(cpreg, v)  asm volatile("mrrc  " cpreg :  "=r"(v))
#define MRC(cpreg, v)  asm volatile("mrc  " cpreg :  "=r"(v))

#define CNTFRQ     " p15, 0,  %0, c14,  c0, 0" /* 32-bit RW Counter Frequency register */
#define CNTPCT     " p15, 0, %Q0, %R0, c14" /* 64-bit RO Physical Count register */
#define CNTKCTL    " p15, 0,  %0, c14,  c1, 0" /* 32-bit RW Timer PL1 Control register */

uint64_t generic_timer_get_time(generic_timer_t *timer)
{
    uint64_t time;

    MRRC(CNTPCT, time);

    /* convert to ns */
    return time / (uint64_t) timer->freq * NS_IN_US;
}

int generic_timer_get_init(generic_timer_t *timer)
{

    if (timer == NULL) {
        ZF_LOGE("Must provide memory for generic timer");
        return EINVAL;
    }

    timer->freq = 0;

    /* try to read the frequency */
    MRC(CNTFRQ, timer->freq);

#ifdef PCT_TICKS_PER_US
    if (timer->freq == 0) {
        freq = PCT_TICKS_PER_US;
    }
#endif

    if (timer->freq == 0) {
        /* fail init, we don't know what the frequency of the timer is */
        ZF_LOGE("Failed to find generic timer frequency");
        return ENXIO;
    }

    return 0;
}
#endif /* CONFIG_EXPORT_PCNT_USER */
