/*
 * Micrel PHY drivers
 *
 * Copyright 2010-2011 Freescale Semiconductor, Inc.
 * author Andy Fleming
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include "config.h"
#include "common.h"
#include "micrel.h"
#include "phy.h"

#define CONFIG_PHY_MICREL_KSZ9021

static struct phy_driver KSZ804_driver = {
    .name = "Micrel KSZ804",
    .uid = 0x221510,
    .mask = 0xfffff0,
    .features = PHY_BASIC_FEATURES,
    .config = &genphy_config,
    .startup = &genphy_startup,
    .shutdown = &genphy_shutdown,
};

/* ksz9021 PHY Registers */
#define MII_KSZ9021_EXTENDED_CTRL   0x0b
#define MII_KSZ9021_EXTENDED_DATAW  0x0c
#define MII_KSZ9021_EXTENDED_DATAR  0x0d
#define MII_KSZ9021_PHY_CTL     0x1f
#define MIIM_KSZ9021_PHYCTL_1000    (BIT(6))
#define MIIM_KSZ9021_PHYCTL_100     (BIT(5))
#define MIIM_KSZ9021_PHYCTL_10      (BIT(4))
#define MIIM_KSZ9021_PHYCTL_DUPLEX  (BIT(3))

#define CTRL1000_PREFER_MASTER      (BIT(10))
#define CTRL1000_CONFIG_MASTER      (BIT(11))
#define CTRL1000_MANUAL_CONFIG      (BIT(12))

int ksz9021_phy_extended_write(struct phy_device *phydev, int regnum, uint16_t val)
{
    /* extended registers */
    phy_write(phydev, MDIO_DEVAD_NONE, MII_KSZ9021_EXTENDED_CTRL, regnum | 0x8000);
    return phy_write(phydev, MDIO_DEVAD_NONE, MII_KSZ9021_EXTENDED_DATAW, val);
}

int ksz9021_phy_extended_read(struct phy_device *phydev, int regnum)
{
    /* extended registers */
    phy_write(phydev, MDIO_DEVAD_NONE, MII_KSZ9021_EXTENDED_CTRL, regnum);
    return phy_read(phydev, MDIO_DEVAD_NONE, MII_KSZ9021_EXTENDED_DATAR);
}

/* Micrel ksz9021 */
int ksz9021_config(struct phy_device *phydev)
{
    unsigned ctrl1000 = 0;
    const unsigned master = CTRL1000_PREFER_MASTER |
                            CTRL1000_CONFIG_MASTER | CTRL1000_MANUAL_CONFIG;
    unsigned features = phydev->drv->features;

    /* force master mode for 1000BaseT due to chip errata */
    if (features & SUPPORTED_1000baseT_Half) {
        ctrl1000 |= ADVERTISE_1000HALF | master;
    }
    if (features & SUPPORTED_1000baseT_Full) {
        ctrl1000 |= ADVERTISE_1000FULL | master;
    }
    phydev->advertising = phydev->supported = features;
    phy_write(phydev, MDIO_DEVAD_NONE, MII_CTRL1000, ctrl1000);
    genphy_config_aneg(phydev);
    genphy_restart_aneg(phydev);
    return 0;
}

void print_phyregs(struct phy_device *phydev)
{
    int i;
    printf("\n\nPHY config\n");
    printf("\n\nIEEE\n");
    for (i = 0; i < 16; i++) {
        uint16_t val = phy_read(phydev, MDIO_DEVAD_NONE, i);
        printf("%3d | 0x%04x\n", i, val);
    }

    printf("vendor\n");
    for (i = 16; i < 32; i++) {
        uint16_t val = phy_read(phydev, MDIO_DEVAD_NONE, i);
        printf("%3d | 0x%04x\n", i, val);
    }

    printf("extended\n");
    for (i = 257; i < 264; i++) {
        uint16_t val = ksz9021_phy_extended_read(phydev, i);
        printf("%3d | 0x%04x\n", i, val);
    }
    printf("\n\n");
}

int ksz9021_startup(struct phy_device *phydev)
{
    unsigned phy_ctl;
    genphy_update_link(phydev);
    phy_ctl = phy_read(phydev, MDIO_DEVAD_NONE, MII_KSZ9021_PHY_CTL);

    if (phy_ctl & MIIM_KSZ9021_PHYCTL_DUPLEX) {
        phydev->duplex = DUPLEX_FULL;
    } else {
        phydev->duplex = DUPLEX_HALF;
    }
    if (phy_ctl & MIIM_KSZ9021_PHYCTL_1000) {
        phydev->speed = SPEED_1000;
    } else if (phy_ctl & MIIM_KSZ9021_PHYCTL_100) {
        phydev->speed = SPEED_100;
    } else if (phy_ctl & MIIM_KSZ9021_PHYCTL_10) {
        phydev->speed = SPEED_10;
    }
    return 0;
}

static struct phy_driver ksz9021_driver = {
    .name = "Micrel ksz9021",
    .uid  = 0x221610,
    .mask = 0xfffff0,
    .features = PHY_GBIT_FEATURES,
    .config = &ksz9021_config,
    .startup = &ksz9021_startup,
    .shutdown = &genphy_shutdown,
};

int phy_micrel_init(void)
{
    phy_register(&KSZ804_driver);
    phy_register(&ksz9021_driver);
    return 0;
}
