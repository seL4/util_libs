/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <utils/attribute.h>
#include <platsupport/plat/hpet.h>
#include <platsupport/plat/pit.h>

/* just read the tsc. This may be executed out of order as it is unserialised */
static inline uint64_t
rdtsc_pure(void)
{
    uint32_t high, low;

    __asm__ __volatile__ (
        "rdtsc"
        : "=a" (low),
        "=d" (high)
        : /* no input */
        : /* no clobbers */
    );

    return (((uint64_t) high) << 32llu) + (uint64_t) low;

}

/* serialised read of the tsc. This will execute in order and no memory loads will be executed
 * beforehand */
static inline uint64_t
rdtsc_cpuid(void)
{

    uint32_t high, low;

    __asm__ __volatile__ (
        "movl $0, %%eax \n"
        "movl $0, %%ecx \n"
        "cpuid          \n"
        "rdtsc          \n"
        "movl %%edx, %0 \n"
        "movl %%eax, %1 \n"
        "movl $0, %%eax \n"
        "movl $0, %%ecx \n"
        "cpuid          \n"
        : "=r" (high), "=r" (low)
        : /* no inputs */
        : "eax", "ebx", "ecx", "edx"
    );

    return ((uint64_t) high) << 32llu | (uint64_t) low;
}

#define TSC_TICKS_TO_NS(cycles_per_us) ((rdtsc_pure() / (uint64_t) cycles_per_us) * NS_IN_US)

/**
 * Calculates number of ticks per second of the time stamp counter
 * This function takes complete control of the given timer for
 * the duraction of the calculation and will reprogram it. It
 * may also leave un-acked interrupts
 *
 * @return Ticks per second, or 0 on error
 */
uint64_t tsc_calculate_frequency_hpet(const hpet_t *hpet);
uint64_t tsc_calculate_frequency_pit(pit_t *pit);

static inline uint64_t tsc_get_time(uint64_t freq)
{
    return muldivu64(rdtsc_pure(), NS_IN_S, freq);
}
