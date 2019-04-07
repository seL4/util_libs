/*
 * Copyright 2017, Data61
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

#if CONFIG_MAX_NUM_NODES > 1

#include <printf.h>
#include <types.h>
#include <scu.h>
#include <abort.h>
#include <armv/machine.h>

#define BIT(x) (1U << (x))

#define CPU_JUMP_PTR              0xFFFFFFF0

/*
 * A9_CPU_RST_CTRL Register definitions.
 * See TRM B.28 System Level Control Registers (slcr)
 */
#define A9_CPU_RST_CTRL     0xF8000244
#define PERI_RST_BIT        8
#define A9_CLKSTOP1_BIT     5
#define A9_CLKSTOP0_BIT     4
#define A9_RST1_BIT         1
#define A9_RST0_BIT         0

#define REG(base) *((volatile uint32_t*)(((uint32_t)(base))))

/* See TRM 3.7 Application Processing Unit (APU) Reset */
static void zynq_stop_core1(void)
{
    REG(A9_CPU_RST_CTRL) |= BIT(A9_RST1_BIT);
    dsb();
    REG(A9_CPU_RST_CTRL) |= BIT(A9_CLKSTOP1_BIT);
    dsb();
}

static void zynq_start_core1(void)
{
    REG(A9_CPU_RST_CTRL) &= ~BIT(A9_RST1_BIT);
    dsb();
    REG(A9_CPU_RST_CTRL) &= ~BIT(A9_CLKSTOP1_BIT);
    dsb();
}

extern void non_boot_core(void);

static void *get_scu_base(void)
{
    void *scu;
    asm("mrc p15, 4, %0, c15, c0, 0" : "=r"(scu));
    return scu;
}

static void boot_cpus(void (*entry)(void))
{
    /* Zynq only has 2 cores, we only need to reset core 1 */
    zynq_stop_core1();
    *((volatile uint32_t *)CPU_JUMP_PTR) = (uint32_t) entry;
    zynq_start_core1();
    dsb();
    asm volatile("sev;");
}

void init_cpus(void)
{
    unsigned int num;
    void *scu = get_scu_base();

    num = scu_get_core_count(scu);
    /* Currently there is no support for choosing which CPUs to boot, however,
     * there are only 2 CPUs on the Zynq7000. Either we boot no additional CPUs
     * or we boot all additional CPUs.
     */

    if (num > CONFIG_MAX_NUM_NODES) {
        num = CONFIG_MAX_NUM_NODES;
    } else if (num < CONFIG_MAX_NUM_NODES) {
        printf("Error: Unsupported number of CPUs! This platform has %u CPUs, while static configuration provided is %u CPUs\n",
               num, CONFIG_MAX_NUM_NODES);
        abort();
    }

    printf("Bringing up %d other cpus\n", num - 1);
    if (num != 1) {
        boot_cpus(&non_boot_core);
    }
}

#endif /* CONFIG_MAX_NUM_NODES > 1 */
