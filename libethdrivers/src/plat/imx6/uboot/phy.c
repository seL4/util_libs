/*
 * Generic PHY Management code
 * Based loosely off of Linux's PHY Lib
 *
 * Copyright 2011 Freescale Semiconductor, Inc.
 * author Andy Fleming
 * Copyright 2020, HENSOLDT Cyber GmbH
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
#include "fec_mxc.h"
#include "miiphy.h"
#include "phy.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* Generic PHY support and helper functions */

/**
 * genphy_config_advert - sanitize and advertise auto-negotation parameters
 * @phydev: target phy_device struct
 *
 * Description: Writes MII_ADVERTISE with the appropriate values,
 *   after sanitizing the values to make sure we only advertise
 *   what is supported.  Returns < 0 on error, 0 if the PHY's advertisement
 *   hasn't changed, and > 0 if it has changed.
 */
static int genphy_config_advert(struct phy_device *phydev)
{
    u32 advertise;
    int oldadv, adv;
    int err, changed = 0;

    /* Only allow advertising what
     * this PHY supports */
    phydev->advertising &= phydev->supported;
    advertise = phydev->advertising;

    /* Setup standard advertisement */
    oldadv = adv = phy_read(phydev, MDIO_DEVAD_NONE, MII_ADVERTISE);

    if (adv < 0) {
        return adv;
    }

    adv &= ~(ADVERTISE_ALL | ADVERTISE_100BASE4 | ADVERTISE_PAUSE_CAP |
             ADVERTISE_PAUSE_ASYM);
    if (advertise & ADVERTISED_10baseT_Half) {
        adv |= ADVERTISE_10HALF;
    }
    if (advertise & ADVERTISED_10baseT_Full) {
        adv |= ADVERTISE_10FULL;
    }
    if (advertise & ADVERTISED_100baseT_Half) {
        adv |= ADVERTISE_100HALF;
    }
    if (advertise & ADVERTISED_100baseT_Full) {
        adv |= ADVERTISE_100FULL;
    }
    if (advertise & ADVERTISED_Pause) {
        adv |= ADVERTISE_PAUSE_CAP;
    }
    if (advertise & ADVERTISED_Asym_Pause) {
        adv |= ADVERTISE_PAUSE_ASYM;
    }

    if (adv != oldadv) {
        err = phy_write(phydev, MDIO_DEVAD_NONE, MII_ADVERTISE, adv);

        if (err < 0) {
            return err;
        }
        changed = 1;
    }

    /* Configure gigabit if it's supported */
    if (phydev->supported & (SUPPORTED_1000baseT_Half |
                             SUPPORTED_1000baseT_Full)) {
        oldadv = adv = phy_read(phydev, MDIO_DEVAD_NONE, MII_CTRL1000);

        if (adv < 0) {
            return adv;
        }

        adv &= ~(ADVERTISE_1000FULL | ADVERTISE_1000HALF);
        if (advertise & SUPPORTED_1000baseT_Half) {
            adv |= ADVERTISE_1000HALF;
        }
        if (advertise & SUPPORTED_1000baseT_Full) {
            adv |= ADVERTISE_1000FULL;
        }

        if (adv != oldadv) {
            err = phy_write(phydev, MDIO_DEVAD_NONE, MII_CTRL1000,
                            adv);

            if (err < 0) {
                return err;
            }
            changed = 1;
        }
    }

    return changed;
}

/**
 * genphy_setup_forced - configures/forces speed/duplex from @phydev
 * @phydev: target phy_device struct
 *
 * Description: Configures MII_BMCR to force speed/duplex
 *   to the values in phydev. Assumes that the values are valid.
 */
static int genphy_setup_forced(struct phy_device *phydev)
{
    int err;
    int ctl = 0;

    phydev->pause = phydev->asym_pause = 0;

    if (SPEED_1000 == phydev->speed) {
        ctl |= BMCR_SPEED1000;
    } else if (SPEED_100 == phydev->speed) {
        ctl |= BMCR_SPEED100;
    }

    if (DUPLEX_FULL == phydev->duplex) {
        ctl |= BMCR_FULLDPLX;
    }

    err = phy_write(phydev, MDIO_DEVAD_NONE, MII_BMCR, ctl);

    return err;
}

/**
 * genphy_restart_aneg - Enable and Restart Autonegotiation
 * @phydev: target phy_device struct
 */
int genphy_restart_aneg(struct phy_device *phydev)
{
    int ctl;

    ctl = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMCR);

    if (ctl < 0) {
        return ctl;
    }

    ctl |= (BMCR_ANENABLE | BMCR_ANRESTART);

    /* Don't isolate the PHY if we're negotiating */
    ctl &= ~(BMCR_ISOLATE);

    ctl = phy_write(phydev, MDIO_DEVAD_NONE, MII_BMCR, ctl);

    return ctl;
}

/**
 * genphy_config_aneg - restart auto-negotiation or write BMCR
 * @phydev: target phy_device struct
 *
 * Description: If auto-negotiation is enabled, we configure the
 *   advertising, and then restart auto-negotiation.  If it is not
 *   enabled, then we write the BMCR.
 */
int genphy_config_aneg(struct phy_device *phydev)
{
    int result;

    if (AUTONEG_ENABLE != phydev->autoneg) {
        return genphy_setup_forced(phydev);
    }

    result = genphy_config_advert(phydev);

    if (result < 0) { /* error */
        return result;
    }

    if (result == 0) {
        /* Advertisment hasn't changed, but maybe aneg was never on to
         * begin with?  Or maybe phy was isolated? */
        int ctl = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMCR);

        if (ctl < 0) {
            return ctl;
        }

        if (!(ctl & BMCR_ANENABLE) || (ctl & BMCR_ISOLATE)) {
            result = 1;    /* do restart aneg */
        }
    }

    /* Only restart aneg if we are advertising something different
     * than we were before.  */
    if (result > 0) {
        result = genphy_restart_aneg(phydev);
    }

    return result;
}

/**
 * genphy_update_link - update link status in @phydev
 * @phydev: target phy_device struct
 *
 * Description: Update the value in phydev->link to reflect the
 *   current link value.  In order to do this, we need to read
 *   the status register twice, keeping the second value.
 */
int genphy_update_link(struct phy_device *phydev)
{
    unsigned int mii_reg;

    /*
     * Wait if the link is up, and autonegotiation is in progress
     * (ie - we're capable and it's not done)
     */
    mii_reg = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMSR);

    /*
     * If we already saw the link up, and it hasn't gone down, then
     * we don't need to wait for autoneg again
     */
    if (phydev->link && mii_reg & BMSR_LSTATUS) {
        ZF_LOGI("link already established");
        return 0;
    }

    if ((mii_reg & BMSR_ANEGCAPABLE) && !(mii_reg & BMSR_ANEGCOMPLETE)) {
        int i = 0;

        ZF_LOGI(
            "waiting for auto negotiation to complete on phy %d, '%s'",
            phydev->addr, phydev->dev->name);
        fflush(stdout);
        while (!(mii_reg & BMSR_ANEGCOMPLETE)) {
            /*
             * Timeout reached ?
             */
            if (i > PHY_ANEG_TIMEOUT) {
                ZF_LOGE("auto negotiation timeout");
                phydev->link = 0;
                return 0;
            }

            if ((i++ % 500) == 0) {
                printf(".");
                fflush(stdout);
            }

            udelay(1000);   /* 1 ms */
            mii_reg = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMSR);
        }
        ZF_LOGI("auto negotiation complete on phy %d, '%s'",
                phydev->addr, phydev->dev->name);
        phydev->link = 1;
    } else {
        /* Read the link a second time to clear the latched state */
        mii_reg = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMSR);

        if (mii_reg & BMSR_LSTATUS) {
            phydev->link = 1;
        } else {
            phydev->link = 0;
        }
    }

    return 0;
}

/*
 * Generic function which updates the speed and duplex.  If
 * autonegotiation is enabled, it uses the AND of the link
 * partner's advertised capabilities and our advertised
 * capabilities.  If autonegotiation is disabled, we use the
 * appropriate bits in the control register.
 *
 * Stolen from Linux's mii.c and phy_device.c
 */
static int genphy_parse_link(struct phy_device *phydev)
{
    int mii_reg = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMSR);

    /* We're using autonegotiation */
    if (mii_reg & BMSR_ANEGCAPABLE) {
        u32 lpa = 0;
        u32 gblpa = 0;

        /* Check for gigabit capability */
        if (mii_reg & BMSR_ERCAP) {
            /* We want a list of states supported by
             * both PHYs in the link
             */
            gblpa = phy_read(phydev, MDIO_DEVAD_NONE, MII_STAT1000);
            gblpa &= phy_read(phydev,
                              MDIO_DEVAD_NONE, MII_CTRL1000) << 2;
        }

        /* Set the baseline so we only have to set them
         * if they're different
         */
        phydev->speed = SPEED_10;
        phydev->duplex = DUPLEX_HALF;

        /* Check the gigabit fields */
        if (gblpa & (PHY_1000BTSR_1000FD | PHY_1000BTSR_1000HD)) {
            phydev->speed = SPEED_1000;

            if (gblpa & PHY_1000BTSR_1000FD) {
                phydev->duplex = DUPLEX_FULL;
            }

            /* We're done! */
            return 0;
        }

        lpa = phy_read(phydev, MDIO_DEVAD_NONE, MII_ADVERTISE);
        lpa &= phy_read(phydev, MDIO_DEVAD_NONE, MII_LPA);

        if (lpa & (LPA_100FULL | LPA_100HALF)) {
            phydev->speed = SPEED_100;

            if (lpa & LPA_100FULL) {
                phydev->duplex = DUPLEX_FULL;
            }

        } else if (lpa & LPA_10FULL) {
            phydev->duplex = DUPLEX_FULL;
        }
    } else {
        u32 bmcr = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMCR);

        phydev->speed = SPEED_10;
        phydev->duplex = DUPLEX_HALF;

        if (bmcr & BMCR_FULLDPLX) {
            phydev->duplex = DUPLEX_FULL;
        }

        if (bmcr & BMCR_SPEED1000) {
            phydev->speed = SPEED_1000;
        } else if (bmcr & BMCR_SPEED100) {
            phydev->speed = SPEED_100;
        }
    }

    return 0;
}

int genphy_config(struct phy_device *phydev)
{
    int val;
    u32 features;

    /* For now, I'll claim that the generic driver supports
     * all possible port types */
    features = (SUPPORTED_TP | SUPPORTED_MII
                | SUPPORTED_AUI | SUPPORTED_FIBRE |
                SUPPORTED_BNC);

    /* Do we support autonegotiation? */
    val = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMSR);

    if (val < 0) {
        return val;
    }

    if (val & BMSR_ANEGCAPABLE) {
        features |= SUPPORTED_Autoneg;
    }

    if (val & BMSR_100FULL) {
        features |= SUPPORTED_100baseT_Full;
    }
    if (val & BMSR_100HALF) {
        features |= SUPPORTED_100baseT_Half;
    }
    if (val & BMSR_10FULL) {
        features |= SUPPORTED_10baseT_Full;
    }
    if (val & BMSR_10HALF) {
        features |= SUPPORTED_10baseT_Half;
    }

    if (val & BMSR_ESTATEN) {
        val = phy_read(phydev, MDIO_DEVAD_NONE, MII_ESTATUS);

        if (val < 0) {
            return val;
        }

        if (val & ESTATUS_1000_TFULL) {
            features |= SUPPORTED_1000baseT_Full;
        }
        if (val & ESTATUS_1000_THALF) {
            features |= SUPPORTED_1000baseT_Half;
        }
    }

    phydev->supported = features;
    phydev->advertising = features;

    genphy_config_aneg(phydev);

    return 0;
}

int genphy_startup(struct phy_device *phydev)
{
    genphy_update_link(phydev);
    genphy_parse_link(phydev);

    return 0;
}

int genphy_shutdown(struct phy_device *phydev)
{
    return 0;
}

static struct phy_driver genphy_driver = {
    .uid        = 0xffffffff,
    .mask       = 0xffffffff,
    .name       = "Generic PHY",
    .features   = 0,
    .config     = genphy_config,
    .startup    = genphy_startup,
    .shutdown   = genphy_shutdown,
};

struct list_head phy_drivers = { &phy_drivers, &phy_drivers };

int phy_register(struct phy_driver *drv)
{
    drv->list.next = &drv->list;
    drv->list.prev = &drv->list;
    list_add_tail(&drv->list, &phy_drivers);

    return 0;
}

static int phy_probe(struct phy_device *phydev)
{
    int err = 0;

    phydev->advertising = phydev->supported = phydev->drv->features;
    phydev->mmds = phydev->drv->mmds;

    if (phydev->drv->probe) {
        err = phydev->drv->probe(phydev);
    }

    return err;
}

static struct phy_driver *generic_for_interface(phy_interface_t interface)
{
#ifdef CONFIG_PHYLIB_10G
    if (is_10g_interface(interface)) {
        return &gen10g_driver;
    }
#endif

    return &genphy_driver;
}

static struct phy_driver *get_phy_driver(struct phy_device *phydev,
                                         phy_interface_t interface)
{
    struct list_head *entry;
    int phy_id = phydev->phy_id;
    struct phy_driver *drv = NULL;

    list_for_each(entry, &phy_drivers) {
        drv = list_entry(entry, struct phy_driver, list);
        if ((drv->uid & drv->mask) == (phy_id & drv->mask)) {
            return drv;
        }
    }

    /* If we made it here, there's no driver for this PHY */
    return generic_for_interface(interface);
}

static struct phy_device *phy_device_create(struct mii_dev *bus, int addr,
                                            int phy_id,
                                            phy_interface_t interface)
{
    struct phy_device *dev;

    /* We allocate the device, and initialize the
     * default values */
    dev = malloc(sizeof(*dev));
    if (!dev) {
        ZF_LOGE("Failed to allocate PHY device for '%s':%d", bus->name, addr);
        return NULL;
    }

    memset(dev, 0, sizeof(*dev));

    dev->duplex = -1;
    dev->link = 1;
    dev->interface = interface;

    dev->autoneg = AUTONEG_ENABLE;

    dev->addr = addr;
    dev->phy_id = phy_id;
    dev->bus = bus;

    dev->drv = get_phy_driver(dev, interface);

    phy_probe(dev);

    bus->phymap[addr] = dev;

    return dev;
}

/**
 * get_phy_id - reads the specified addr for its ID.
 * @bus: the target MII bus
 * @addr: PHY address on the MII bus
 * @phy_id: where to store the ID retrieved.
 *
 * Description: Reads the ID registers of the PHY at @addr on the
 *   @bus, stores it in @phy_id and returns zero on success.
 */
static int get_phy_id(struct mii_dev *bus, int addr, int devad, u32 *phy_id)
{
    int phy_reg;

    /* Grab the bits from PHYIR1, and put them
     * in the upper half */
    phy_reg = bus->read(bus, addr, devad, MII_PHYSID1);

    if (phy_reg < 0) {
        ZF_LOGE("read MII_PHYSID1 failed, code %d", phy_reg);
        return -EIO;
    }

    *phy_id = (phy_reg & 0xffff) << 16;

    /* Grab the bits from PHYIR2, and put them in the lower half */
    phy_reg = bus->read(bus, addr, devad, MII_PHYSID2);

    if (phy_reg < 0) {
        ZF_LOGE("read MII_PHYSID2 failed, code %d", phy_reg);
        return -EIO;
    }

    *phy_id |= (phy_reg & 0xffff);

    return 0;
}

static struct phy_device *create_phy_by_mask(struct mii_dev *bus,
                                             unsigned phy_mask, int devad, phy_interface_t interface)
{
    u32 phy_id = 0xffffffff;
    while (phy_mask) {
        int addr = ffs(phy_mask) - 1;
        int r = get_phy_id(bus, addr, devad, &phy_id);
        if (r < 0) {
            ZF_LOGE("Failed to get PHY ID, code %d", r);
            return NULL;
        }
        /* If the PHY ID is mostly f's, we didn't find anything */
        if ((phy_id & 0x1fffffff) != 0x1fffffff) {
            return phy_device_create(bus, addr, phy_id, interface);
        }
        phy_mask &= ~(BIT(addr));
    }
    ZF_LOGE("Failed to create PHY by mask");
    return NULL;
}

static struct phy_device *search_for_existing_phy(struct mii_dev *bus,
                                                  unsigned phy_mask, phy_interface_t interface)
{
    /* If we have one, return the existing device, with new interface */
    while (phy_mask) {
        int addr = ffs(phy_mask) - 1;
        if (bus->phymap[addr]) {
            bus->phymap[addr]->interface = interface;
            return bus->phymap[addr];
        }
        phy_mask &= ~(BIT(addr));
    }
    return NULL;
}

static struct phy_device *get_phy_device_by_mask(struct mii_dev *bus,
                                                 unsigned phy_mask, phy_interface_t interface)
{
    struct phy_device *phydev = search_for_existing_phy(bus, phy_mask, interface);
    if (phydev) {
        return phydev;
    }
    /* Try Standard (ie Clause 22) access */
    /* Otherwise we have to try Clause 45 */
    for (unsigned int i = 0; i < 5; i++) {
        phydev = create_phy_by_mask(bus, phy_mask, i ? i : MDIO_DEVAD_NONE, interface);
        if (phydev) {
            return phydev;
        }
    }
    ZF_LOGE("PHY not found");
    return phy_device_create(bus, ffs(phy_mask) - 1, 0xffffffff, interface);
}

/**
 * get_phy_device - reads the specified PHY device and returns its @phy_device struct
 * @bus: the target MII bus
 * @addr: PHY address on the MII bus
 *
 * Description: Reads the ID registers of the PHY at @addr on the
 *   @bus, then allocates and returns the phy_device to represent it.
 */
static struct phy_device *get_phy_device(struct mii_dev *bus, int addr,
                                         phy_interface_t interface)
{
    return get_phy_device_by_mask(bus, BIT(addr), interface);
}

int phy_reset(struct phy_device *phydev)
{
    int reg;
    int timeout = 500;
    int devad = MDIO_DEVAD_NONE;

#ifdef CONFIG_PHYLIB_10G
    /* If it's 10G, we need to issue reset through one of the MMDs */
    if (is_10g_interface(phydev->interface)) {
        if (!phydev->mmds) {
            gen10g_discover_mmds(phydev);
        }

        devad = ffs(phydev->mmds) - 1;
    }
#endif

    reg = phy_read(phydev, devad, MII_BMCR);
    if (reg < 0) {
        ZF_LOGE("PHY status read failed");
        return -1;
    }

    reg |= BMCR_RESET;

    if (phy_write(phydev, devad, MII_BMCR, reg) < 0) {
        ZF_LOGE("PHY reset failed");
        return -1;
    }

#ifdef CONFIG_PHY_RESET_DELAY
    udelay(CONFIG_PHY_RESET_DELAY); /* Intel LXT971A needs this */
#endif
    /*
     * Poll the control register for the reset bit to go to 0 (it is
     * auto-clearing).  This should happen within 0.5 seconds per the
     * IEEE spec.
     */
    while ((reg & BMCR_RESET) && timeout--) {
        reg = phy_read(phydev, devad, MII_BMCR);

        if (reg < 0) {
            ZF_LOGE("PHY status read failed");
            return -1;
        }
        udelay(1000);
    }

    if (reg & BMCR_RESET) {
        ZF_LOGE("PHY reset timed out");
        return -1;
    }

    return 0;
}

int miiphy_reset(const char *devname, unsigned char addr)
{
    struct mii_dev *bus = miiphy_get_dev_by_name(devname);
    struct phy_device *phydev;

    /*
     * miiphy_reset was only used on standard PHYs, so we'll fake it here.
     * If later code tries to connect with the right interface, this will
     * be corrected by get_phy_device in phy_connect()
     */
    phydev = get_phy_device(bus, addr, PHY_INTERFACE_MODE_MII);

    return phy_reset(phydev);
}

struct phy_device *phy_connect_by_mask(struct mii_dev *bus, unsigned phy_mask,
                                       struct eth_device *dev, phy_interface_t interface)
{
    struct phy_device *phydev;

    /* Reset the bus */
    if (bus->reset) {
        bus->reset(bus);
    }

    /* Wait 15ms to make sure the PHY has come out of hard reset */
    udelay(15000);

    phydev = get_phy_device_by_mask(bus, phy_mask, interface);

    if (!phydev) {
        ZF_LOGE("Could not get PHY for '%s' phy mask 0x%x",
                bus->name, phy_mask);
        return NULL;
    }

    /* Soft Reset the PHY */
    phy_reset(phydev);

    if (phydev->dev) {
        ZF_LOGI("%s:%d is connected to %s.  Reconnecting to %s",
                bus->name, phydev->addr, phydev->dev->name, dev->name);
    }
    phydev->dev = dev;

    ZF_LOGI("Connected PHY '%s' at address %d to '%s'",
            phydev->drv->name, phydev->addr, phydev->dev->name);

    return phydev;
}

/*
 * Start the PHY.  Returns 0 on success, or a negative error code.
 */
struct phy_device *phy_connect(struct mii_dev *bus, int addr,
                               struct eth_device *dev, phy_interface_t interface)
{
    return phy_connect_by_mask(bus, BIT(addr), dev, interface);
}

int phy_shutdown(struct phy_device *phydev)
{
    if (phydev->drv->shutdown) {
        phydev->drv->shutdown(phydev);
    }

    return 0;
}
