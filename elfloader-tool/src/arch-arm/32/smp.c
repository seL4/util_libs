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

#include <autoconf.h>
#include <elfloader/gen_config.h>

#if CONFIG_MAX_NUM_NODES > 1
#include <types.h>
#include <elfloader.h>
#include <armv/machine.h>
#include <armv/smp.h>
#include <printf.h>

unsigned long core_stacks[CONFIG_MAX_NUM_NODES][STACK_SIZE / sizeof(unsigned long)] ALIGN(BIT(12));
volatile int core_up[CONFIG_MAX_NUM_NODES];

extern void core_entry_head(void);
extern void non_boot_main(void);

void core_entry(uint32_t sp)
{
    int id;
    // get the logic ID
    id = (sp - (unsigned long)&core_stacks[0][0]) / STACK_SIZE;

    core_up[id] = id;
    dsb();
    non_boot_main();
}

int is_core_up(int i)
{
    return core_up[i] == i;
}

#endif
