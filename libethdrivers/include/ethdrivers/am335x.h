/*
 * Copyright 2016, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(D61_GPL)
 */

#ifndef __ETHIF_AM335X_DRIVER_H
#define __ETHIF_AM335X_DRIVER_H

#include <platsupport/io.h>
#include <ethdrivers/raw.h>

/**
 * This function initialises the hardware and conforms to the ethif_driver_init
 * type in raw.h
 * @param[out] eth_driver   Ethernet driver structure to fill out
 * @param[in] io_ops        A structure containing os specific data and
 *                          functions.
 * @param[in] config        Unused, should be NULL
 */
int ethif_am335x_init(struct eth_driver *eth_driver, ps_io_ops_t io_ops, void *config);

#endif
