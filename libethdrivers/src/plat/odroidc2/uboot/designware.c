/*
 * @TAG(OTHER_GPL)
 */

// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2010
 * Vipin Kumar, ST Micoelectronics, vipin.kumar@st.com.
 */

/*
 * Designware ethernet IP driver for U-Boot
 */

#include "common.h"
#include "net.h"
#include <errno.h>
#include "miiphy.h"
#include <malloc.h>
#include "../io.h"
#include "designware.h"

#define        GMAC_INT_MASK           0x0000003c
#define        GMAC_INT_DISABLE_RGMII          BIT(0)
#define        GMAC_INT_DISABLE_PCSLINK        BIT(1)
#define        GMAC_INT_DISABLE_PCSAN          BIT(2)
#define        GMAC_INT_DISABLE_PMT            BIT(3)
#define        GMAC_INT_DISABLE_TIMESTAMP      BIT(9)
#define        GMAC_INT_DISABLE_PCS    (GMAC_INT_DISABLE_RGMII | \
                                GMAC_INT_DISABLE_PCSLINK | \
                                GMAC_INT_DISABLE_PCSAN)
#define        GMAC_INT_DEFAULT_MASK   (GMAC_INT_DISABLE_TIMESTAMP | \
                                GMAC_INT_DISABLE_PCS)


static int dw_mdio_read(struct mii_dev *bus, int addr, int devad, int reg)
{
#ifdef CONFIG_DM_ETH
    struct dw_eth_dev *priv = dev_get_priv((struct udevice *)bus->priv);
    struct eth_mac_regs *mac_p = priv->mac_regs_p;
#else
    struct eth_mac_regs *mac_p = bus->priv;
#endif
    ulong start;
    u16 miiaddr;
    int timeout = CONFIG_MDIO_TIMEOUT;

    miiaddr = ((addr << MIIADDRSHIFT) & MII_ADDRMSK) |
              ((reg << MIIREGSHIFT) & MII_REGMSK);

    writel(miiaddr | MII_CLKRANGE_150_250M | MII_BUSY, &mac_p->miiaddr);

    for (int i = 0; i < 10; i++) {
        if (!(readl(&mac_p->miiaddr) & MII_BUSY)) {
            return readl(&mac_p->miidata);
        }
        uboot_udelay(10);
    };

    return -ETIMEDOUT;
}

static int dw_mdio_write(struct mii_dev *bus, int addr, int devad, int reg,
                         u16 val)
{
#ifdef CONFIG_DM_ETH
    struct dw_eth_dev *priv = dev_get_priv((struct udevice *)bus->priv);
    struct eth_mac_regs *mac_p = priv->mac_regs_p;
#else
    struct eth_mac_regs *mac_p = bus->priv;
#endif
    ulong start;
    u16 miiaddr;
    int ret = -ETIMEDOUT, timeout = CONFIG_MDIO_TIMEOUT;

    writel(val, &mac_p->miidata);
    miiaddr = ((addr << MIIADDRSHIFT) & MII_ADDRMSK) |
              ((reg << MIIREGSHIFT) & MII_REGMSK) | MII_WRITE;

    writel(miiaddr | MII_CLKRANGE_150_250M | MII_BUSY, &mac_p->miiaddr);

    for (int i = 0; i < 10; i++) {
        if (!(readl(&mac_p->miiaddr) & MII_BUSY)) {
            ret = 0;
            break;
        }
        uboot_udelay(10);
    };

    return ret;
}

#if defined(CONFIG_DM_ETH) && defined(CONFIG_DM_GPIO)
static int dw_mdio_reset(struct mii_dev *bus)
{
    struct udevice *dev = bus->priv;
    struct dw_eth_dev *priv = dev_get_priv(dev);
    struct dw_eth_pdata *pdata = dev_get_platdata(dev);
    int ret;

    if (!dm_gpio_is_valid(&priv->reset_gpio)) {
        return 0;
    }

    /* reset the phy */
    ret = dm_gpio_set_value(&priv->reset_gpio, 0);
    if (ret) {
        return ret;
    }

    uboot_udelay(pdata->reset_delays[0]);

    ret = dm_gpio_set_value(&priv->reset_gpio, 1);
    if (ret) {
        return ret;
    }

    uboot_udelay(pdata->reset_delays[1]);

    ret = dm_gpio_set_value(&priv->reset_gpio, 0);
    if (ret) {
        return ret;
    }

    uboot_udelay(pdata->reset_delays[2]);

    return 0;
}
#endif

static int dw_mdio_init(const char *name, void *priv)
{
    struct mii_dev *bus = mdio_alloc();

    if (!bus) {
        printf("Failed to allocate MDIO bus\n");
        return -ENOMEM;
    }

    bus->read = dw_mdio_read;
    bus->write = dw_mdio_write;
    snprintf(bus->name, sizeof(bus->name), "%s", name);
#if defined(CONFIG_DM_ETH) && defined(CONFIG_DM_GPIO)
    bus->reset = dw_mdio_reset;
#endif

    bus->priv = priv;

    return mdio_register(bus);
}

static int _dw_write_hwaddr(struct dw_eth_dev *priv, u8 *mac_id)
{
    struct eth_mac_regs *mac_p = priv->mac_regs_p;
    u32 macid_lo, macid_hi;

    macid_lo = mac_id[0] + (mac_id[1] << 8) + (mac_id[2] << 16) +
               (mac_id[3] << 24);
    macid_hi = mac_id[4] + (mac_id[5] << 8);

    writel(macid_hi, &mac_p->macaddr0hi);
    writel(macid_lo, &mac_p->macaddr0lo);

    return 0;
}

static int _dw_read_hwaddr(struct dw_eth_dev *priv, u8 *mac_out)
{
    assert(mac_out);
    struct eth_mac_regs *mac_p = priv->mac_regs_p;

    u32 macid_hi = readl(&mac_p->macaddr0hi);
    u32 macid_lo = readl(&mac_p->macaddr0lo);

    memcpy(mac_out,   &macid_lo, 4);
    memcpy(mac_out + 4, &macid_hi, 2);

    return 0;
}

static int dw_adjust_link(struct dw_eth_dev *priv, struct eth_mac_regs *mac_p,
                          struct phy_device *phydev)
{
    u32 conf = readl(&mac_p->conf) | FRAMEBURSTENABLE | DISABLERXOWN;

    if (!phydev->link) {
        printf("%s: No link.\n", phydev->dev->name);
        return 0;
    }

    if (phydev->speed != 1000) {
        conf |= MII_PORTSELECT;
    } else {
        conf &= ~MII_PORTSELECT;
    }

    if (phydev->speed == 100) {
        conf |= FES_100;
    }

    if (phydev->duplex) {
        conf |= FULLDPLXMODE;
    }

    writel(conf, &mac_p->conf);

    printf("Speed: %d, %s duplex%s\n", phydev->speed,
           (phydev->duplex) ? "full" : "half",
           (phydev->port == PORT_FIBRE) ? ", fiber mode" : "");

    return 0;
}

static void _dw_eth_halt(struct dw_eth_dev *priv)
{
    struct eth_mac_regs *mac_p = priv->mac_regs_p;
    struct eth_dma_regs *dma_p = priv->dma_regs_p;

    writel(readl(&mac_p->conf) & ~(RXENABLE | TXENABLE), &mac_p->conf);
    writel(readl(&dma_p->opmode) & ~(RXSTART | TXSTART), &dma_p->opmode);

    phy_shutdown(priv->phydev);
}

int designware_eth_init(struct dw_eth_dev *priv, u8 *enetaddr)
{
    struct eth_mac_regs *mac_p = priv->mac_regs_p;
    struct eth_dma_regs *dma_p = priv->dma_regs_p;
    ulong start;
    int ret;

    writel(readl(&dma_p->busmode) | DMAMAC_SRST, &dma_p->busmode);

    int curr = 0;
    while (readl(&dma_p->busmode) & DMAMAC_SRST) {
        if (curr > 10) {
            printf("DMA reset timeout\n");
            return -ETIMEDOUT;
        }
        curr++;

        uboot_udelay(100000);
    };

    /*
     * Soft reset above clears HW address registers.
     * So we have to set it here once again.
     */
    _dw_write_hwaddr(priv, enetaddr);

    writel(FIXEDBURST | PRIORXTX_41 | DMA_PBL, &dma_p->busmode);

#ifndef CONFIG_DW_MAC_FORCE_THRESHOLD_MODE
    writel(readl(&dma_p->opmode) | FLUSHTXFIFO | STOREFORWARD,
           &dma_p->opmode);
#else
    writel(readl(&dma_p->opmode) | FLUSHTXFIFO,
           &dma_p->opmode);
#endif

#ifdef CONFIG_DW_AXI_BURST_LEN
    writel((CONFIG_DW_AXI_BURST_LEN & 0x1FF >> 1), &dma_p->axibus);
#endif


    /* enable transmit and recv interrupts */
    writel(readl(&dma_p->intenable) | DMA_INTR_DEFAULT_MASK, &dma_p->intenable);

    /* mask unneeded GMAC interrupts */
    writel(GMAC_INT_DEFAULT_MASK, &mac_p->intmask);

    /* Start up the PHY */
    ret = phy_startup(priv->phydev);
    if (ret) {
        printf("Could not initialize PHY %s\n",
               priv->phydev->dev->name);
        return ret;
    }

    ret = dw_adjust_link(priv, mac_p, priv->phydev);
    if (ret) {
        return ret;
    }

    return 0;
}


#define ETH_ZLEN    60

static int dw_phy_init(struct dw_eth_dev *priv, void *dev)
{
    struct phy_device *phydev;
    int mask = 0xffffffff, ret;

#ifdef CONFIG_PHY_ADDR
    mask = 1 << CONFIG_PHY_ADDR;
#endif

    phydev = phy_find_by_mask(priv->bus, mask, priv->interface);
    if (!phydev) {
        return -ENODEV;
    }

    phy_connect_dev(phydev, dev);

    phydev->supported &= PHY_GBIT_FEATURES;
    if (priv->max_speed) {
        ret = phy_set_supported(phydev, priv->max_speed);
        if (ret) {
            return ret;
        }
    }
    phydev->advertising = phydev->supported;

    priv->phydev = phydev;
    phy_config(phydev);

    return 0;
}

int designware_ack(struct eth_device *dev, u32 status)
{
    struct dw_eth_dev *priv = dev->priv;
    writel(status, &priv->dma_regs_p->status);
}

int designware_read_hwaddr(struct eth_device *dev, u8 *mac_out)
{
    return _dw_read_hwaddr(dev->priv, mac_out);
}

int designware_initialize(ulong base_addr, u32 interface, struct eth_device *dev)
{
    struct dw_eth_dev *priv;

    /* This needs to exist for driver lifetime. Doesn't need to be DMA
     * any more as we allocate our DMA descriptors below */
    priv = (struct dw_eth_dev *) malloc(sizeof(struct dw_eth_dev));

    if (!priv) {
        free(dev);
        return -ENOMEM;
    }

    memset(dev, 0, sizeof(struct eth_device));
    memset(priv, 0, sizeof(struct dw_eth_dev));

    sprintf(dev->name, "dwmac.%lx", base_addr);
    dev->iobase = (int)base_addr;
    dev->priv = priv;

    priv->dev = dev;
    priv->mac_regs_p = (struct eth_mac_regs *)base_addr;
    priv->dma_regs_p = (struct eth_dma_regs *)(base_addr +
                                               DW_DMA_BASE_OFFSET);

    priv->interface = interface;

    dw_mdio_init(dev->name, priv->mac_regs_p);
    priv->bus = miiphy_get_dev_by_name(dev->name);

    return dw_phy_init(priv, dev);
}

int designware_startup(struct eth_device *dev)
{
    int ret;

    ret = designware_eth_init(dev->priv, dev->enetaddr);

    return ret;
}

int designware_enable(struct eth_device *dev)
{
    struct dw_eth_dev *priv = (struct dw_eth_dev *) dev->priv;

    struct eth_mac_regs *mac_p = priv->mac_regs_p;
    struct eth_dma_regs *dma_p = priv->dma_regs_p;

    if (!priv->phydev->link) {
        return -EIO;
    }

    writel(readl(&mac_p->conf) | RXENABLE | TXENABLE, &mac_p->conf);
    writel(readl(&dma_p->opmode) | TXSTART | RXSTART, &dma_p->opmode);

    return 0;
}

int designware_write_descriptors(struct eth_device *dev, uintptr_t tx_ring_base, uintptr_t rx_ring_base)
{
    struct dw_eth_dev *priv = (struct dw_eth_dev *) dev->priv;
    struct eth_dma_regs *dma_p = priv->dma_regs_p;
    writel((ulong)tx_ring_base, &dma_p->txdesclistaddr);
    writel((ulong)rx_ring_base, &dma_p->rxdesclistaddr);

    return 0;
}

int designware_interrupt_status(struct eth_device *dev, uint32_t *ret_status)
{
    struct dw_eth_dev *priv = (struct dw_eth_dev *) dev->priv;
    *ret_status = readl(&priv->dma_regs_p->status);
    return 0;
}

int designware_start_send(struct eth_device *dev)
{
    /* Start the transmission */
    struct dw_eth_dev *priv = (struct dw_eth_dev *) dev->priv;
    struct eth_dma_regs *dma_p = priv->dma_regs_p;
    writel(POLL_DATA, &dma_p->txpolldemand);
    return 0;
}

int designware_start_receive(struct eth_device *dev)
{
    /* Start the transmission */
    struct dw_eth_dev *priv = (struct dw_eth_dev *) dev->priv;
    struct eth_dma_regs *dma_p = priv->dma_regs_p;
    writel(POLL_DATA, &dma_p->rxpolldemand);
    return 0;
}
