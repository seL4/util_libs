/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#ifndef __PLAT_SUPPORT_IMX31_EPIT_CONSTANTS_H
#define __PLAT_SUPPORT_IMX31_EPIT_CONSTANTS_H

/**
 * Address at which the EPIT2 should be in the initial
 * device space. Map a frame, uncached, to this address
 * to access the gpt. Pass the virtual address of
 * this frame onto timer_init to use the gpt.
 */
#define EPIT1_DEVICE_PADDR 0x53f94000
#define EPIT1_INTERRUPT 28

#define EPIT2_DEVICE_PADDR 0x53f98000
#define EPIT2_INTERRUPT 27

#endif /* __PLAT_SUPPORT_IMX31_EPIT_CONSTANTS_H */
