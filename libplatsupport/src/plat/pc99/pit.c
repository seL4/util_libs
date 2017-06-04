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

#ifndef __PLATSUPPORT_TIMERDEV_H__
#define __PLATSUPPORT_TIMERDEV_H__

#include <errno.h>

#include <platsupport/timer.h>
#include <platsupport/io.h>
#include <platsupport/plat/pit.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <utils/util.h>
#include <utils/util.h>

#include "../../stubtimer.h"

#define PIT_IOPORT_CHANNEL(x) (0x40 + x) /* valid channels are 0, 1, 2. we'll be using 0 exclusively, though */
#define PIT_IOPORT_COMMAND  0x43
#define PIT_IOPORT_PITCR    PIT_IOPORT_COMMAND

/* PIT command register macros */
#define PITCR_SET_CHANNEL(x, channel)   (((channel) << 6) | x)
#define PITCR_SET_OP_MODE(x, mode)      (((mode) << 1)    | x)
#define PITCR_SET_ACCESS_MODE(x, mode)  (((mode) << 4)    | x)

#define PITCR_LATCH_CHANNEL(channel)    PITCR_SET_CHANNEL(0, channel)

#define PITCR_ACCESS_LOW   0x1
#define PITCR_ACCESS_HIGH  0x2
#define PITCR_ACCESS_LOHI  0x3

#define PITCR_MODE_ONESHOT   0x0
#define PITCR_MODE_PERIODIC  0x2
#define PITCR_MODE_SQUARE    0x3
#define PITCR_MODE_SWSTROBE  0x4

#define TICKS_PER_SECOND 1193182
#define PIT_PERIODIC_MAX 54925000

typedef struct {
    ps_io_port_ops_t *ops;
} pit_data_t;

/* helper functions */
static inline int
set_pit_mode(ps_io_port_ops_t *ops, uint8_t channel, uint8_t mode)
{
    return ps_io_port_out(ops, PIT_IOPORT_PITCR, 1,
                          PITCR_SET_CHANNEL(PITCR_SET_OP_MODE(PITCR_SET_ACCESS_MODE(0, PITCR_ACCESS_LOHI), mode), channel));
}

static inline int
configure_pit(const pstimer_t *timer, uint8_t mode, uint64_t ns)
{

    int error;

    if (ns > (0xFFFFFFFFFFFFFFFFllu / TICKS_PER_SECOND)) {
        /* ns will overflow out calculation, but also way too high for pit */
        return EINVAL;
    }

    uint64_t ticks = ns * TICKS_PER_SECOND / NS_IN_S;
    if (ticks < 2) {
        /* ns is too low */
        fprintf(stderr, "Ticks too low\n");
        return ETIME;
    }

    /* pit is only 16 bits */
    if (ticks > 0xFFFF) {
        /* ticks too high */
        fprintf(stderr, "Ticks too high\n");
        return EINVAL;
    }

    ps_io_port_ops_t *ops = ((pit_data_t *) timer->data)->ops;

    /* configure correct mode */
    error = set_pit_mode(ops, 0, mode);
    if (error) {
        fprintf(stderr, "ps_io_port_out failed on channel %u\n", PIT_IOPORT_CHANNEL(0));
        return EIO;
    }

    /* program timeout */
    error = ps_io_port_out(ops, PIT_IOPORT_CHANNEL(0), 1, (uint8_t) (ticks & 0xFF));
    if (error) {
        fprintf(stderr, "ps_io_port_out failed on channel %u\n", PIT_IOPORT_CHANNEL(0));
        return EIO;
    }

    error = ps_io_port_out(ops, PIT_IOPORT_CHANNEL(0), 1, (uint8_t) (ticks >> 8) & 0xFF);
    if (error) {
        fprintf(stderr, "ps_io_port_out failed on channel %u\n", PIT_IOPORT_CHANNEL(0));
        return EIO;
    }

    return 0;
}


/* interface functions */

static int
pit_start(const pstimer_t* device)
{
    /* we don't need to do anything to start the pit */
    return 0;
}


static int
pit_stop(const pstimer_t* device)
{
    /* There's no way to disable the PIT, so we set it up in mode 0 and don't
     * start it
     */
    ps_io_port_ops_t *ops = ((pit_data_t *) device->data)->ops;
    return set_pit_mode(ops, 0, PITCR_MODE_ONESHOT);
}


static uint64_t
pit_get_time(const pstimer_t* device)
{
    ps_io_port_ops_t *ops = ((pit_data_t *) device->data)->ops;
    int error = ps_io_port_out(ops, PIT_IOPORT_CHANNEL(3), 1, 0);
    if (error) {
        return 0;
    }

    uint32_t low, high;

    /* Read the low 8 bits of the current timer value. */
    error = ps_io_port_in(ops, PIT_IOPORT_CHANNEL(0), 1, &low);
    if (error) {
        return 0;
    }

    /* Read the high 8 bits of the current timer value. */
    error = ps_io_port_in(ops, PIT_IOPORT_CHANNEL(0), 1, &high);
    if (error) {
        return 0;
    }

    /* Assemble the high and low 8 bits using (high << 8) + low, and then convert to nanoseconds. */
    return ((high << 8) + low) * NS_IN_S / TICKS_PER_SECOND;
}

static int
pit_oneshot_relative(const pstimer_t* device, uint64_t relative_ns)
{
    return configure_pit(device, PITCR_MODE_ONESHOT, relative_ns);
}


static int
pit_periodic(const pstimer_t* device, uint64_t ns)
{
    return configure_pit(device, PITCR_MODE_PERIODIC, ns);
}

static void
pit_handle_irq(const pstimer_t* device, uint32_t irq)
{
    /* do nothing */
}

static uint32_t
pit_get_nth_irq(const pstimer_t *device, uint32_t n)
{
    uint32_t irq = 0;

    if (n == 0) {
        irq = PIT_INTERRUPT;
    }

    assert(n == 0);
    return irq;
}

/* static global vars */
static pstimer_t pit_timer = {
    .properties = {
        .upcounter = false,
        .timeouts = true,
        .relative_timeouts = true,
        .periodic_timeouts = true,
        .absolute_timeouts = false,
        .bit_width = 16,
        .irqs = 1
    },
    .start = pit_start,
    .stop = pit_stop,
    .get_time = pit_get_time,
    .oneshot_absolute = stub_timer_timeout,
    .oneshot_relative = pit_oneshot_relative,
    .periodic = pit_periodic,
    .handle_irq = pit_handle_irq,
    .get_nth_irq = pit_get_nth_irq,
    /* data is set to null originally to prevent double initilisation */
    .data = NULL
};

static pit_data_t pit_data;

/* initialisation function */
pstimer_t *
pit_get_timer(ps_io_port_ops_t *io_port_ops)
{

    /* timer already initialised */
    if (pit_timer.data != NULL) {
        return &pit_timer;
    }

    pit_timer.data = &pit_data;
    pit_data.ops = io_port_ops;

    return &pit_timer;
}

#endif /* __PLATSUPPORT_TIMER_H__ */
