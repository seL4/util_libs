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

#include <autoconf.h>
#include <elfloader/gen_config.h>
.extern main
.global _start
.extern __global_pointer$
.extern elfloader_stack_alloc

#define BIT(n) (1 << (n))
.section ".text.start"

_start:

.option push
.option norelax
1:auipc gp, %pcrel_hi(__global_pointer$)
  addi  gp, gp, %pcrel_lo(1b)
.option pop

/* BBL starts all HARTs and sends them to this entry point. */
  li s0, CONFIG_FIRST_HART_ID
  bne  a0, s0, secondary_harts

  la sp, (elfloader_stack_alloc + BIT(12))

  la s0, main
  jr s0

#if CONFIG_MAX_NUM_NODES > 1
.extern next_logical_core_id
.data
bootstack_secondary_cores:
.align 12
.space 4096 * (CONFIG_MAX_NUM_NODES - 1)
#endif

.text

secondary_harts:
#if CONFIG_MAX_NUM_NODES > 1
  la a1, next_logical_core_id
  li t2, 1
  amoadd.w a1, t2, (a1)
  /* now a1 has the logical core id */
  li t2, CONFIG_MAX_NUM_NODES
  bge a1, t2, spin_hart
  mv t0, a1
  slli t0, t0, 12
  la sp, bootstack_secondary_cores
  add sp, sp, t0
  la s0, secondary_entry
  jr s0
#endif
spin_hart:
  wfi
  j spin_hart
