/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include <autoconf.h>

#ifdef ARMV5

.text

/*
 * Enable ARM MMU V5
 * r0: pagetable
 */

.extern _boot_pd 

.global arm_enable_mmu
arm_enable_mmu:

	# save the pg table addr
	ldr r0, =_boot_pd
	mov r2, r0
	
	mov r0, #0

	# disable and invalidte the icache
	mrc p15, 0, r0, c1, c0, 0
	bic r1, r0, #(1 << 12)
	mcr p15, 0, r1, c1, c0, 0
	
	# clear entire d-cache
	mov r1, #0
	nop
	mcr p15, 0, r1, c7, c14, 0
	nop
	nop
	
	# draine write-buffer
	mcr p15, 0, r1, c7, c10, 0

	# clean icache
	mcr p15, 0, r1, c7, c5, 0

	# setup client to only have access to domain 0
	# and DACR
	mov r1, #1
	mcr p15, 0, r1, c3, c0, 0

	# set page table
	mcr p15, 0, r2, c2, c0, 0

	mov r1, #0
	mcr p15, 0, r1, c8, c7, 0
	mcr p15, 0, r1, c7, c5, 4
	mcr p15, 0, r1, c7, c5, 6

	# enable mmu and d/i cache
	mov r0, #0
	mrc p15, 0, r0, c1, c0, 0
	orr r0, r0, #1
	orr r0, r0, #(1 << 2)
	orr r0, r0, #(1 << 12)
	mcr p15, 0, r0, c1, c0, 0
	nop
	nop

	bx lr

#endif
