/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <platsupport/arch/tsc.h>
#include <utils/time.h>
#include <stdio.h>

#define WAIT_SECONDS 2ull
#define WAIT_NS (NS_IN_S * WAIT_SECONDS) /* 2 seconds in nano seconds to wait for */

uint64_t
tsc_calculate_frequency(pstimer_t *timer)
{
    /* Our goal here is to set a timer for a long enough time to get
     * an accurate measure of the tsc frequency. To that end we want
     * to wait for some small N seconds. However, if the timer we
     * have been given cannot do such a long wait then we will
     * attempt a shorter wait and try and observe multiple overflows
     */
    uint64_t wait_ns = WAIT_NS;
    uint64_t total_observed = 0;
    uint64_t last_time;
    uint64_t last_absolute = 0;
    int direction = timer->properties.upcounter;
    uint64_t start_time = 0;
    if (!timer->properties.timeouts) {
        return 0;
    }
    do {
        switch (timer_periodic(timer, wait_ns)) {
        case ENOSYS:
            /* the fuck? */
            printf("Timer claimed to support timeouts, but doesn't\n");
            return 0;
        case ETIME:
            printf("Could not find a time that was not too large or too small for this timer\n");
            return 0;
        case EINVAL:
            wait_ns /= 2;
            break;
        default:
            start_time = rdtsc_pure();
            break;
        }
    } while(!start_time);
    /* observe the timer and try and work out when enough time has elapsed.
     * this gets tricky as some timers might count overflows internally and
     * report back a monotonically increasing time, where as others might not
     * While the former is unlikely to happen as interrupts shouldn't be being
     * processed, take no chances.
     * If we do overflow though, we assume that we overflowed after the
     * programmed number of nanoseconds */
    last_time = timer_get_time(timer);
    while(total_observed + last_absolute < WAIT_NS) {
        uint64_t current_time = timer_get_time(timer);
        if (direction) {
            /* if we are counting up and time went down, then we overflowed */
            if (current_time < last_time) {
                total_observed += wait_ns;
            }
            last_absolute = current_time;
        } else {
            if (current_time > last_time) {
                total_observed += wait_ns;
            }
            /* might have counted too far */
            if (current_time > wait_ns) {
                last_absolute = wait_ns;
            } else {
                last_absolute = wait_ns - current_time;
            }
        }
        last_time = current_time;
    }
    uint64_t end_time = rdtsc_pure();
    /* well that was a fucking trial. Now hopefully we got something sane */
    return (end_time - start_time) / WAIT_SECONDS;
}
