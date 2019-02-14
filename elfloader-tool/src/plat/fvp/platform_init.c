/*
 * Copyright 2019, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */

void platform_init(void)
{
    /* On FVP, The MPIDR_EL1 changes to 0x80000000 after
     * MMU is enabled for unknown reason. We save the
     * MPIDR_EL1 in TPIDR_EL0 before switching the MMU, and
     * the correct MPIDR_EL1 can be picked up by the kernel
     * from TPIDR_EL0.
     * The same operation is performed in smp_head.S as well
     * when SMP is enabled.
     */
    asm volatile ("mrs x0, mpidr_el1\n"
                  "msr tpidr_el0, x0\n"
                  ::: "x0");
}
