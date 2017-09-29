/*
 * Copyright 2017, Data61
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

typedef struct ethif_virtio_pci_config {
    uint16_t io_base;
    void *mmio_base;
} ethif_virtio_pci_config_t;

/**
 * This function initialises the hardware and conforms to the ethif_driver_init
 * type in raw.h
 * @param[out] eth_driver   Ethernet driver structure to fill out
 * @param[in] io_ops        A structure containing os specific data and
 *                          functions.
 * @param[in] config        Pointer to a ethif_virtio_pci_config struct
 */
int ethif_virtio_pci_init(struct eth_driver *eth_driver, ps_io_ops_t io_ops, void *config);

