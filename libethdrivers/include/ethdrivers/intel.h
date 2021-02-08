/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <platsupport/io.h>
#include <ethdrivers/raw.h>

typedef struct ethif_intel_config {
    void *bar0;
    uint8_t prom_mode;
    size_t num_irqs;
    ps_irq_t irq_info[];
} ethif_intel_config_t;

/**
 * This function initialises the hardware and conforms to the ethif_driver_init
 * type in raw.h
 * @param[out] eth_driver   Ethernet driver structure to fill out
 * @param[in] io_ops        A structure containing os specific data and
 *                          functions.
 * @param[in] config        Pointer to a ethif_intel_config struct
 */
int ethif_e82580_init(struct eth_driver *eth_driver, ps_io_ops_t io_ops, void *config);

/**
 * This function initialises the hardware and conforms to the ethif_driver_init
 * type in raw.h
 * @param[out] eth_driver   Ethernet driver structure to fill out
 * @param[in] io_ops        A structure containing os specific data and
 *                          functions.
 * @param[in] config        Pointer to a ethif_intel_config struct
 */
int ethif_e82574_init(struct eth_driver *eth_driver, ps_io_ops_t io_ops, void *config);

