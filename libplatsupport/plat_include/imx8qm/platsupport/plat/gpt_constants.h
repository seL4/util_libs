/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * Copyright 2017, DornerWorks
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_DORNERWORKS_BSD)
 */

#pragma once

/**
 * Address at which the GPT should be in the initial
 * device space. Map a frame, uncached, to this address
 * to access the gpt. Pass the virtual address of
 * this frame onto timer_init to use the gpt.
 */
#define GPT0_DEVICE_PADDR 0x5D140000
#define GPT1_DEVICE_PADDR 0x5D150000
#define GPT2_DEVICE_PADDR 0x5D160000
#define GPT3_DEVICE_PADDR 0x5D170000
#define GPT4_DEVICE_PADDR 0x5D180000

#define GPT0_INTERRUPT 112
#define GPT1_INTERRUPT 113
#define GPT2_INTERRUPT 114
#define GPT3_INTERRUPT 115
#define GPT4_INTERRUPT 116


#define GPT_PRESCALER (1)
