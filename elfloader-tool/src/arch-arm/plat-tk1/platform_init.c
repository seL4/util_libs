/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include <autoconf.h>
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


#define MONITOR_MODE        (0x16)
#define SUPERVISOR_MODE     (0x13)
#define HYPERVISOR_MODE     (0x1a)

/* if seL4 is used a hypervior, we should enable both HCe and SCE bits.
 * the secure monitor exception handler does very limited things, so
 * we let the seL4 handle interrupts/exceptions.
 */

#if defined(CONFIG_ARM_MONITOR_MODE) || defined(CONFIG_ARM_MONITOR_HOOK)

void
arm_halt(void)
{
    while (1) {
        asm volatile ("wfe");
    }
}

/* steal the last 1 MiB physical memory for monitor mode */

#define MON_PA_START        (0x80000000 + 0x27f00000)
#define MON_PA_SIZE         (1 << 20)  
#define MON_PA_END          (MON_PA_START + MON_PA_SIZE)
#define MON_PA_STACK        (MON_PA_END - 0x10)  
#define MON_VECTOR_START    (MON_PA_START)
#define MON_HANDLER_START   (MON_PA_START + 0x10000)
#define LOADED_OFFSET       0x82000000

static int mon_init_done = 0;

static void
switch_to_mon_mode(void)
{
    if (mon_init_done == 0) {
        /* first need to make sure that we are in secure world */
        uint32_t scr = 0;

        /* read the secure configuration register, note if we are
         * in nonsecure world, the instruction fails.
         */

        asm volatile ("mrc p15, 0, %0, c1, c1, 0":"=r"(scr));
    
        if (scr & BIT(SCR_NS)) {
            printf("In nonsecure world, you should never see this!\n");
            arm_halt();
        }

        /* enable hyper call */ 
        scr = BIT(SCR_HCE);

        asm volatile ("mcr p15, 0, %0, c1, c1, 0"::"r"(scr));
    
        /* now switch to secure monitor mode */
        asm volatile ("cps %0\n\t"
                      "isb\n"
                     ::"I"(MONITOR_MODE));
        /* set up stack */
        asm volatile ("mov sp, %0" :: "r"(MON_PA_STACK));
        mon_init_done = 1;
        printf("ELF loader: monitor mode init done\n");
    }
}

#endif


#ifdef CONFIG_ARM_MONITOR_HOOK

extern void arm_monitor_vector(void);
extern void arm_monitor_vector_end(void);
extern void *memcpy(void *dest, void *src, size_t n);
extern char _bootstack_top[1];

static void
install_monitor_hook(void)
{
    uint32_t size = arm_monitor_vector_end - arm_monitor_vector;
    /* switch monitor mode if not already */
    switch_to_mon_mode(); 
    printf("Copy monitor mode vector from %x to %x size %x\n", (arm_monitor_vector), MON_VECTOR_START, size);
    memcpy((void *)MON_VECTOR_START, (void *)(arm_monitor_vector), size);

    asm volatile ("mcr p15, 0, %0, c12, c0, 1"::"r"(MON_VECTOR_START));
}

#endif

#ifdef CONFIG_ARM_HYPERVISOR_MODE
static void
switch_to_hyp_mode(void)
{
    uint32_t scr = 0;
    asm volatile ("mov r0, sp");
    asm volatile ("mrc p15, 0, %0, c1, c1, 0":"=r"(scr));
    scr |= BIT(SCR_NS);
    asm volatile ("mcr p15, 0, %0, c1, c1, 0"::"r"(scr));
    asm volatile ("cps %0\n\t"
                  "isb\n"
                  ::"I"(HYPERVISOR_MODE));
    asm volatile ("mov sp, r0");
    asm volatile ("mrs %0, cpsr":"=r"(scr));
    printf("Load seL4 in nonsecure HYP mode %x", scr);
}
#endif

#ifdef CONFIG_ARM_NS_SUPERVISOR_MODE

static void
switch_to_ns_svc_mode(void)
{
    uint32_t scr = 0;

    asm volatile ("cps %0\n\t"
                  "isb\n"
                  ::"I"(SUPERVISOR_MODE));

    asm volatile ("mov r0, sp");
    asm volatile ("mrc p15, 0, %0, c1, c1, 0":"=r"(scr));
    scr |= BIT(SCR_NS);
    asm volatile ("mcr p15, 0, %0, c1, c1, 0"::"r"(scr));
    asm volatile ("mov sp, r0");
    
    printf("Load seL4 in nonsecure SVC mode\n");
}
#endif

extern void arm_monitor_vector(void);
void platform_init(void)
{
    /* Nothing to do here */
    /* not really!        */
#ifdef CONFIG_ARM_MONITOR_HOOK
    install_monitor_hook();
#endif

#ifdef CONFIG_ARM_NS_SUPERVISOR_MODE
    switch_to_ns_svc_mode();
#endif

#ifdef CONFIG_ARM_HYPERVISOR_MODE
    switch_to_hyp_mode();
#endif

#ifdef CONFIG_ARM_MONITOR_MODE
    switch_to_mon_mode();
#endif

}


