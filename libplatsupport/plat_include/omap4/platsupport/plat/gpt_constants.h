/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __PLAT_SUPPORT_OMAP4_H
#define __PLAT_SUPPORT_OMAP4_H

/**
 * Address at which the GPT should be in the initial
 * device space. Map a frame, uncached, to this address
 * to access the gpt. Pass the virtual address of
 * this frame onto timer_init to use the gpt.
 */
#define GPT1_DEVICE_PADDR 0x4A318000
#define GPT1_INTERRUPT 37

#define GPT2_DEVICE_PADDR 0x48032000
#define GPT2_INTERRUPT 38

#define GPT3_DEVICE_PADDR 0x48034000
#define GPT3_INTERRUPT 39

#define GPT4_DEVICE_PADDR 0x48036000
#define GPT4_INTERRUPT 40

#define GPT5_DEVICE_PADDR 0x49038000
#define GPT5_INTERRUPT 41

#define GPT6_DEVICE_PADDR 0x4903A000
#define GPT6_INTERRUPT 42

#define GPT7_DEVICE_PADDR 0x4903C000
#define GPT7_INTERRUPT 43

#define GPT8_DEVICE_PADDR 0x4903E000
#define GPT8_INTERRUPT 44

#define GPT9_DEVICE_PADDR 0x4803E000
#define GPT9_INTERRUPT 45

#define GPT10_DEVICE_PADDR 0x48086000
#define GPT10_INTERRUPT 46

#define GPT11_DEVICE_PADDR 0x48088000
#define GPT11_INTERRUPT 47

#define CLK_FREQ 32000 /* TODO Check this is is correct */

static void *paddrs[] = {(void *) GPT1_DEVICE_PADDR,
                         (void *) GPT2_DEVICE_PADDR,
                         (void *) GPT3_DEVICE_PADDR,
                         (void *) GPT4_DEVICE_PADDR,
                         (void *) GPT5_DEVICE_PADDR,
                         (void *) GPT6_DEVICE_PADDR,
                         (void *) GPT7_DEVICE_PADDR,
                         (void *) GPT8_DEVICE_PADDR,
                         (void *) GPT9_DEVICE_PADDR,
                         (void *) GPT10_DEVICE_PADDR,
                         (void *) GPT11_DEVICE_PADDR
                        };

static inline void* omap_get_gpt_paddr(int n)
{
    return paddrs[n];
}

static inline int omap_get_gpt_irq(int n)
{
    return GPT1_INTERRUPT + (n - 1);
}

#endif /* __PLAT_SUPPORT_IMX31_H */
