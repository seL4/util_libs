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

#include <platsupport/arch/tsc.h>
#include <utils/time.h>
#include <stdio.h>

#define WAIT_SECONDS 2ull
#define WAIT_NS (NS_IN_S * WAIT_SECONDS) /* 2 seconds in nano seconds to wait for */

uint64_t tsc_calculate_frequency_hpet(const hpet_t *hpet)
{
    uint64_t tsc_start = rdtsc_pure();
    uint64_t hpet_start = hpet_get_time(hpet);
    /* spin until WAIT_NS has passed */
    while (hpet_get_time(hpet) - hpet_start < WAIT_NS);
    uint64_t tsc_end = rdtsc_pure();
    return (tsc_end - tsc_start) / WAIT_SECONDS;
}

uint64_t tsc_calculate_frequency_pit(pit_t *pit)
{
    /* the PIT should be able to set a timeout for 50 ms */
    uint64_t wait_ns = 50 * NS_IN_MS;


    int error = pit_set_timeout(pit, wait_ns, true);
    assert(error == 0);

    uint64_t start_time = rdtsc_pure();
    uint64_t last_absolute = 0;
    uint64_t total_observed = 0;
    uint64_t time_offset = pit_get_time(pit);
    uint64_t last_time = time_offset;

    while (total_observed + last_absolute < WAIT_NS) {
        uint64_t current_time = pit_get_time(pit);
        if (current_time > last_time) {
            total_observed += wait_ns;
        }
        last_absolute = wait_ns - current_time + time_offset;
        last_time = current_time;
    }
    uint64_t end_time = rdtsc_pure();
    /* well that was a fucking trial. Now hopefully we got something sane */
    return (end_time - start_time) / WAIT_SECONDS;
}
