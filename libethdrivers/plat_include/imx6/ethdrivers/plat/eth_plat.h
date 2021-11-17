/*
 * Copyright 2017, DORNERWORKS
 * Copyright 2020, HENSOLDT Cyber GmbH
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <ethdrivers/imx6.h>
#include <utils/attribute.h>
#include <ethdrivers/raw.h>

struct enet *get_enet_from_driver(struct eth_driver *driver);

enum {
    NIC_CONFIG_FORCE_MAC        = 1u << 0, /**< Use MAC from config (if not 0) */
    NIC_CONFIG_PROMISCUOUS_MODE = 1u << 1, /**< Enable promiscuous mode */
    NIC_CONFIG_DROP_FRAME_CRC   = 1u << 2, /**< Drop ethernet frame CRC */
    NIC_CONFIG_NO_CLOCK_SYS     = 1u << 3, /**< Don't use clock_sys interface */
} nic_config_flags_t;

typedef int (*sync_func_t)(void);
typedef int (*mdio_read_func_t)(uint16_t reg);
typedef int (*mdio_write_func_t)(uint16_t reg, uint16_t data);

typedef struct {
    unsigned int phy_address; /**< Address of the PHY chip, 0 for auto-detect */
    unsigned int id; /**< ID of the enet */
    unsigned int flags;
    uint64_t mac; /**< MAC address, 0x0000aabbccddeeff, ignored if 0 */
    struct {
        sync_func_t sync; /**< blocks until primary NIC init is finished */
        mdio_read_func_t mdio_read; /**< primary NIC mdio_read rpc function */
        mdio_write_func_t mdio_write; /**< primary NIC mdio_write rpc function */
    } funcs;
} nic_config_t;

/* this function can be provided so the driver can fetch a configuration */
WEAK const nic_config_t *get_nic_configuration(void);
