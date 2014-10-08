/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include <platsupport/io.h>

/**
 * This function initialises the hardware
 * @param[in] dev_id        The device id of the driver to initialise.
 *                          Ignored on platforms that have only 1 device
 * @param[in] io_ops        A structure containing os specific data and
 *                          functions.
 * @param[in] bar0          Where pci bar0 has been mapped into our vspace
 * @return                  A reference to the ethernet drivers state.
 */
struct eth_driver*
ethif_e82580_init(int dev_id, ps_io_ops_t io_ops, void *bar0);
