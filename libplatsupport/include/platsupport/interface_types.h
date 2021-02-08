/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

typedef enum ps_interface_type {
    PS_NULL_INTERFACE,
    /* This ID represents the TX2 BPMP interface in libtx2bpmp inside the project_libs repository */
    TX2_BPMP_INTERFACE,
    /* These IDs represent the respective interfaces inside libplatsupport */
    PS_GPIO_INTERFACE,
    PS_CLOCK_INTERFACE,
    PS_RESET_INTERFACE,
    /* There are plans to add more interface types, but we need to first define
     * the interfaces. */
    PS_ETHERNET_INTERFACE /* Interface is defined as a reference to struct eth_driver defined in
                             libethdrivers */
} ps_interface_type_t;
