/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <platsupport/io.h>
#include <ethdrivers/raw.h>

/**
 * This function initialises the hardware and conforms to the ethif_driver_init
 * type in raw.h
 * @param[out] eth_driver   Ethernet driver structure to fill out
 * @param[in] io_ops        A structure containing os specific data and
 *                          functions.
 * @param[in] config        Platform Specific configuration data
 */
int ethif_odroidc2_init(struct eth_driver *eth_driver, ps_io_ops_t io_ops, void *config);
