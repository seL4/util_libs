/*
 * (C) Copyright 2009 Ilya Yanok, Emcraft Systems Ltd <yanok@emcraft.com>
 * (C) Copyright 2008,2009 Eric Jarrige <eric.jarrige@armadeus.org>
 * (C) Copyright 2008 Armadeus Systems nc
 * (C) Copyright 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 * (C) Copyright 2007 Pengutronix, Juergen Beisert <j.beisert@pengutronix.de>
 * (C) Copyright 2018, NXP
 * (C) Copyright 2020, HENSOLDT Cyber GmbH
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
#include "micrel.h"
#include "../io.h"
#include "../enet.h"
#include "../ocotp_ctrl.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#undef DEBUG

static int fec_phy_read(struct mii_dev *bus, int phyAddr, UNUSED int dev_addr,
                        int regAddr)
{
    struct enet *enet = (struct enet *)bus->priv;
    assert(enet);
    return enet_mdio_read(enet, phyAddr, regAddr);
}

static int fec_phy_write(struct mii_dev *bus, int phyAddr, UNUSED int dev_addr,
                         int regAddr, uint16_t data)
{
    struct enet *enet = (struct enet *)bus->priv;
    assert(enet);
    return enet_mdio_write(enet, phyAddr, regAddr, data);
}

/*
 * Halting the FEC engine could can be done with:
 *   issue graceful stop command
 *      writel(FEC_TCNTRL_GTS | readl(eth->x_cntrl), eth->x_cntrl);
 *   wait (with timeout) for graceful stop to register
 *      counter = 0xffff
 *      while ((counter--) && (!(readl(eth->ievent) & FEC_IEVENT_GRA))) {
 *         udelay(1);
 *      }
 *   issue disable command and clerar FEV
 *      writel(readl(eth->ecntrl) & ~FEC_ECNTRL_ETHER_EN, eth->ecntrl);
 *      fec->rbd_index = 0;
 *      fec->tbd_index = 0;
 */

struct phy_device *fec_init(unsigned int phy_mask, struct enet *enet)
{
    int ret;

    /* Allocate the mdio bus */
    struct mii_dev *bus = mdio_alloc();
    if (!bus) {
        ZF_LOGE("Could not allocate MDIO");
        return NULL;
    }
    strncpy(bus->name, "MDIO", sizeof(bus->name));
    bus->read = fec_phy_read;
    bus->write = fec_phy_write;
    bus->priv = enet;
    ret = mdio_register(bus);
    if (ret) {
        ZF_LOGE("Could not register MDIO, code %d", ret);
        free(bus);
        return NULL;
    }

    /* Configure PHY with a dummy edev, because it is never used. All that
     * happens is that the name is printed */
    static struct eth_device dummy_eth_dev = { .name = "DUMMY-EDEV" };
    struct phy_device *phydev = phy_connect_by_mask(
                                    bus,
                                    phy_mask,
                                    &dummy_eth_dev,
                                    PHY_INTERFACE_MODE_RGMII);
    if (!phydev) {
        ZF_LOGE("Could not connect to PHY");
        return NULL;
    }

#if defined(CONFIG_PLAT_IMX8MQ_EVK)

    /* enable rgmii rxc skew and phy mode select to RGMII copper */
    phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x1f);
    phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x8);
    phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x05);
    phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x100);

#elif defined(CONFIG_PLAT_IMX6)

    /* min rx data delay */
    ksz9021_phy_extended_write(phydev, MII_KSZ9021_EXT_RGMII_RX_DATA_SKEW, 0x0);
    /* min tx data delay */
    ksz9021_phy_extended_write(phydev, MII_KSZ9021_EXT_RGMII_TX_DATA_SKEW, 0x0);
    /* max rx/tx clock delay, min rx/tx control */
    ksz9021_phy_extended_write(phydev, MII_KSZ9021_EXT_RGMII_CLOCK_SKEW, 0xf0f0);

#else
#error "unsupported platform"
#endif

    if (phydev->drv->config) {
        ret = phydev->drv->config(phydev);
        if (ret) {
            ZF_LOGE("Could not configure PHY '%s', code %d",
                    phydev->dev->name, ret);
            return NULL;
        }
    }

    if (phydev->drv->startup) {
        ret = phydev->drv->startup(phydev);
        if (ret) {
            ZF_LOGE("Could not init PHY '%s', code %d",
                    phydev->dev->name, ret);
            return NULL;
        }
    }

    return phydev;
}
