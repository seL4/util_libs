/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include "../stdint.h"
#include "../stdio.h"
#include "../cpuid.h"
#include "../elfloader.h"


#define BOOTCPU 0

#define EXYNOS5_SYSRAM        0x02020000
#define EXYNOS5_POWER         0x10040000

#define EXYNOS5_SYSRAM_NS     (EXYNOS5_SYSRAM + 0x53000)
#define EXYNOS5_POWER_CPU_CFG (EXYNOS5_POWER  +  0x2000)

#define CORE_LOCAL_PWR_EN     0x3

#define SMC_SHUTDOWN          -7

/**
 * This structure and its location is defined by U-Boot
 * It controls the boot behaviour of secondary cores.
 */
typedef volatile struct {
    uint32_t bypass[2];      /* 0x00 */
    uint32_t resume_addr;    /* 0x08 */
    uint32_t resume_flag;    /* 0x0C */
    uint32_t res0[3];        /* 0x10 */
    uint32_t cpu1_boot_reg;  /* 0x1C */
    uint32_t direct_go_flag; /* 0x20 */
    uint32_t direct_go_addr; /* 0x24 */
    uint32_t res1[2];        /* 0x28 */
    /* 0x30 */
} nscode_t;


struct cso {
    uint32_t config;
    uint32_t status;
    uint32_t option;
    uint32_t res[5];
};

typedef volatile struct cpu_cfg {
    struct cso core;
    struct cso dis_irq_local;
    struct cso dis_irq_central;
    struct cso res[1];
} cpu_cfg_t;


/* U-Boot control */
nscode_t* _nsscode  = (nscode_t*)EXYNOS5_SYSRAM_NS;

/* CPU configuration */
cpu_cfg_t *_cpu_cfg = (cpu_cfg_t*)EXYNOS5_POWER_CPU_CFG;


void boot_cpu(int cpu, uintptr_t entry)
{
    /* Setup the CPU's entry point */
    _nsscode->cpu1_boot_reg = entry;
    asm volatile("dmb");
    /* Spin up the CPU */
    _cpu_cfg[cpu].core.config = CORE_LOCAL_PWR_EN;
}

void platform_init(void)
{
    if (get_cortex_a_part() == 7) {
        printf("\nSwitching CPU...\n");
        boot_cpu(BOOTCPU, (uintptr_t)_start);
        /* Shutdown */
        smc(SMC_SHUTDOWN, 0, 0, 0);
        while (1) {
            cpu_idle();
        }
    } else {
        _nsscode->cpu1_boot_reg = 0;
    }
}


