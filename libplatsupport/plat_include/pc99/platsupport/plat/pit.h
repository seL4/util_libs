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
#pragma once

#include <autoconf.h>
#include <platsupport/io.h>
#include <platsupport/timer.h>

#ifdef CONFIG_IRQ_PIC
#define PIT_INTERRUPT       0
#else
#define PIT_INTERRUPT       2
#endif

#define PIT_IO_PORT_MIN 0x40
#define PIT_IO_PORT_MAX 0x43

#define TICKS_PER_SECOND 1193182

#define PIT_NS_TO_TICKS(ns) ((ns) * NS_IN_S / TICKS_PER_SECOND)

#define PIT_MIN_TICKS 2
#define PIT_MAX_TICKS 0xFFFF

#define PIT_MIN_NS PIT_NS_TO_TICKS(PIT_MIN_TICKS)
#define PIT_MAX_NS PIT_NS_TO_TICKS(PIT_MAX_TICKS)

typedef struct {
    ps_io_port_ops_t ops;
} pit_t;

static inline timer_properties_t get_pit_properties(void)
{
   return (timer_properties_t) {
        .upcounter = false,
        .timeouts = true,
        .relative_timeouts = true,
        .periodic_timeouts = true,
        .absolute_timeouts = false,
        .bit_width = 16,
        .irqs = 1
    };
}

/*
 * Get the pit interface. This may only be called once.
 *
 * @param io_port_ops io port operations. This is all the pit requires.
 * @return initialised interface, NULL on error.
 */
int pit_init(pit_t *pit, ps_io_port_ops_t io_port_ops);
int pit_cancel_timeout(pit_t *pit);
uint64_t pit_get_time(pit_t *pit);
int pit_set_timeout(pit_t *pit, uint64_t ns, bool periodic);
