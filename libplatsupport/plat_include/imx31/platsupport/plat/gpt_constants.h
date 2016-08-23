/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __PLAT_SUPPORT_IMX31_GPT_CONSTANTS_H
#define __PLAT_SUPPORT_IMX31_GPT_CONSTANTS_H

/**
 * Address at which the GPT should be in the initial
 * device space. Map a frame, uncached, to this address
 * to access the gpt. Pass the virtual address of
 * this frame onto timer_init to use the gpt.
 */
#define GPT1_DEVICE_PADDR 0x53f90000
#define GPT1_INTERRUPT 29

#endif /* __PLAT_SUPPORT_IMX31_GPT_CONSTANTS_H */
