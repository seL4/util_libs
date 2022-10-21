/*
 * (C) Copyright 2011 Michal Simek
 * Copyright 2017, DornerWorks, Ltd.
 *
 * Ported to seL4
 *
 * Michal SIMEK <monstr@monstr.eu>
 *
 * Based on Xilinx gmac driver:
 * (C) Copyright 2011 Xilinx
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "common.h"
#include "../io.h"
#include "../unimplemented.h"
#include "miiphy.h"
#include "phy.h"
#include "err.h"
#include "net.h"
#include "config.h"
#include "system.h"
#include "../zynq_gem.h"

#include <platsupport/clock.h>
#include <platsupport/io.h>

#include <malloc.h>
#include <errno.h>

#if !defined(CONFIG_PHYLIB)
# error XILINX_GEM_ETHERNET requires PHYLIB
#endif

#ifdef CONFIG_API
void (*push_packet)(void *, int len) = 0;
#endif

// TODO: Get MAC address from CAmkES config
#define ZYNQ_DEFAULT_MAC "\x00\x0a\x35\x03\x61\x46"

ps_io_ops_t *zynq_io_ops;

/* Initialized, rxbd_current, rx_first_buf must be 0 after init */
struct zynq_gem_priv {
    struct emac_bd *tx_bd;
    struct emac_bd *rx_bd;
    char *vrxbuffers;
    char *prxbuffers;
    u32 rxbuf_offset;
    u32 bd_phys;
    u32 rxbd_current;
    u32 rx_first_buf;
    int phyaddr;
    u32 emio;
    int init;
    phy_interface_t interface;
    struct phy_device *phydev;
    struct mii_dev *bus;
    ps_io_ops_t *io_ops;
    bool dma_64bit;
};

static inline int mdio_wait(struct eth_device *dev)
{
    struct zynq_gem_regs *regs = (struct zynq_gem_regs *)dev->iobase;
    u32 timeout = 20000;

    /* Wait till MDIO interface is ready to accept a new transaction. */
    while (--timeout) {
        if (readl(&regs->nwsr) & ZYNQ_GEM_NWSR_MDIOIDLE_MASK) {
            break;
        }
        WATCHDOG_RESET();
    }

    if (!timeout) {
        printf("%s: Timeout\n", __func__);
        return 1;
    }

    return 0;
}

static u32 phy_setup_op(struct eth_device *dev, u32 phy_addr, u32 regnum,
                        u32 op, u16 *data)
{
    u32 mgtcr;
    struct zynq_gem_regs *regs = (struct zynq_gem_regs *)dev->iobase;

    if (mdio_wait(dev)) {
        return 1;
    }

    /* Construct mgtcr mask for the operation */
    mgtcr = ZYNQ_GEM_PHYMNTNC_OP_MASK | op |
            (phy_addr << ZYNQ_GEM_PHYMNTNC_PHYAD_SHIFT_MASK) |
            (regnum << ZYNQ_GEM_PHYMNTNC_PHREG_SHIFT_MASK) | *data;

    /* Write mgtcr and wait for completion */
    writel(mgtcr, &regs->phymntnc);

    if (mdio_wait(dev)) {
        return 1;
    }

    if (op == ZYNQ_GEM_PHYMNTNC_OP_R_MASK) {
        *data = readl(&regs->phymntnc);
    }

    return 0;
}

static u32 phyread(struct eth_device *dev, u32 phy_addr, u32 regnum, u16 *val)
{
    u32 ret;

    ret = phy_setup_op(dev, phy_addr, regnum,
                       ZYNQ_GEM_PHYMNTNC_OP_R_MASK, val);

    if (!ret)
        debug("%s: phy_addr %d, regnum 0x%x, val 0x%x\n", __func__,
              phy_addr, regnum, *val);

    return ret;
}

static u32 phywrite(struct eth_device *dev, u32 phy_addr, u32 regnum, u16 data)
{
    debug("%s: phy_addr %d, regnum 0x%x, data 0x%x\n", __func__, phy_addr,
          regnum, data);

    return phy_setup_op(dev, phy_addr, regnum,
                        ZYNQ_GEM_PHYMNTNC_OP_W_MASK, &data);
}

static int phy_detection(struct eth_device *dev)
{
    int i;
    u16 phyreg;
    struct zynq_gem_priv *priv = dev->priv;

    if (priv->phyaddr != -1) {
        phyread(dev, priv->phyaddr, PHY_DETECT_REG, &phyreg);
        if ((phyreg != 0xFFFF) &&
            ((phyreg & PHY_DETECT_MASK) == PHY_DETECT_MASK)) {
            /* Found a valid PHY address */
            debug("Default phy address %d is valid\n",
                  priv->phyaddr);
            return 0;
        } else {
            debug("PHY address is not setup correctly %d\n",
                  priv->phyaddr);
            priv->phyaddr = -1;
        }
    }

    debug("detecting phy address\n");
    if (priv->phyaddr == -1) {
        /* detect the PHY address */
        for (i = 31; i >= 0; i--) {
            phyread(dev, i, PHY_DETECT_REG, &phyreg);
            if ((phyreg != 0xFFFF) &&
                ((phyreg & PHY_DETECT_MASK) == PHY_DETECT_MASK)) {
                /* Found a valid PHY address */
                priv->phyaddr = i;
                debug("Found valid phy address, %d\n", i);
                return 0;
            }
        }
    }
    printf("PHY is not detected\n");
    return -1;
}

int zynq_gem_setup_mac(struct eth_device *dev)
{
    u32 i, macaddrlow, macaddrhigh;
    struct zynq_gem_regs *regs = (struct zynq_gem_regs *)dev->iobase;

    /* Set the MAC bits [31:0] in BOT */
    macaddrlow = dev->enetaddr[0];
    macaddrlow |= dev->enetaddr[1] << 8;
    macaddrlow |= dev->enetaddr[2] << 16;
    macaddrlow |= dev->enetaddr[3] << 24;

    /* Set MAC bits [47:32] in TOP */
    macaddrhigh = dev->enetaddr[4];
    macaddrhigh |= dev->enetaddr[5] << 8;

    for (i = 0; i < 4; i++) {
        writel(0, &regs->laddr[i][LADDR_LOW]);
        writel(0, &regs->laddr[i][LADDR_HIGH]);
        /* Do not use MATCHx register */
        writel(0, &regs->match[i]);
    }

    writel(macaddrlow, &regs->laddr[0][LADDR_LOW]);
    writel(macaddrhigh, &regs->laddr[0][LADDR_HIGH]);

    return 0;
}

void zynq_set_gem_ioops(ps_io_ops_t *io_ops)
{
    zynq_io_ops = io_ops;
}

void zynq_gem_prom_enable(struct eth_device *dev)
{
    struct zynq_gem_regs *regs = (struct zynq_gem_regs *)dev->iobase;

    /* Read Current Value and Set CopyAll bit */
    uint32_t status = readl(&regs->nwcfg);
    writel(status | ZYNQ_GEM_NWCFG_COPY_ALL, &regs->nwcfg);
}

void zynq_gem_prom_disable(struct eth_device *dev)
{
    struct zynq_gem_regs *regs = (struct zynq_gem_regs *)dev->iobase;

    /* Read Current Value and Clear CopyAll bit */
    uint32_t status = readl(&regs->nwcfg);
    writel(status & ~ZYNQ_GEM_NWCFG_COPY_ALL, &regs->nwcfg);
}

int zynq_gem_init(struct eth_device *dev)
{
    u32 i;
    int ret;
    unsigned long clk_rate = 0;
    struct phy_device *phydev;
    struct zynq_gem_regs *regs = (struct zynq_gem_regs *)dev->iobase;
    struct zynq_gem_priv *priv = dev->priv;
    struct clock *gem_clk;
    const u32 supported = SUPPORTED_10baseT_Half |
                          SUPPORTED_10baseT_Full |
                          SUPPORTED_100baseT_Half |
                          SUPPORTED_100baseT_Full |
                          SUPPORTED_1000baseT_Half |
                          SUPPORTED_1000baseT_Full;

    printf("zynq_gem_init: Start\n");

    if (readl(&regs->dcfg6) & ZYNQ_GEM_DCFG_DBG6_DMA_64B) {
        priv->dma_64bit = true;
    } else {
        priv->dma_64bit = false;
    }

#if defined(CONFIG_PHYS_64BIT)
    if (!priv->dma_64bit) {
        printf("ERR: %s: Using 64-bit DMA but HW doesn't support it\n",
               __func__);
        return -EINVAL;
    }
#else
    if (priv->dma_64bit) {
        debug("WARN: %s: Not using 64-bit dma even though HW supports it\n",
              __func__);
    }
#endif

    if (!priv->init) {
        /* Disable all interrupts */
        writel(0xFFFFFFFF, &regs->idr);

        /* Disable the receiver & transmitter */
        writel(0, &regs->nwctrl);
        writel(0xFFFFFFFF, &regs->txsr);
        writel(0xFFFFFFFF, &regs->rxsr);
        writel(0, &regs->phymntnc);
        writel(0, &regs->transmit_q1_ptr);
        writel(0, &regs->receive_q1_ptr);

        /* Clear the Hash registers for the mac address
         * pointed by AddressPtr
         */
        writel(0x0, &regs->hashl);
        /* Write bits [63:32] in TOP */
        writel(0x0, &regs->hashh);

        /* Clear all counters */
        for (i = 0; i < STAT_SIZE; i++) {
            readl(&regs->stat[i]);
        }

        /* Setup for DMA Configuration register */
        writel(ZYNQ_GEM_DMACR_INIT, &regs->dmacr);

        /* Setup for Network Control register, MDIO, Rx and Tx enable */
        setbits_le32(&regs->nwctrl, ZYNQ_GEM_NWCTRL_MDEN_MASK);

        priv->init++;
    }

    ret = phy_detection(dev);
    if (ret) {
        printf("GEM PHY init failed\n");
        return ret;
    }

    /* interface - look at tsec */
    phydev = phy_connect(priv->bus, priv->phyaddr, dev,
                         priv->interface);

    phydev->supported = supported | ADVERTISED_Pause |
                        ADVERTISED_Asym_Pause;
    phydev->advertising = phydev->supported;
    priv->phydev = phydev;
    phy_config(phydev);

    ret = phy_startup(phydev);
    if (ret) {
        printf("phy_startup failed!\n");
        return ret;
    }

    if (!phydev->link) {
        printf("%s: No link.\n", phydev->dev->name);
        return -1;
    }

    switch (phydev->speed) {
    case SPEED_1000:
        writel(ZYNQ_GEM_NWCFG_INIT | ZYNQ_GEM_NWCFG_SPEED1000,
               &regs->nwcfg);
        clk_rate = ZYNQ_GEM_FREQUENCY_1000;
        break;
    case SPEED_100:
        writel(ZYNQ_GEM_NWCFG_INIT | ZYNQ_GEM_NWCFG_SPEED100,
               &regs->nwcfg);
        clk_rate = ZYNQ_GEM_FREQUENCY_100;
        break;
    case SPEED_10:
        clk_rate = ZYNQ_GEM_FREQUENCY_10;
        break;
    }

    /* Note:
     * This is the place to configure the the GEM clock if not using the EMIO interface
     * (!priv->emio). Clock configuration on the ZynqMP is different from the Zynq7000, making
     * porting non-trivial. For now, assume that the bootloader has set appropriate clock settings.
     */

    /* Enable IRQs */
    writel((ZYNQ_GEM_IXR_FRAMERX | ZYNQ_GEM_IXR_TXCOMPLETE), &regs->ier);
    setbits_le32(&regs->nwctrl, ZYNQ_GEM_NWCTRL_TXEN_MASK);

    return 0;
}

int zynq_gem_start_send(struct eth_device *dev, uintptr_t txbase)
{
    struct zynq_gem_regs *regs = (struct zynq_gem_regs *)dev->iobase;

    /* Wait until GEM isn't sending */
    volatile uint32_t status = readl(&regs->txsr);
    while (status & ZYNQ_GEM_TXSR_TXGO) {
        status = readl(&regs->txsr);
    }

    writel(lower_32_bits(txbase), &regs->txqbase);
#if defined(CONFIG_PHYS_64BIT)
    writel(upper_32_bits(txbase), &regs->upper_txqbase);
#endif

    /* Start transmit */
    setbits_le32(&regs->nwctrl, ZYNQ_GEM_NWCTRL_STARTTX_MASK);

    return 0;
}

int zynq_gem_recv_enabled(struct eth_device *dev)
{
    struct zynq_gem_regs *regs = (struct zynq_gem_regs *)dev->iobase;
    uint32_t val;
    /* Check Receive Enabled bit */
    val = readl(&regs->nwctrl);
    return (val & ZYNQ_GEM_NWCTRL_RXEN_MASK);
}

void zynq_gem_recv_enable(struct eth_device *dev)
{
    struct zynq_gem_regs *regs = (struct zynq_gem_regs *)dev->iobase;

    /* Enable Receive */
    setbits_le32(&regs->nwctrl, ZYNQ_GEM_NWCTRL_RXEN_MASK);
}

void zynq_gem_halt(struct eth_device *dev)
{
    struct zynq_gem_regs *regs = (struct zynq_gem_regs *)dev->iobase;

    clrsetbits_le32(&regs->nwctrl, ZYNQ_GEM_NWCTRL_RXEN_MASK |
                    ZYNQ_GEM_NWCTRL_TXEN_MASK, 0);
}

static struct eth_device *gem3_dev;

static int zynq_gem_miiphyread(const char *devname, uchar addr,
                               uchar reg, ushort *val)
{
    struct eth_device *dev = gem3_dev;
    int ret;

    ret = phyread(dev, addr, reg, val);
    debug("%s 0x%x, 0x%x, 0x%x\n", __func__, addr, reg, *val);
    return ret;
}

static int zynq_gem_miiphy_write(const char *devname, uchar addr,
                                 uchar reg, ushort val)
{
    struct eth_device *dev = gem3_dev;

    debug("%s 0x%x, 0x%x, 0x%x\n", __func__, addr, reg, val);
    return phywrite(dev, addr, reg, val);
}

struct eth_device *zynq_gem_initialize(phys_addr_t base_addr,
                                       int phy_addr, u32 emio)
{
    struct eth_device *dev;
    struct zynq_gem_priv *priv;

    dev = calloc(1, sizeof(*dev));
    if (dev == NULL) {
        return NULL;
    }

    dev->priv = calloc(1, sizeof(struct zynq_gem_priv));
    if (dev->priv == NULL) {
        free(dev);
        return NULL;
    }
    priv = dev->priv;

    if (NULL == zynq_io_ops) {
        return NULL;
    }

    priv->phyaddr = phy_addr;
    priv->emio = emio;

#ifndef CONFIG_ZYNQ_GEM_INTERFACE
    priv->interface = PHY_INTERFACE_MODE_MII;
#else
    priv->interface = CONFIG_ZYNQ_GEM_INTERFACE;
#endif

    sprintf(dev->name, "Gem.%lx", base_addr);

    dev->iobase = base_addr;

    /* fill enetaddr */
    struct zynq_gem_regs *regs = (struct zynq_gem_regs *)base_addr;
    u32 maclow = readl(&regs->laddr[0][LADDR_LOW]);
    u32 machigh = readl(&regs->laddr[0][LADDR_HIGH]);

    if (maclow | machigh) {
        dev->enetaddr[0] = maclow;
        dev->enetaddr[1] = maclow >> 8;
        dev->enetaddr[2] = maclow >> 16;
        dev->enetaddr[3] = maclow >> 24;

        dev->enetaddr[4] = machigh;
        dev->enetaddr[5] = machigh >> 8;
    } else {
        memcpy(dev->enetaddr, ZYNQ_DEFAULT_MAC, 6);
    }


    miiphy_register(dev->name, zynq_gem_miiphyread, zynq_gem_miiphy_write);
    priv->bus = miiphy_get_dev_by_name(dev->name);

    /* TBD: This is a bad way to handle this */
    gem3_dev = dev;

    return dev;
}
