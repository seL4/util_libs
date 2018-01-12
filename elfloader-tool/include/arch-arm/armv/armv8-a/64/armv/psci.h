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

#pragma once


#define PSCI_SUCCESS                 0
#define PSCI_NOT_SUPPORTED          -1
#define PSCI_INVALID_PARAMETERS     -2
#define PSCI_DENIED                 -3
#define PSCI_ALREADY_ON             -4
#define PSCI_ON_PENDING             -5
#define PSCI_INTERNAL_FAILURE       -6
#define PSCI_NOT_PRESETN            -7
#define PSCI_DISABLED               -8
#define PSCI_INVALID_ADDRESS        -9



int psci_version(void);
int psci_cpu_suspend(int power_state, unsigned long entry_point,
                     unsigned long context_id);
/* this function does not return when successful */
int psci_cpu_off(void);
int psci_cpu_on(unsigned long target_cpu, unsigned long entry_point,
                unsigned long context_id);
int psci_system_reset(void);
