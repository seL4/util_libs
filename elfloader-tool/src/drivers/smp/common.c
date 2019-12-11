/*
 * Copyright 2019, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */

#include <devices_gen.h>
#include <drivers/common.h>
#include <drivers/smp.h>
#include <elfloader_common.h>
#include <printf.h>

#include <strops.h>

static struct elfloader_device *smp_ops = NULL;

struct smp_cpu_data secondary_data;

void smp_register_handler(struct elfloader_device *dev)
{
    if (dev->drv->type != DRIVER_SMP) {
        return;
    }

    smp_ops = dev;
}

int __attribute__((weak)) plat_cpu_on(struct elfloader_cpu *cpu, void *entry, void *stack)
{
    if (!smp_ops) {
        return -1;
    }

    if (cpu->enable_method == NULL || dev_get_smp(smp_ops)->enable_method == NULL) {
        /* if cpu has a NULL enable_method, expect a driver with a NULL enable_method too */
        if (cpu->enable_method != NULL || dev_get_smp(smp_ops)-> enable_method != NULL) {
            return -1;
        }
    } else if (strcmp(cpu->enable_method, dev_get_smp(smp_ops)->enable_method)) {
        return -1;
    }

    return dev_get_smp(smp_ops)->cpu_on(smp_ops, cpu, entry, stack);
}
