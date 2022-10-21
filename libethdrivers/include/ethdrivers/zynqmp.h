/*
 * Copyright 2017, DornerWorks, Ltd.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <platsupport/io.h>
#include <ethdrivers/raw.h>

#define ZYNQMP_INTERRUPT_GEM3 95

/**
 * This function initialises the hardware and conforms to the ethif_driver_init
 * type in raw.h
 * @param[out] eth_driver   Ethernet driver structure to fill out
 * @param[in] io_ops        A structure containing os specific data and
 *                          functions.
 * @param[in] config        Platform Specific configuration data
 */
int ethif_zynqmp_init(struct eth_driver *eth_driver, ps_io_ops_t io_ops, void *config);
