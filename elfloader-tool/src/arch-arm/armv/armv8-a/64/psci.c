/*
 * Copyright 2018, Data61
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
#include <elfloader/gen_config.h>
#include <printf.h>

#define SMC64_FID_VER           0x84000000
#define SMC64_FID_CPU_SUSPEND   0xc4000001
#define SMC64_FID_CPU_OFF       0x84000002
#define SMC64_FID_CPU_ON        0xc4000003
#define SMC64_FID_SYSTEM_RESET  0x84000009


extern int psci_func(unsigned int id, unsigned long param1,
                     unsigned long param2, unsigned long param3);

int psci_version(void)
{
    int ver = psci_func(SMC64_FID_VER, 0, 0, 0);
    return ver;
}


int psci_cpu_suspend(int power_state, unsigned long entry_point,
                     unsigned long context_id)
{
    int ret = psci_func(SMC64_FID_CPU_SUSPEND, power_state, entry_point, context_id);
    return ret;
}

/* this function does not return when successful */
int psci_cpu_off(void)
{
    int ret = psci_func(SMC64_FID_CPU_OFF, 0, 0, 0);
    return ret;
}

int psci_cpu_on(unsigned long target_cpu, unsigned long entry_point,
                unsigned long context_id)
{
    int ret = psci_func(SMC64_FID_CPU_ON, target_cpu, entry_point, context_id);
    return ret;
}

int psci_system_reset(void)
{
    int ret = psci_func(SMC64_FID_SYSTEM_RESET, 0, 0, 0);
    return ret;
}

