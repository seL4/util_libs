/*
 * Copyright 2017, DornerWorks, Ltd.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef __ETHIF_ZYNQ7000_DRIVER_H
#define __ETHIF_ZYNQ7000_DRIVER_H

#include <platsupport/io.h>
#include <ethdrivers/raw.h>

#define ZYNQ7000_INTERRUPT_ETH0 54

/**
 * This function initialises the hardware and conforms to the ethif_driver_init
 * type in raw.h
 * @param[out] eth_driver   Ethernet driver structure to fill out
 * @param[in] io_ops        A structure containing os specific data and
 *                          functions.
 * @param[in] config        Platform Specific configuration data
 */
int ethif_zynq7000_init(struct eth_driver *eth_driver, ps_io_ops_t io_ops, void *config);

#endif
