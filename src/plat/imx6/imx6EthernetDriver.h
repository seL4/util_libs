/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#ifndef __IMX6_ETH_DRIVER_H
#define __IMX6_ETH_DRIVER_H

struct desc_data {
    uint32_t tx_phys;
    uint32_t tx_bufsize;
    uint32_t rx_phys;
    uint32_t rx_bufsize;
};

/**
 * Returns HW relevant data for descriptors 
 * @param[in] desc  A reference to descriptors.
 * @retun           Physical address of the rings and their associated 
 *                  buffer sizes.
 */
struct desc_data desc_get_ringdata(struct desc *desc);
#endif
