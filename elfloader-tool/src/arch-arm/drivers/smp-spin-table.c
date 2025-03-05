/*
 * Copyright 2025, Kry10 Pty. Ltd.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <elfloader_common.h>
#include <devices_gen.h>
#include <drivers/common.h>
#include <drivers/smp.h>
#include <armv/machine.h>

#include <armv/smp.h>

#include <printf.h>
#include <types.h>

static int smp_spin_table_cpu_on(UNUSED struct elfloader_device *dev,
                                 UNUSED struct elfloader_cpu *cpu, UNUSED void *entry, UNUSED void *stack)
{
#if CONFIG_MAX_NUM_NODES > 1
    secondary_data.entry = entry;
    secondary_data.stack = stack;
    // cpu->extra_data holds the address that the core is spinning on from the device tree
    // Writing a new program address to this spinning address will cause the core to jump execution.
    // Caches on the target cores are disabled, and our caches are disabled.
    // We use dmb() to ensure that once the update to the spin table address is observed,
    // the secondary_data writes are also observed by the remote core.
    dmb();
    // This write is sandwiched between a system dmb and system dsb both of which have ::memory clobber
    // on the inline assembly. The effect is that the write here will be serialized after the previous writes,
    // And the dsb() will wait for the write to complete before starting the following sev.
    *((unsigned long *)(cpu->extra_data)) = (unsigned long)&secondary_startup;
    dsb();
    // Send an event to other cores in the system that may be in a wfi/wfe loop.
    asm volatile("sev" ::: "memory");
    return 0;
#else
    return -1;
#endif
}

static int smp_spin_table_init(struct elfloader_device *dev,
                               UNUSED void *match_data)
{
    smp_register_handler(dev);
    return 0;
}


static const struct dtb_match_table smp_spin_table_matches[] = {
    { .compatible = "raspberrypi,bcm2835-firmware" },
    { .compatible = NULL /* sentinel */ },
};

static const struct elfloader_smp_ops smp_spin_table_ops = {
    .enable_method = "spin-table",
    .cpu_on = &smp_spin_table_cpu_on,
};

static const struct elfloader_driver smp_spin_table = {
    .match_table = smp_spin_table_matches,
    .type = DRIVER_SMP,
    .init = &smp_spin_table_init,
    .ops = &smp_spin_table_ops,
};

ELFLOADER_DRIVER(smp_spin_table);
