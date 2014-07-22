/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#ifndef __ETHIF_IMX6_DRIVER_H
#define __ETHIF_IMX6_DRIVER_H

#include <platsupport/io.h>

/**
 * This function initialises the hardware
 * @param[in] dev_id    The device id of the driver to initialise.
 *                      Ignored on platforms that have only 1 device
 * @param[in] io_ops    A structure containing os specific data and
 *                      functions. 
 * @return              A reference to the ethernet drivers state.
 */
struct eth_driver* ethif_imx6_init(int dev_id, ps_io_ops_t io_ops);

#endif
