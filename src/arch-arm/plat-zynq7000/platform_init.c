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

#define MPCORE_PRIV               0xF8F00000

#define SCU_BASE                  (MPCORE_PRIV + 0x0)
#define SCU_CTRL_OFFSET           0x000
#define SCU_CTRL_EN               BIT(0)
#define SCU_CTRL_ADDRFILT_EN      BIT(1)
#define SCU_FILTADDR_START_OFFSET 0x040
#define SCU_FILTADDR_END_OFFSET   0x044

#define SLCR_BASE                 0xF8000000
#define SLCR_OCM_CFG_OFFSET       0x910
#define SLCR_OCM_CFG_RAMHI(x)     BIT(x)
#define SLCR_OCM_CFG_RAMHI_ALL    ( SLCR_OCM_CFG_RAMHI(0) \
                                  | SLCR_OCM_CFG_RAMHI(1) \
                                  | SLCR_OCM_CFG_RAMHI(2) \
                                  | SLCR_OCM_CFG_RAMHI(3) )

#define REG(a) *(volatile uint32_t*)(a)

#define SCU(o)  REG(SCU_BASE + SCU_##o##_OFFSET)
#define SLCR(o) REG(SLCR_BASE + SLCR_##o##_OFFSET)

/* Remaps the OCM and ensures DDR is accessible at 0x00000000 */
void remap_ram(void)
{
    /* SCU - translate from 0x00000000 - 0xffe0000 */
    SCU(FILTADDR_START) = 0x00000000;
    SCU(FILTADDR_END) = 0xFFE00000;
    SCU(CTRL) |= (SCU_CTRL_EN | SCU_CTRL_ADDRFILT_EN);
    /* OCM - Remap to high address */
    SLCR(OCM_CFG) |= SLCR_OCM_CFG_RAMHI_ALL;
}

void platform_init(void)
{
    remap_ram();
}

