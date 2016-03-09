/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include "../elfloader.h"
#include "../stdio.h"

/* non-secure bit: 0 secure; 1 nonsecure */
#define SCR_NS      (0)

/* controls which mode takes IRQ exceptions: 0 IRQ mode; 1 monitor mode */
#define SCR_IRQ     (1)

/* FIQ mode control */
#define SCR_FIQ     (2)

/* external abort handler. 0 abort mode; 1 monitor mode */
#define SCR_EA      (3)

/* CPSR.F can be modified in nonsecure mode */
#define SCR_FW      (4)

/* CPSR.A can be modified in nonsecure mode */
#define SCR_AW      (5)

/* not early terminination. not implmented */
#define SCR_NET     (6)

/* secure monitor call disabled: 0 smc executes in nonsecure state; 
 * 1 undefined instruction in nonsecure state
 */
#define SCR_SCD     (7)

/* hyp call enable: 0 hvc instruction is undefined in nonsecure pl1 mode
 *                    and unpredictable in hyp mode
 *                  1 hvc is enabled in nonsecure pl1.
 */
#define SCR_HCE     (8)

/* secure instruction fetch. when in secure state, the bit disables 
 * instruction fetches from non-secure memory */
#define SCR_SIF     (9)       


/* if seL4 is used a hypervior, we should enable both HCe and SCE bits.
 * the secure monitor exception handler does very limited things, so
 * we let the seL4 handle interrupts/exceptions.
 */


static void
halt(void)
{
    while (1) {
        asm volatile ("wfe");
    }
}

#define SECURE_STATE    (0x16)
static char monitor_stack[4096];

static void
switch_to_monitor(void)
{
    /* first need to make sure that we are in secure world */
    uint32_t scr = 0;
    /* read the secure configuration register, note if we are
     * in nonsecure world, the instruction fails.
     */
    asm volatile ("mrc p15, 0, %0, c1, c1, 0":"=r"(scr));
    
    if (scr & BIT(SCR_NS)) {
        printf("In nonsecure world, you should never see this!\n");
        halt();
    }
    /* enable hyper call */ 
    scr = BIT(SCR_HCE);

    asm volatile ("mcr p15, 0, %0, c1, c1, 0"::"r"(scr));
    
    /* now switch to secure monitor state */
    asm volatile ("cps %0\n\t"
                  "isb\n"
                 ::"I"(SECURE_STATE));
    /* set up stack */
    asm volatile ("mov sp, %0"::"r"(monitor_stack));
    printf("switched to monitor mode\n");
}

void platform_init(void)
{
    /* Nothing to do here */

    switch_to_monitor();
}


