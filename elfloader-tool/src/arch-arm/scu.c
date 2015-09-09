/*
 * @TAG(OTHER_GPL)
 */

/* Some code in here loosely derived from Linux */

/*
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 */

/* Driver for the ARM Snoop Control Unit (SCU) used on multicore systems */

#include <autoconf.h>

#ifdef CONFIG_SMP_ARM_MPCORE

#include <stdint.h>

#include "cpuid.h"
#include "elfloader.h"

#define SCU_CTRL 0
#define SCU_CONFIG 1

/* Enable the SCU */
void scu_enable(void *_scu_base)
{
    uint32_t scu_ctrl;
    volatile uint32_t *scu_base = (volatile uint32_t*)_scu_base;

#ifdef CONFIG_ARM_ERRATA_764369
    /* Cortex-A9 only */
    if ((read_cpuid_id() & 0xff0ffff0) == 0x410fc090) {
        scu_ctrl = scu_base[0x30 / 4];
        if (!(scu_ctrl & 1)) {
            scu_base[0x30 / 4] = scu_ctrl | 0x1;
        }
    }
#endif

    scu_ctrl = scu_base[SCU_CTRL];
    /* already enabled? */
    if (scu_ctrl & 1) {
        return;
    }

    scu_ctrl |= 1;
    scu_base[SCU_CTRL] = scu_ctrl;

    /*
     * Ensure that the data accessed by CPU0 before the SCU was
     * initialised is visible to the other CPUs.
     */
    flush_dcache();
}

/*
 * Get the number of CPU cores from the SCU configuration
 */
unsigned int scu_get_core_count(void *_scu_base)
{
    volatile uint32_t *scu_base = (volatile uint32_t*)_scu_base;
    unsigned int ncores = (unsigned int)scu_base[SCU_CONFIG];
    return (ncores & 0x03) + 1;
}

#endif
