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
#include "micrel.h"
#include "fec_mxc.h"
#include "../io.h"
#include "../enet.h"
#include "../ocotp_ctrl.h"
#include <utils/attribute.h>
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

int cb_phy_read(
    struct mii_dev *bus,
    UNUSED int phyAddr,
    UNUSED int dev_addr,
    int regAddr)
{
    nic_config_t *nic_config = (nic_config_t *)bus->priv;
    if ((!nic_config) || (!nic_config->funcs.mdio_read)) {
        ZF_LOGE("mdio_read() from nic_config not set");
        assert(nic_config); // we should never be here if nic_config is NULL
        return -1;
    }
    return nic_config->funcs.mdio_read(regAddr);
}

int cb_phy_write(
    struct mii_dev *bus,
    UNUSED int phyAddr,
    UNUSED int dev_addr,
    int regAddr,
    uint16_t data)
{
    nic_config_t *nic_config = (nic_config_t *)bus->priv;
    if ((!nic_config) || (!nic_config->funcs.mdio_write)) {
        ZF_LOGE("mdio_write() from nic_config not set");
        assert(nic_config); // we should never be here if nic_config is NULL
        return -1;
    }

    return nic_config->funcs.mdio_write(regAddr, data);
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

struct phy_device *fec_init(unsigned int phy_mask, struct enet *enet,
                            const nic_config_t *nic_config)
{
    int ret;

    /* Allocate the mdio bus */
    struct mii_dev *bus = mdio_alloc();
    if (!bus) {
        ZF_LOGE("Could not allocate MDIO");
        return NULL;
    }

#ifdef CONFIG_PLAT_IMX6SX
    // on the i.MX6 SoloX Nitrogen board, both PHYs are connected to enet1's
    // MDIO, while enet2's MDIO pins are used for other I/O purposes.
    strncpy(bus->name, "MDIO-ENET1", sizeof(bus->name));
#else
    strncpy(bus->name, "MDIO", sizeof(bus->name));
#endif

    /* if we don't have direct access to MDIO, use the callbacks from config */
    if (!enet && !nic_config) {
        ZF_LOGE("Neither ENET nor nic_config is set, can't access MDIO");
        free(bus);
        return NULL;
    }

    bus->priv = enet ? enet : (struct enet *)nic_config;
    bus->read = enet ? fec_phy_read : cb_phy_read;
    bus->write = enet ? fec_phy_write : cb_phy_write;

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

#elif defined(CONFIG_PLAT_SABRE)

    if (0x00221610 == (phydev->phy_id & 0xfffffff0)) { /* ignore silicon rev */
        /* min rx data delay */
        ksz9021_phy_extended_write(phydev, MII_KSZ9021_EXT_RGMII_RX_DATA_SKEW, 0x0);
        /* min tx data delay */
        ksz9021_phy_extended_write(phydev, MII_KSZ9021_EXT_RGMII_TX_DATA_SKEW, 0x0);
        /* max rx/tx clock delay, min rx/tx control */
        ksz9021_phy_extended_write(phydev, MII_KSZ9021_EXT_RGMII_CLOCK_SKEW, 0xf0f0);
    } else if (0x0007c0d1 == phydev->phy_id) {
        /* seems we are running on QEMU, no special init for the emulated PHY */
    } else {
        ZF_LOGW("SABRE: unexpected PHY with ID 0x%x", phydev->phy_id);
    }

#elif defined(CONFIG_PLAT_NITROGEN6SX)

    if (0x004dd072 == phydev->phy_id) {
        /* Disable Ar803x PHY SmartEEE feature, it causes link status glitches
         * that result in the ethernet link going down and up.
         */
        phy_write(phydev, MDIO_DEVAD_NONE, 0xd, 0x3);
        phy_write(phydev, MDIO_DEVAD_NONE, 0xe, 0x805d);
        phy_write(phydev, MDIO_DEVAD_NONE, 0xd, 0x4003);
        int val = phy_read(phydev, MDIO_DEVAD_NONE, 0xe);
        phy_write(phydev, MDIO_DEVAD_NONE, 0xe, val & ~(1 << 8));
    } else {
        ZF_LOGW("NITROGEN6SX: unexpected PHY with ID 0x%x", phydev->phy_id);
    }

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
