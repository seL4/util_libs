/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * (C) Copyright 2009 Ilya Yanok, Emcraft Systems Ltd <yanok@emcraft.com>
 * (C) Copyright 2008,2009 Eric Jarrige <eric.jarrige@armadeus.org>
 * (C) Copyright 2008 Armadeus Systems nc
 * (C) Copyright 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 * (C) Copyright 2007 Pengutronix, Juergen Beisert <j.beisert@pengutronix.de>
 * (C) Copyright 2018, NXP
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include "common.h"
#include "miiphy.h"
#include "fec_mxc.h"

#include "imx-regs.h"
#include "../io.h"

#include "micrel.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "../enet.h"
#include "../ocotp_ctrl.h"
/*
 * Timeout the transfer after 5 mS. This is usually a bit more, since
 * the code in the tightloops this timeout is used in adds some overhead.
 */
#define FEC_XFER_TIMEOUT    5000

#ifndef CONFIG_MII
#error "CONFIG_MII has to be defined!"
#endif

#ifndef CONFIG_FEC_XCV_TYPE
#define CONFIG_FEC_XCV_TYPE MII100
#endif

#undef DEBUG

int fec_phy_read(struct mii_dev *bus, int phyAddr, int dev_addr, int regAddr)
{
    return enet_mdio_read((struct enet *)bus->priv, phyAddr, regAddr);
}

int fec_phy_write(struct mii_dev *bus, int phyAddr, int dev_addr, int regAddr,
                  uint16_t data)
{
    return enet_mdio_write((struct enet *)bus->priv, phyAddr, regAddr, data);
}

/**
 * Halt the FEC engine
 * @param[in] dev Our device to handle
 */
void fec_halt(struct eth_device *dev)
{
#if 0
    struct fec_priv *fec = (struct fec_priv *)dev->priv;
    int counter = 0xffff;
    /* issue graceful stop command to the FEC transmitter if necessary */
    writel(FEC_TCNTRL_GTS | readl(&fec->eth->x_cntrl), &fec->eth->x_cntrl);
    /* wait for graceful stop to register */
    while ((counter--) && (!(readl(&fec->eth->ievent) & FEC_IEVENT_GRA))) {
        udelay(1);
    }
    writel(readl(&fec->eth->ecntrl) & ~FEC_ECNTRL_ETHER_EN, &fec->eth->ecntrl);
    fec->rbd_index = 0;
    fec->tbd_index = 0;
#else
    assert(!"unimplemented");
#endif
}
int fec_init(unsigned phy_mask, struct enet *enet)
{
    struct eth_device *edev;
    struct phy_device *phydev;
    struct mii_dev *bus;
    int ret = 0;
    struct eth_device _eth;
    /* create and fill edev struct */
    edev = &_eth;
    memset(edev, 0, sizeof(*edev));

    edev->priv = (void *)enet;
    edev->write_hwaddr = NULL;

    /* Allocate the mdio bus */
    bus = mdio_alloc();
    if (!bus) {
        return -1;
    }
    bus->read = fec_phy_read;
    bus->write = fec_phy_write;
    bus->priv = enet;
    strcpy(bus->name, edev->name);
    ret = mdio_register(bus);
    if (ret) {
        free(bus);
        return -1;
    }

    /****** Configure phy ******/
    phydev = phy_connect_by_mask(bus, phy_mask, edev, PHY_INTERFACE_MODE_RGMII);
    if (!phydev) {
        return -1;
    }

    if (config_set(CONFIG_PLAT_IMX8MQ_EVK)) {
        /* enable rgmii rxc skew and phy mode select to RGMII copper */
        phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x1f);
        phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x8);
        phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x05);
        phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x100);

        if (phydev->drv->config) {
            phydev->drv->config(phydev);
        }

        if (phydev->drv->startup) {
            phydev->drv->startup(phydev);
        }
    } else if (config_set(CONFIG_PLAT_IMX6)) {
        /* min rx data delay */
        ksz9021_phy_extended_write(phydev, MII_KSZ9021_EXT_RGMII_RX_DATA_SKEW, 0x0);
        /* min tx data delay */
        ksz9021_phy_extended_write(phydev, MII_KSZ9021_EXT_RGMII_TX_DATA_SKEW, 0x0);
        /* max rx/tx clock delay, min rx/tx control */
        ksz9021_phy_extended_write(phydev, MII_KSZ9021_EXT_RGMII_CLOCK_SKEW, 0xf0f0);
        ksz9021_config(phydev);

        /* Start up the PHY */
        ret = ksz9021_startup(phydev);
        if (ret) {
            printf("Could not initialize PHY %s\n", phydev->dev->name);
            return ret;
        }

    }

    printf("\n  * Link speed: %4i Mbps, ", phydev->speed);
    if (phydev->duplex == DUPLEX_FULL) {
        enet_set_speed(enet, phydev->speed, 1);
        printf("full-duplex *\n");
    } else {
        enet_set_speed(enet, phydev->speed, 0);
        printf("half-duplex *\n");
    }

    udelay(100000);
    return 0;
}
