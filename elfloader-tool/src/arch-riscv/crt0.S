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
.extern main
.global _start
.extern __global_pointer$
.extern elfloader_stack_alloc
.extern boot_seconday_core

#define BIT(n) (1 << (n))

_start:

.option push
.option norelax
1:auipc gp, %pcrel_hi(__global_pointer$)
  addi  gp, gp, %pcrel_lo(1b)
.option pop

  /* mhartid value is in a0 */
  /* la sp, (elfo loader_stack_alloc + BIT(12)) */
  li      sp, BIT(12)
  mul     sp, sp, a0
  la      x1, elfloader_stack_alloc
  add     sp, sp, x1

  bne x0, a0, boot_seconday_core

  la s0, main
  jr s0

.align 8
.space 4096
_bootstack_top_core1:
