/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <autoconf.h>
#include <platsupport/gen_config.h>
#include <platsupport/delay.h>
#include <stdint.h>
#include <stdio.h>

#if defined(CONFIG_PLAT_EXYNOS5250)
#define DEFAULT_CPUFREQ 1000000000UL
#elif defined(CONFIG_PLAT_EXYNOS5410)
#define DEFAULT_CPUFREQ 1000000000UL
#elif defined(CONFIG_PLAT_EXYNOS4)
#define DEFAULT_CPUFREQ 1000000000UL
#elif defined(CONFIG_PLAT_IMX6)
#define DEFAULT_CPUFREQ  792000000UL
#else
#define DEFAULT_CPUFREQ 0UL
#endif

static unsigned long cpufreq_hint = DEFAULT_CPUFREQ;
#define CYCLES_PER_US (cpufreq_hint / 1000000)

void ps_udelay(unsigned long us);

static void ps_do_cycle_delay(int32_t cycles)
{
    /* Loop while the number of required instructions is +ve
     * We unfold the loop to avoid branch predictor optimisation.
     * We tolerate some error if the number of instruction is not
     * a multiple of 16.
     */
    asm volatile(
        "1:"
        "    subs %0, %0, #1;"  /*  1 */
        "    subs %0, %0, #1;"  /*  2 */
        "    subs %0, %0, #1;"  /*  3 */
        "    subs %0, %0, #1;"  /*  4 */
        "    subs %0, %0, #1;"  /*  5 */
        "    subs %0, %0, #1;"  /*  6 */
        "    subs %0, %0, #1;"  /*  7 */
        "    subs %0, %0, #1;"  /*  8 */
        "    subs %0, %0, #1;"  /*  9 */
        "    subs %0, %0, #1;"  /* 10 */
        "    subs %0, %0, #1;"  /* 11 */
        "    subs %0, %0, #1;"  /* 12 */
        "    subs %0, %0, #1;"  /* 13 */
        "    subs %0, %0, #1;"  /* 14 */
        "    subs %0, %0, #1;"  /* 15 */
        "    subs %0, %0, #1;"  /* 16 */
        "    bpl 1b" : "+r"(cycles));
}

void ps_udelay(unsigned long us)
{
    if (cpufreq_hint == 0) {
        printf("%s:%d - Unable to determine CPU frequency for delay loop\n", __FILE__, __LINE__);
        cpufreq_hint = 2 * 1000 * 1000 * 1000;
    }
    while (us--) {
        ps_do_cycle_delay(CYCLES_PER_US);
    }
}

void ps_cpufreq_hint(unsigned long hz)
{
    cpufreq_hint = hz;
}
