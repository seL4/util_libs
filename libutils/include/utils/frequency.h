/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef _UTILS_FREQUENCY_H
#define _UTILS_FREQUENCY_H

#include <utils/time.h>

#define KHZ (1000)
#define MHZ (1000 * KHZ)
#define GHZ (1000 * MHZ)

typedef uint64_t freq_t;

static inline uint64_t freq_cycles_and_hz_to_ns(uint64_t ncycles, freq_t hz) {
    return (ncycles * NS_IN_S) / hz;
}

static inline freq_t freq_cycles_and_ns_to_hz(uint64_t ncycles, uint64_t ns) {
    return (ncycles * NS_IN_S) / ns;
}

static inline uint64_t freq_ns_and_hz_to_cycles(uint64_t ns, freq_t hz) {
    return (ns * hz) / NS_IN_S;
}

#endif /* _UTILS_FREQUENCY_H */
