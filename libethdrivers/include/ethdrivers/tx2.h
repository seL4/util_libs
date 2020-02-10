/*
 * Copyright 2019, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */

#pragma once

#include <platsupport/io.h>
#include <ethdrivers/raw.h>

#define TX2_INT_POWER_ETHER_QOS 227
#define TX2_INT_COMMON_ETHER_QOS 226
#define TX2_INT_RX3_ETHER_QOS 225
#define TX2_INT_RX2_ETHER_QOS 224
#define TX2_INT_RX1_ETHER_QOS 223
#define TX2_INT_RX0_ETHER_QOS 222
#define TX2_INT_TX3_ETHER_QOS 221
#define TX2_INT_TX2_ETHER_QOS 220
#define TX2_INT_TX1_ETHER_QOS 219
#define TX2_INT_TX0_ETHER_QOS 218

/**
 * This function initialises the hardware and conforms to the ethif_driver_init
 * type in raw.h
 * @param[out] eth_driver   Ethernet driver structure to fill out
 * @param[in] io_ops        A structure containing os specific data and
 *                          functions.
 * @param[in] config        Platform Specific configuration data
 */
int ethif_tx2_init(struct eth_driver *eth_driver, ps_io_ops_t io_ops, void *config);

