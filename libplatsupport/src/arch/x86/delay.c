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
#include <platsupport/arch/tsc.h>
#include <platsupport/delay.h>
#include <stdint.h>
#include <stdio.h>

#define DEFAULT_CPUFREQ 1500000000UL

static unsigned long cpufreq_hint = DEFAULT_CPUFREQ;
#define CYCLES_PER_US(cpufreq) (cpufreq / 1000000)

static void
ps_do_cycle_delay(uint64_t cycles)
{
    uint64_t end = rdtsc_pure() + cycles;

    while (1) {
        if (end <= rdtsc_pure()) {
            break;
        }
    }
}

void
ps_udelay(unsigned long us)
{
    ps_do_cycle_delay((uint64_t)us * CYCLES_PER_US(cpufreq_hint));
}

void
ps_cpufreq_hint(unsigned long hz)
{
    if (hz == 0) {
        ZF_LOGW("%s:%d - Invalid CPU frequency for delay loop, use the default\n",
		__FILE__, __LINE__);
        hz = DEFAULT_CPUFREQ;
    }

    cpufreq_hint = hz;
}
