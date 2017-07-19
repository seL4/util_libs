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

#include <errno.h>
#include <platsupport/io.h>
#include <platsupport/plat/pit.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <utils/util.h>

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

/* helper functions */
static inline int
set_pit_mode(ps_io_port_ops_t *ops, uint8_t channel, uint8_t mode)
{
    return ps_io_port_out(ops, PIT_IOPORT_PITCR, 1,
                          PITCR_SET_CHANNEL(PITCR_SET_OP_MODE(PITCR_SET_ACCESS_MODE(0, PITCR_ACCESS_LOHI), mode), channel));
}

static inline int
configure_pit(pit_t *pit, uint8_t mode, uint64_t ns)
{

    int error;

    if (ns > (0xFFFFFFFFFFFFFFFFllu / TICKS_PER_SECOND)) {
        /* ns will overflow out calculation, but also way too high for pit */
        return EINVAL;
    }

    uint64_t ticks = ns * TICKS_PER_SECOND / NS_IN_S;
    if (ticks < 2) {
        /* ns is too low */
        ZF_LOGE("Ticks too low\n");
        return ETIME;
    }

    /* pit is only 16 bits */
    if (ticks > 0xFFFF) {
        /* ticks too high */
        ZF_LOGE("Ticks too high\n");
        return EINVAL;
    }

    /* configure correct mode */
    error = set_pit_mode(&pit->ops, 0, mode);
    if (error) {
        ZF_LOGE("ps_io_port_out failed on channel %u\n", PIT_IOPORT_CHANNEL(0));
        return EIO;
    }

    /* program timeout */
    error = ps_io_port_out(&pit->ops, PIT_IOPORT_CHANNEL(0), 1, (uint8_t) (ticks & 0xFF));
    if (error) {
        ZF_LOGE("ps_io_port_out failed on channel %u\n", PIT_IOPORT_CHANNEL(0));
        return EIO;
    }

    error = ps_io_port_out(&pit->ops, PIT_IOPORT_CHANNEL(0), 1, (uint8_t) (ticks >> 8) & 0xFF);
    if (error) {
        ZF_LOGE("ps_io_port_out failed on channel %u\n", PIT_IOPORT_CHANNEL(0));
        return EIO;
    }

    return 0;
}

/* interface functions */

int pit_cancel_timeout(pit_t *pit)
{
    /* There's no way to disable the PIT, so we set it up in mode 0 and don't
     * start it
     */
    return set_pit_mode(&pit->ops, 0, PITCR_MODE_ONESHOT);
}

uint64_t pit_get_time(pit_t *pit)
{
    int error = ps_io_port_out(&pit->ops, PIT_IOPORT_CHANNEL(3), 1, 0);
    if (error) {
        return 0;
    }

    uint32_t low, high;

    /* Read the low 8 bits of the current timer value. */
    error = ps_io_port_in(&pit->ops, PIT_IOPORT_CHANNEL(0), 1, &low);
    if (error) {
        return 0;
    }

    /* Read the high 8 bits of the current timer value. */
    error = ps_io_port_in(&pit->ops, PIT_IOPORT_CHANNEL(0), 1, &high);
    if (error) {
        return 0;
    }

    /* Assemble the high and low 8 bits using (high << 8) + low, and then convert to nanoseconds. */
    return ((high << 8) + low) * NS_IN_S / TICKS_PER_SECOND;
}

int pit_set_timeout(pit_t *pit, uint64_t ns, bool periodic)
{
    uint32_t mode = periodic ? PITCR_MODE_PERIODIC : PITCR_MODE_ONESHOT;
    return configure_pit(pit, mode, ns);
}

/* initialisation function */
int pit_init(pit_t *pit, ps_io_port_ops_t io_port_ops)
{
    if (pit == NULL) {
        return EINVAL;
    }

    pit->ops = io_port_ops;
    return 0;
}
