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
.extern main
.global _start
.extern __global_pointer$
.extern elfloader_stack_alloc

#define BIT(n) (1 << (n))
.section ".text.start"

_start:

/* BBL starts all HARTs and sends them to this entry point.
 * While we only support 1 core, we send remaining HARTs to a WFI loop.
 */
  li s0, CONFIG_FIRST_HART_ID
  bne  a0, s0, spin_hart

.option push
.option norelax
1:auipc gp, %pcrel_hi(__global_pointer$)
  addi  gp, gp, %pcrel_lo(1b)
.option pop

  la sp, (elfloader_stack_alloc + BIT(12))

  la s0, main
  jr s0
spin_hart:
  wfi
  j spin_hart


.align 8
.space 4096
_bootstack_top_core1:
