/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <autoconf.h>
#include <elfloader/gen_config.h>

.extern main
.extern __global_pointer$
.extern elfloader_stack_alloc
.extern hsm_exists

#define BIT(n) (1 << (n))
#define SBI_HSM_BASE 0x48534DULL
#define SBI_HSM_BASE_HART_START 0
#define SBI_EXT_BASE 0x10
#define SBI_EXT_BASE_PROBE_EXT 3

.section ".text.start"

/* OpenSBI starts all HARTs and sends them to this entry point. */
.global _start
_start:

.option push
.option norelax
1:auipc gp, %pcrel_hi(__global_pointer$)
  addi  gp, gp, %pcrel_lo(1b)
.option pop

/* a0 should have hart id, store it in s0 so as not to clobber from HSM calls */
  mv s0, a0

#ifdef CONFIG_IMAGE_BINARY
/* Clear the BSS before we get to do anything more specific */
  jal clear_bss
#endif

/* Check Heart State Management extension exists so we can use that to switch harts
   if we are not hart 1, otherwise rely on the legacy extension. */

/* sbi_probe_extension: Returns 0 if the given SBI extension ID (EID) is not available,
   or an extension-specific non-zero value if it is available.*/

  jal spi_probe_extension
  mv a0, s0
  li s1, 1
  blt x1, s1, _start1

/* Update bool to tell boot code HSM exists */
  la t1, hsm_exists
  li t2, 1
  amoadd.w t1, t2, (t1)

  mv a0, s0
  li s0, CONFIG_FIRST_HART_ID
  beq  a0, s0, _start1

/* start hart 1*/
hsm_switch_hart:
  li a7, SBI_HSM_BASE
  li a6, SBI_HSM_BASE_HART_START
  li a0, 1       // hart id to start
  la a1, _start1 // where to start new hart
  li a2, 0       // passed in a1 when new hart starts, this is the logical hart_id
  ecall

  mv a0, s0 // Copy the ID of the HART again, since we've clobbered a0
  j secondary_harts

_start1:

.option push
.option norelax
1:auipc gp, %pcrel_hi(__global_pointer$)
  addi  gp, gp, %pcrel_lo(1b)
.option pop

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

.global secondary_harts
secondary_harts:

.option push
.option norelax
1:auipc gp, %pcrel_hi(__global_pointer$)
  addi  gp, gp, %pcrel_lo(1b)
.option pop

#if CONFIG_MAX_NUM_NODES > 1
  la a1, next_logical_core_id
  li t2, 1
  amoadd.w t0, t2, (a1)
  /* now a1 has the logical core id */
  li t2, CONFIG_MAX_NUM_NODES
  bge t0, t2, spin_hart

  mv a1, t0
  slli t0, t0, 12
  la sp, bootstack_secondary_cores
  add sp, sp, t0
  la s0, secondary_entry
  jr s0
#endif
spin_hart:
  wfi
  j spin_hart

spi_probe_extension:
  li a7, SBI_EXT_BASE
  li a6, SBI_EXT_BASE_PROBE_EXT
  li a0, SBI_HSM_BASE
  ecall
  ret
