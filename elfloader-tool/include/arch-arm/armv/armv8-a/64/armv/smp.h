/*
 * Copyright 2018, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */

#pragma once

#include <autoconf.h>
#include <elfloader/gen_config.h>

#if CONFIG_MAX_NUM_NODES > 1

#define STACK_SIZE  4096

extern unsigned long core_stacks[CONFIG_MAX_NUM_NODES][STACK_SIZE / sizeof(unsigned long)] ALIGN(BIT(12));
void core_entry(uint64_t sp);
int  is_core_up(int id);

#endif
