/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include <types.h>
#include <scu.h>

void flush_dcache();

void omap_write_auxcoreboot_addr(void *);
void omap_write_auxcoreboot0(uint32_t, uint32_t);
void omap_non_boot(void);

#ifdef CONFIG_SMP_ARM_MPCORE
void init_cpus()
{
    scu_enable((void *)0x48240000);
    omap_write_auxcoreboot_addr(omap_non_boot);
    flush_dcache();
    asm("dsb");
    asm("sev":::"memory");
    omap_write_auxcoreboot0(0x200, 0xfffffdff);
}
#endif
