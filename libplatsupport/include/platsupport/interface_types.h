/*
 * Copyright 2020, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#pragma once

typedef enum ps_interface_type {
    PS_NULL_INTERFACE,
    /* This ID represents the TX2 BPMP interface in libtx2bpmp inside the project_libs repository */
    TX2_BPMP_INTERFACE,
    /* These IDs represent the respective interfaces inside libplatsupport */
    PS_GPIO_INTERFACE,
    PS_CLOCK_INTERFACE,
    PS_RESET_INTERFACE
    /* There are plans to add more interface types, but we need to first define
     * the interfaces. */
} ps_interface_type_t;
