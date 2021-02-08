/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

/*
 * Copyright (c) 2016, NVIDIA CORPORATION.
 *
 * Portions based on U-Boot's rtl8169.c.
 */

/*
 * This driver supports the Synopsys Designware Ethernet QOS (Quality Of
 * Service) IP block. The IP supports multiple options for bus type, clocking/
 * reset structure, and feature list.
 *
 * The driver is written such that generic core logic is kept separate from
 * configuration-specific logic. Code that interacts with configuration-
 * specific resources is split out into separate functions to avoid polluting
 * common code. If/when this driver is enhanced to support multiple
 * configurations, the core code should be adapted to call all configuration-
 * specific functions through function pointers, with the definition of those
 * function pointers being supplied by struct udevice_id eqos_ids[]'s .data
 * field.
 *
 * The following configurations are currently supported:
 * tegra186:
 *    NVIDIA's Tegra186 chip. This configuration uses an AXI master/DMA bus, an
 *    AHB slave/register bus, contains the DMA, MTL, and MAC sub-blocks, and
 *    supports a single RGMII PHY. This configuration also has SW control over
 *    all clock and reset signals to the HW block.
 */

#include "miiphy.h"
#include "net.h"
#include "phy.h"

#include "wait_bit.h"

#include <platsupport/clock.h>
#include <platsupport/io.h>
#include <platsupportports/plat/gpio.h>
#include <platsupport/gpio.h>
#include <platsupport/reset.h>

#include "../tx2.h"
#include "tx2_configs.h"

#include <string.h>
#include <ethdrivers/helpers.h>

#include "dwc_eth_qos.h"
void eqos_dma_disable_rxirq(struct tx2_eth_data *dev)
{
    struct eqos_priv *eqos = (struct eqos_priv *)dev->eth_dev;
    uint32_t regval;

    regval = eqos->dma_regs->ch0_dma_ie;
    regval &= ~DWCEQOS_DMA_CH0_IE_RIE;
    eqos->dma_regs->ch0_dma_ie = regval;
}

void eqos_dma_enable_rxirq(struct tx2_eth_data *dev)
{
    struct eqos_priv *eqos = (struct eqos_priv *)dev->eth_dev;
    uint32_t regval;

    regval = eqos->dma_regs->ch0_dma_ie;
    regval |= DWCEQOS_DMA_CH0_IE_RIE;
    eqos->dma_regs->ch0_dma_ie = regval;
}

void eqos_dma_disable_txirq(struct tx2_eth_data *dev)
{
    struct eqos_priv *eqos = (struct eqos_priv *)dev->eth_dev;
    uint32_t regval;

    regval = eqos->dma_regs->ch0_dma_ie;
    regval &= ~DWCEQOS_DMA_CH0_IE_TIE;
    eqos->dma_regs->ch0_dma_ie = regval;
}

void eqos_dma_enable_txirq(struct tx2_eth_data *dev)
{
    struct eqos_priv *eqos = (struct eqos_priv *)dev->eth_dev;
    uint32_t regval;

    regval = eqos->dma_regs->ch0_dma_ie;
    regval |= DWCEQOS_DMA_CH0_IE_TIE;
    eqos->dma_regs->ch0_dma_ie = regval;
}

void eqos_set_rx_tail_pointer(struct tx2_eth_data *dev)
{
    struct eqos_priv *eqos = (struct eqos_priv *)dev->eth_dev;
    uint32_t *dma_status = (uint32_t *)(eqos->regs + REG_DWCEQOS_DMA_CH0_STA);
    *dma_status |= DWCEQOS_DMA_CH0_IS_RI;
    size_t num_buffers_in_ring = dev->rx_size - dev->rx_remain;

    if (num_buffers_in_ring > 0) {
        uintptr_t last_rx_desc = (dev->rx_ring_phys + ((dev->rdh + num_buffers_in_ring) * sizeof(struct eqos_desc)));
        eqos->dma_regs->ch0_rxdesc_tail_pointer = last_rx_desc;
    }
}

int eqos_handle_irq(struct tx2_eth_data *dev, int irq)
{
    struct eqos_priv *eqos = (struct eqos_priv *)dev->eth_dev;

    uint32_t cause = eqos->dma_regs->dma_control[0];
    uint32_t *dma_status;
    int ret = 0;

    if (cause & DWCEQOS_DMA_IS_DC0IS) {
        dma_status = (uint32_t *)(eqos->regs + REG_DWCEQOS_DMA_CH0_STA);

        /* Transmit Interrupt */
        if (*dma_status & DWCEQOS_DMA_CH0_IS_TI) {
            ret |= TX_IRQ;
        }

        /* Receive Interrupt */
        if (*dma_status & DWCEQOS_DMA_CH0_IS_RI) {
            ret |= RX_IRQ;
        }

        /* Ack */
        *dma_status = *dma_status;
    }

    return ret;
}

static int eqos_mdio_wait_idle(struct eqos_priv *eqos)
{
    return wait_for_bit_le32(&eqos->mac_regs->mdio_address,
                             EQOS_MAC_MDIO_ADDRESS_GB, false,
                             1000000, true);
}

static int eqos_mdio_read(struct mii_dev *bus, int mdio_addr, int mdio_devad,
                          int mdio_reg)
{
    struct eqos_priv *eqos = bus->priv;
    uint32_t val;
    int ret;

    ret = eqos_mdio_wait_idle(eqos);
    if (ret) {
        ZF_LOGF("MDIO not idle at entry");
        return ret;
    }

    val = eqos->mac_regs->mdio_address;
    val &= EQOS_MAC_MDIO_ADDRESS_SKAP |
           EQOS_MAC_MDIO_ADDRESS_C45E;
    val |= (mdio_addr << EQOS_MAC_MDIO_ADDRESS_PA_SHIFT) |
           (mdio_reg << EQOS_MAC_MDIO_ADDRESS_RDA_SHIFT) |
           (eqos->config->config_mac_mdio <<
            EQOS_MAC_MDIO_ADDRESS_CR_SHIFT) |
           (EQOS_MAC_MDIO_ADDRESS_GOC_READ <<
            EQOS_MAC_MDIO_ADDRESS_GOC_SHIFT) |
           EQOS_MAC_MDIO_ADDRESS_GB;
    eqos->mac_regs->mdio_address = val;

    udelay(eqos->config->mdio_wait);

    ret = eqos_mdio_wait_idle(eqos);
    if (ret) {
        ZF_LOGF("MDIO read didn't complete");
        return ret;
    }

    val = eqos->mac_regs->mdio_data;
    val &= EQOS_MAC_MDIO_DATA_GD_MASK;

    return val;
}

static int eqos_mdio_write(struct mii_dev *bus, int mdio_addr, int mdio_devad,
                           int mdio_reg, u16 mdio_val)
{
    struct eqos_priv *eqos = bus->priv;
    u32 val;
    int ret;

    ret = eqos_mdio_wait_idle(eqos);
    if (ret) {
        ZF_LOGF("MDIO not idle at entry");
        return ret;
    }

    writel(mdio_val, &eqos->mac_regs->mdio_data);

    val = readl(&eqos->mac_regs->mdio_address);
    val &= EQOS_MAC_MDIO_ADDRESS_SKAP |
           EQOS_MAC_MDIO_ADDRESS_C45E;
    val |= (mdio_addr << EQOS_MAC_MDIO_ADDRESS_PA_SHIFT) |
           (mdio_reg << EQOS_MAC_MDIO_ADDRESS_RDA_SHIFT) |
           (eqos->config->config_mac_mdio <<
            EQOS_MAC_MDIO_ADDRESS_CR_SHIFT) |
           (EQOS_MAC_MDIO_ADDRESS_GOC_WRITE <<
            EQOS_MAC_MDIO_ADDRESS_GOC_SHIFT) |
           EQOS_MAC_MDIO_ADDRESS_GB;
    writel(val, &eqos->mac_regs->mdio_address);

    udelay(eqos->config->mdio_wait);

    ret = eqos_mdio_wait_idle(eqos);
    if (ret) {
        ZF_LOGF("MDIO read didn't complete");
        return ret;
    }

    return 0;
}

static int eqos_start_clks_tegra186(struct eqos_priv *eqos)
{
    int ret;

    assert(clock_sys_valid(eqos->clock_sys));

    eqos->clk_slave_bus = clk_get_clock(eqos->clock_sys, CLK_AXI_CBB);
    if (eqos->clk_slave_bus == NULL) {
        ZF_LOGE("clk_get_clock failed CLK_SLAVE_BUS");
        return -ENODEV;
    }
    ret = clk_gate_enable(eqos->clock_sys, CLK_GATE_AXI_CBB, CLKGATE_ON);
    if (ret) {
        ZF_LOGE("Failed to enable CLK_GATE_AXI_CBB") ;
        return -EIO;
    }

    ret = clk_gate_enable(eqos->clock_sys, CLK_GATE_EQOS_AXI, CLKGATE_ON);
    if (ret) {
        ZF_LOGE("Failed to enable CLK_GATE_EQOS_AXI");
        return -EIO;
    }

    eqos->clk_rx = clk_get_clock(eqos->clock_sys, CLK_EQOS_RX_INPUT);
    if (eqos->clk_rx == NULL) {
        ZF_LOGE("clk_get_clock failed CLK_RX");
        return -ENODEV;
    }
    ret = clk_gate_enable(eqos->clock_sys, CLK_GATE_EQOS_RX, CLKGATE_ON);
    if (ret) {
        ZF_LOGE("Failed to enable CLK_GATE_EQOS_RX");
        return -EIO;
    }

    eqos->clk_ptp_ref = clk_get_clock(eqos->clock_sys, CLK_EQOS_PTP_REF);
    if (eqos->clk_ptp_ref == NULL) {
        ZF_LOGE("clk_get_clock failed CLK_EQOS_PTP_REF");
        return -ENODEV;
    }
    ret = clk_gate_enable(eqos->clock_sys, CLK_GATE_EQOS_PTP_REF, CLKGATE_ON);
    if (ret) {
        ZF_LOGE("Failed to enable CLK_GATE_EQOS_PTP_REF");
        return -EIO;
    }

    eqos->clk_tx = clk_get_clock(eqos->clock_sys, CLK_EQOS_TX);
    if (eqos->clk_tx == NULL) {
        ZF_LOGE("clk_get_clock failed CLK_TX");
        return -ENODEV;
    }
    ret = clk_gate_enable(eqos->clock_sys, CLK_GATE_EQOS_TX, CLKGATE_ON);
    if (ret) {
        ZF_LOGE("Failed to enable CLK_GATE_EQOS_TX");
        return -EIO;
    }

    return 0;
}

static int eqos_calibrate_pads_tegra186(struct eqos_priv *eqos)
{
    int ret;

    eqos->tegra186_regs->sdmemcomppadctrl |= (EQOS_SDMEMCOMPPADCTRL_PAD_E_INPUT_OR_E_PWRD);

    udelay(1);

    eqos->tegra186_regs->auto_cal_config |= (EQOS_AUTO_CAL_CONFIG_START | EQOS_AUTO_CAL_CONFIG_ENABLE);

    ret = wait_for_bit_le32(&eqos->tegra186_regs->auto_cal_status,
                            EQOS_AUTO_CAL_STATUS_ACTIVE, true, 10, false);
    if (ret) {
        ZF_LOGE("calibrate didn't start");
        goto failed;
    }

    ret = wait_for_bit_le32(&eqos->tegra186_regs->auto_cal_status,
                            EQOS_AUTO_CAL_STATUS_ACTIVE, false, 100, false);
    if (ret) {
        ZF_LOGE("calibrate didn't finish");
        goto failed;
    }

    ret = 0;

failed:
    eqos->tegra186_regs->sdmemcomppadctrl &= ~(EQOS_SDMEMCOMPPADCTRL_PAD_E_INPUT_OR_E_PWRD);

    return ret;
}

static int eqos_disable_calibration_tegra186(struct eqos_priv *eqos)
{

    eqos->tegra186_regs->auto_cal_config &= ~(EQOS_AUTO_CAL_CONFIG_ENABLE);

    return 0;
}

static UNUSED freq_t eqos_get_tick_clk_rate_tegra186(struct eqos_priv *eqos)
{
    return clk_get_freq(eqos->clk_slave_bus);
}


static int eqos_set_full_duplex(struct eqos_priv *eqos)
{

    eqos->mac_regs->configuration |= (EQOS_MAC_CONFIGURATION_DM);

    return 0;
}

static int eqos_set_half_duplex(struct eqos_priv *eqos)
{

    eqos->mac_regs->configuration &= ~(EQOS_MAC_CONFIGURATION_DM);

    /* WAR: Flush TX queue when switching to half-duplex */
    eqos->mtl_regs->txq0_operation_mode |= (EQOS_MTL_TXQ0_OPERATION_MODE_FTQ);

    return 0;
}

static int eqos_set_gmii_speed(struct eqos_priv *eqos)
{

    eqos->mac_regs->configuration &= ~(EQOS_MAC_CONFIGURATION_PS | EQOS_MAC_CONFIGURATION_FES);

    return 0;
}

static int eqos_set_mii_speed_100(struct eqos_priv *eqos)
{

    eqos->mac_regs->configuration |=
        (EQOS_MAC_CONFIGURATION_PS | EQOS_MAC_CONFIGURATION_FES);

    return 0;
}

static int eqos_set_mii_speed_10(struct eqos_priv *eqos)
{

    eqos->mac_regs->configuration &= ~(EQOS_MAC_CONFIGURATION_FES);
    eqos->mac_regs->configuration |= (EQOS_MAC_CONFIGURATION_PS);

    return 0;
}

static int eqos_set_tx_clk_speed_tegra186(struct eqos_priv *eqos)
{
    ulong rate;
    int ret;

    switch (eqos->phy->speed) {
    case SPEED_1000:
        rate = 125 * 1000 * 1000;
        break;
    case SPEED_100:
        rate = 25 * 1000 * 1000;
        break;
    case SPEED_10:
        rate = 2.5 * 1000 * 1000;
        break;
    default:
        ZF_LOGE("invalid speed %d", eqos->phy->speed);
        return -EINVAL;
    }

    ret = clk_set_freq(eqos->clk_tx, rate);

    if (ret < 0) {
        ZF_LOGE("clk_set_rate(tx_clk, %lu) failed: %d", rate, ret);
        return ret;
    }

    return 0;
}

static int eqos_adjust_link(struct eqos_priv *eqos)
{
    int ret;
    bool en_calibration;


    if (eqos->phy->duplex) {
        ret = eqos_set_full_duplex(eqos);
    } else {
        ret = eqos_set_half_duplex(eqos);
    }
    if (ret < 0) {
        return ret;
    }

    switch (eqos->phy->speed) {
    case SPEED_1000:
        en_calibration = true;
        ret = eqos_set_gmii_speed(eqos);
        break;
    case SPEED_100:

        en_calibration = true;
        ret = eqos_set_mii_speed_100(eqos);
        break;
    case SPEED_10:

        en_calibration = false;
        ret = eqos_set_mii_speed_10(eqos);
        break;
    default:
        return -EINVAL;
    }
    if (ret < 0) {
        return ret;
    }

    ret = eqos_set_tx_clk_speed_tegra186(eqos);
    if (ret < 0) {
        ZF_LOGE("eqos_set_tx_clk_speed() failed: %d", ret);
        return ret;
    }

    if (en_calibration) {
        ret = eqos_calibrate_pads_tegra186(eqos);
        if (ret < 0) {
            ZF_LOGE("eqos_calibrate_pads() failed: %d", ret);
            return ret;
        }
    } else {
        ret = eqos_disable_calibration_tegra186(eqos);
        if (ret < 0) {
            return ret;
        }
    }

    return 0;
}

int eqos_send(struct tx2_eth_data *dev, void *packet, int length)
{
    struct eqos_priv *eqos = (struct eqos_priv *)dev->eth_dev;
    volatile struct eqos_desc *tx_desc;
    uint32_t ioc = 0;
    if (dev->tdt % 32 == 0) {
        ioc = EQOS_DESC2_IOC;
    }
    tx_desc = &(dev->tx_ring[dev->tdt]);

    tx_desc->des0 = (uintptr_t)packet;
    tx_desc->des1 = 0;
    tx_desc->des2 = ioc | length;
    tx_desc->des3 = EQOS_DESC3_FD | EQOS_DESC3_LD | length;

    __sync_synchronize();

    tx_desc->des3 |= EQOS_DESC3_OWN;

    eqos->dma_regs->ch0_txdesc_tail_pointer = (uintptr_t)(&(dev->tx_ring[dev->tdt + 1])) +
                                              sizeof(struct eqos_desc);

    return 0;
}

static const struct eqos_config eqos_tegra186_config = {
    .reg_access_always_ok = false,
    .mdio_wait = 10,
    .swr_wait = 10,
    .config_mac = EQOS_MAC_RXQ_CTRL0_RXQ0EN_ENABLED_DCB,
    .config_mac_mdio = EQOS_MAC_MDIO_ADDRESS_CR_20_35,
};

static int eqos_start_resets_tegra186(struct eqos_priv *eqos)
{
    int ret;

    ret = gpio_set(&eqos->gpio);
    if (ret < 0) {
        ZF_LOGF("dm_gpio_set_value(phy_reset, assert) failed: %d", ret);
        return ret;
    }

    udelay(2);

    ret = gpio_clr(&eqos->gpio);
    if (ret < 0) {
        ZF_LOGF("dm_gpio_set_value(phy_reset, deassert) failed: %d", ret);
        return ret;
    }

    ret = reset_sys_assert(eqos->reset_sys, RESET_EQOS);
    if (ret < 0) {
        ZF_LOGF("reset_assert() failed: %d", ret);
        return ret;
    }

    udelay(2);

    ret = reset_sys_deassert(eqos->reset_sys, RESET_EQOS);
    if (ret < 0) {
        ZF_LOGF("reset_deassert() failed: %d", ret);
        return ret;
    }

    return 0;
}

int eqos_start(struct tx2_eth_data *d)
{
    struct eqos_priv *eqos = (struct eqos_priv *)d->eth_dev;
    int ret;
    u32 val, tx_fifo_sz, rx_fifo_sz, tqs, rqs, pbl;

    eqos->reg_access_ok = true;
    uint32_t *dma_ie;

    ret = eqos_start_clks_tegra186(eqos);
    if (ret) {
        ZF_LOGE("eqos_start_clks_tegra186 failed");
        goto err;
    }

    ret = eqos_start_resets_tegra186(eqos);
    if (ret) {
        ZF_LOGE("eqos_start_resets_tegra186 failed");
        goto err_stop_clks;
    }

    udelay(10);

    ret = wait_for_bit_le32(&eqos->dma_regs->mode,
                            EQOS_DMA_MODE_SWR, false,
                            eqos->config->swr_wait, false);
    if (ret) {
        ZF_LOGE("EQOS_DMA_MODE_SWR stuck");
        goto err_stop_resets;
    }

    ret = eqos_calibrate_pads_tegra186(eqos);
    if (ret < 0) {
        ZF_LOGE("eqos_calibrate_pads() failed: %d", ret);
        goto err_stop_resets;
    }

    /*
     * if PHY was already connected and configured,
     * don't need to reconnect/reconfigure again
     */
    if (!eqos->phy) {
        eqos->phy = phy_connect(eqos->mii, 0, NULL, PHY_INTERFACE_MODE_MII);
        if (!eqos->phy) {
            ZF_LOGE("phy_connect() failed");
            goto err_stop_resets;
        }
        ret = phy_config(eqos->phy);
        if (ret < 0) {
            ZF_LOGE("phy_config() failed: %d", ret);
            goto err_shutdown_phy;
        }
    }
    ZF_LOGF_IF(!eqos->phy, "For some reason the phy is not on????");

    ret = phy_startup(eqos->phy);
    if (ret < 0) {
        ZF_LOGE("phy_startup() failed: %d", ret);
        goto err_shutdown_phy;
    }

    if (!eqos->phy->link) {
        ZF_LOGE("No link");
        goto err_shutdown_phy;
    }

    ret = eqos_adjust_link(eqos);
    if (ret < 0) {
        ZF_LOGE("eqos_adjust_link() failed: %d", ret);
        goto err_shutdown_phy;
    }

    /* Configure MTL */

    /* Flush TX queue */
    eqos->mtl_regs->txq0_operation_mode = (EQOS_MTL_TXQ0_OPERATION_MODE_FTQ);

    while (*((uint32_t *)eqos->regs + 0xd00));
    /* Enable Store and Forward mode for TX */
    eqos->mtl_regs->txq0_operation_mode = (EQOS_MTL_TXQ0_OPERATION_MODE_TSF);
    /* Program Tx operating mode */
    eqos->mtl_regs->txq0_operation_mode |= (EQOS_MTL_TXQ0_OPERATION_MODE_TXQEN_ENABLED <<
                                            EQOS_MTL_TXQ0_OPERATION_MODE_TXQEN_SHIFT);
    /* Transmit Queue weight */
    eqos->mtl_regs->txq0_quantum_weight = 0x10;

    /* Enable Store and Forward mode for RX, since no jumbo frame */
    eqos->mtl_regs->rxq0_operation_mode = (EQOS_MTL_RXQ0_OPERATION_MODE_RSF);

    /* Transmit/Receive queue fifo size; use all RAM for 1 queue */
    val = eqos->mac_regs->hw_feature1;
    tx_fifo_sz = (val >> EQOS_MAC_HW_FEATURE1_TXFIFOSIZE_SHIFT) &
                 EQOS_MAC_HW_FEATURE1_TXFIFOSIZE_MASK;
    rx_fifo_sz = (val >> EQOS_MAC_HW_FEATURE1_RXFIFOSIZE_SHIFT) &
                 EQOS_MAC_HW_FEATURE1_RXFIFOSIZE_MASK;

    /*
     * r/tx_fifo_sz is encoded as log2(n / 128). Undo that by shifting.
     * r/tqs is encoded as (n / 256) - 1.
     */
    tqs = (128 << tx_fifo_sz) / 256 - 1;
    rqs = (128 << rx_fifo_sz) / 256 - 1;

    eqos->mtl_regs->txq0_operation_mode &= ~(EQOS_MTL_TXQ0_OPERATION_MODE_TQS_MASK <<
                                             EQOS_MTL_TXQ0_OPERATION_MODE_TQS_SHIFT);
    eqos->mtl_regs->txq0_operation_mode |=
        tqs << EQOS_MTL_TXQ0_OPERATION_MODE_TQS_SHIFT;
    eqos->mtl_regs->rxq0_operation_mode &= ~(EQOS_MTL_RXQ0_OPERATION_MODE_RQS_MASK <<
                                             EQOS_MTL_RXQ0_OPERATION_MODE_RQS_SHIFT);
    eqos->mtl_regs->rxq0_operation_mode |=
        rqs << EQOS_MTL_RXQ0_OPERATION_MODE_RQS_SHIFT;

    /* Flow control used only if each channel gets 4KB or more FIFO */
    if (rqs >= ((4096 / 256) - 1)) {
        u32 rfd, rfa;

        eqos->mtl_regs->rxq0_operation_mode |= (EQOS_MTL_RXQ0_OPERATION_MODE_EHFC);

        /*
         * Set Threshold for Activating Flow Contol space for min 2
         * frames ie, (1500 * 1) = 1500 bytes.
         *
         * Set Threshold for Deactivating Flow Contol for space of
         * min 1 frame (frame size 1500bytes) in receive fifo
         */
        if (rqs == ((4096 / 256) - 1)) {
            /*
             * This violates the above formula because of FIFO size
             * limit therefore overflow may occur inspite of this.
             */
            rfd = 0x3;  /* Full-3K */
            rfa = 0x1;  /* Full-1.5K */
        } else if (rqs == ((8192 / 256) - 1)) {
            rfd = 0x6;  /* Full-4K */
            rfa = 0xa;  /* Full-6K */
        } else if (rqs == ((16384 / 256) - 1)) {
            rfd = 0x6;  /* Full-4K */
            rfa = 0x12; /* Full-10K */
        } else {
            rfd = 0x6;  /* Full-4K */
            rfa = 0x1E; /* Full-16K */
        }

        eqos->mtl_regs->rxq0_operation_mode &= ~((EQOS_MTL_RXQ0_OPERATION_MODE_RFD_MASK <<
                                                  EQOS_MTL_RXQ0_OPERATION_MODE_RFD_SHIFT) |
                                                 (EQOS_MTL_RXQ0_OPERATION_MODE_RFA_MASK <<
                                                  EQOS_MTL_RXQ0_OPERATION_MODE_RFA_SHIFT));
        eqos->mtl_regs->rxq0_operation_mode |= (rfd <<
                                                EQOS_MTL_RXQ0_OPERATION_MODE_RFD_SHIFT) |
                                               (rfa <<
                                                EQOS_MTL_RXQ0_OPERATION_MODE_RFA_SHIFT);
    }

    dma_ie = (uint32_t *)(eqos->regs + 0xc30);
    *dma_ie = 0x3020100;

    /* Configure MAC, not sure if L4T is the same */
    eqos->mac_regs->rxq_ctrl0 =
        (eqos->config->config_mac <<
         EQOS_MAC_RXQ_CTRL0_RXQ0EN_SHIFT);

    /* Set TX flow control parameters */
    /* Set Pause Time */
    eqos->mac_regs->q0_tx_flow_ctrl = (0xffff << EQOS_MAC_Q0_TX_FLOW_CTRL_PT_SHIFT);
    /* Assign priority for RX flow control */
    eqos->mac_regs->rxq_ctrl2 = (1 << EQOS_MAC_RXQ_CTRL2_PSRQ0_SHIFT);

    /* Enable flow control */
    eqos->mac_regs->q0_tx_flow_ctrl |= (EQOS_MAC_Q0_TX_FLOW_CTRL_TFE);

    eqos->mac_regs->rx_flow_ctrl = (EQOS_MAC_RX_FLOW_CTRL_RFE);

    eqos->mac_regs->configuration &=
        ~(EQOS_MAC_CONFIGURATION_GPSLCE |
          EQOS_MAC_CONFIGURATION_WD |
          EQOS_MAC_CONFIGURATION_JD |
          EQOS_MAC_CONFIGURATION_JE);

    /* PLSEN is set to 1 so that LPI is not initiated */
    // MAC_LPS_PLSEN_WR(1); << this macro below
    uint32_t v = eqos->mac_regs->unused_0ac[9];
    v = (v & (MAC_LPS_RES_WR_MASK_20)) | (((0) & (MAC_LPS_MASK_20)) << 20);
    v = (v & (MAC_LPS_RES_WR_MASK_10)) | (((0) & (MAC_LPS_MASK_10)) << 10);
    v = (v & (MAC_LPS_RES_WR_MASK_4)) | (((0) & (MAC_LPS_MASK_4)) << 4);
    v = ((v & MAC_LPS_PLSEN_WR_MASK) | ((1 & MAC_LPS_PLSEN_MASK) << 18));
    eqos->mac_regs->unused_0ac[9] = v;

    /* Update the MAC address */
    memcpy(eqos->enetaddr, TX2_DEFAULT_MAC, 6);
    uint32_t val1 = (eqos->enetaddr[5] << 8) | (eqos->enetaddr[4]);
    eqos->mac_regs->address0_high = val1;
    val1 = (eqos->enetaddr[3] << 24) | (eqos->enetaddr[2] << 16) |
           (eqos->enetaddr[1] << 8) | (eqos->enetaddr[0]);
    eqos->mac_regs->address0_low = val1;

    eqos->mac_regs->configuration &= 0xffcfff7c;
    eqos->mac_regs->configuration |=  DWCEQOS_MAC_CFG_TE | DWCEQOS_MAC_CFG_RE;

    /* Configure DMA */
    /* Enable OSP mode */
    eqos->dma_regs->ch0_tx_control = EQOS_DMA_CH0_TX_CONTROL_OSP;

    /* RX buffer size. Must be a multiple of bus width */
    eqos->dma_regs->ch0_rx_control = (EQOS_MAX_PACKET_SIZE << EQOS_DMA_CH0_RX_CONTROL_RBSZ_SHIFT);

    eqos->dma_regs->ch0_control = (EQOS_DMA_CH0_CONTROL_PBLX8);

    /*
     * Burst length must be < 1/2 FIFO size.
     * FIFO size in tqs is encoded as (n / 256) - 1.
     * Each burst is n * 8 (PBLX8) * 16 (AXI width) == 128 bytes.
     * Half of n * 256 is n * 128, so pbl == tqs, modulo the -1.
     */
    pbl = tqs + 1;
    if (pbl > 32) {
        pbl = 32;
    }
    eqos->dma_regs->ch0_tx_control &=
        ~(EQOS_DMA_CH0_TX_CONTROL_TXPBL_MASK <<
          EQOS_DMA_CH0_TX_CONTROL_TXPBL_SHIFT);
    eqos->dma_regs->ch0_tx_control |= (pbl << EQOS_DMA_CH0_TX_CONTROL_TXPBL_SHIFT);

    eqos->dma_regs->ch0_rx_control &=
        ~(EQOS_DMA_CH0_RX_CONTROL_RXPBL_MASK <<
          EQOS_DMA_CH0_RX_CONTROL_RXPBL_SHIFT);
    eqos->dma_regs->ch0_rx_control |= (1 << EQOS_DMA_CH0_RX_CONTROL_RXPBL_SHIFT);

    /* DMA performance configuration */
    val = (2 << EQOS_DMA_SYSBUS_MODE_RD_OSR_LMT_SHIFT) |
          EQOS_DMA_SYSBUS_MODE_EAME | EQOS_DMA_SYSBUS_MODE_BLEN16 |
          EQOS_DMA_SYSBUS_MODE_BLEN8;
    eqos->dma_regs->sysbus_mode = val;

    eqos->dma_regs->ch0_txdesc_list_haddress = 0;
    eqos->dma_regs->ch0_txdesc_list_address = d->tx_ring_phys;
    eqos->dma_regs->ch0_txdesc_ring_length = EQOS_DESCRIPTORS_TX - 1;

    eqos->dma_regs->ch0_rxdesc_list_haddress = 0;
    eqos->dma_regs->ch0_rxdesc_list_address = d->rx_ring_phys;
    eqos->dma_regs->ch0_rxdesc_ring_length = EQOS_DESCRIPTORS_RX - 1;

    eqos->dma_regs->ch0_dma_ie = 0;
    eqos->dma_regs->ch0_dma_ie = DWCEQOS_DMA_CH0_IE_RIE | DWCEQOS_DMA_CH0_IE_TIE |
                                 DWCEQOS_DMA_CH0_IE_NIE | DWCEQOS_DMA_CH0_IE_AIE |
                                 DWCEQOS_DMA_CH0_IE_FBEE | DWCEQOS_DMA_CH0_IE_RWTE;
    eqos->dma_regs->ch0_dma_rx_int_wd_timer = 120;
    udelay(100);

    eqos->dma_regs->ch0_tx_control = EQOS_DMA_CH0_TX_CONTROL_ST;
    eqos->dma_regs->ch0_rx_control = EQOS_DMA_CH0_RX_CONTROL_SR;

    eqos->last_rx_desc = (d->rx_ring_phys + ((EQOS_DESCRIPTORS_RX) * (uintptr_t)(sizeof(struct eqos_desc))));
    eqos->last_tx_desc = (d->tx_ring_phys + ((EQOS_DESCRIPTORS_TX) * (uintptr_t)(sizeof(struct eqos_desc))));

    /* Disable MMC event counters */
    *(uint32_t *)(eqos->regs + REG_DWCEQOS_ETH_MMC_CONTROL) |= REG_DWCEQOS_MMC_CNTFREEZ;

    return 0;

err_shutdown_phy:
    phy_shutdown(eqos->phy);
err_stop_resets:
    // eqos_stop_resets_tegra186(dev);
err_stop_clks:
    // eqos_stop_clks_tegra186(dev);
err:
    return ret;
}

static int hardware_interface_searcher(void *handler_data, void *interface_instance, char **properties)
{

    /* For now, just take the first one that appears, note that we pass the
     * pointer of each individual subsystem as the cookie. */
    *((void **) handler_data) = interface_instance;
    return PS_INTERFACE_FOUND_MATCH;
}

static int tx2_initialise_hardware(struct eqos_priv *eqos)
{
    if (!eqos) {
        ZF_LOGE("eqos is NULL");
        return -EINVAL;
    }

    bool found_clock_interface = false;
    bool found_reset_interface = false;
    bool found_gpio_interface = false;

    /* Check if a clock, reset, and gpio interface was registered, if not
     * initialise them ourselves */
    int error = ps_interface_find(&eqos->tx2_io_ops->interface_registration_ops,
                                  PS_CLOCK_INTERFACE, hardware_interface_searcher, &eqos->clock_sys);
    if (!error) {
        found_clock_interface = true;
    }

    error = ps_interface_find(&eqos->tx2_io_ops->interface_registration_ops,
                              PS_RESET_INTERFACE, hardware_interface_searcher, &eqos->reset_sys);
    if (!error) {
        found_reset_interface = true;
    }

    error = ps_interface_find(&eqos->tx2_io_ops->interface_registration_ops,
                              PS_GPIO_INTERFACE, hardware_interface_searcher, &eqos->gpio_sys);
    if (!error) {
        found_gpio_interface = true;
    }

    if (found_clock_interface && found_reset_interface && found_gpio_interface) {
        return 0;
    }

    if (!found_clock_interface) {
        ZF_LOGW("Did not found a suitable clock interface, going to be initialising our own");
        error = ps_calloc(&eqos->tx2_io_ops->malloc_ops, 1, sizeof(*(eqos->clock_sys)),
                          (void **) &eqos->clock_sys);
        if (error) {
            /* Too early to be cleaning up anything */
            return error;
        }
        error = clock_sys_init(eqos->tx2_io_ops, eqos->clock_sys);
        if (error) {
            goto fail;
        }
    }

    if (!found_reset_interface) {
        ZF_LOGW("Did not found a suitable reset interface, going to be initialising our own");
        error = ps_calloc(&eqos->tx2_io_ops->malloc_ops, 1, sizeof(*(eqos->reset_sys)),
                          (void **) &eqos->reset_sys);
        if (error) {
            goto fail;
        }
        error = reset_sys_init(eqos->tx2_io_ops, NULL, eqos->reset_sys);
        if (error) {
            goto fail;
        }
    }

    if (!found_gpio_interface) {
        ZF_LOGW("Did not found a suitable gpio interface, going to be initialising our own");
        error = ps_calloc(&eqos->tx2_io_ops->malloc_ops, 1, sizeof(*(eqos->gpio_sys)),
                          (void **) &eqos->gpio_sys);
        if (error) {
            goto fail;
        }
        error = gpio_sys_init(eqos->tx2_io_ops, eqos->gpio_sys);
        if (error) {
            goto fail;
        }
    }

    return 0;

fail:

    if (eqos->clock_sys) {
        ZF_LOGF_IF(ps_free(&eqos->tx2_io_ops->malloc_ops, sizeof(*(eqos->clock_sys)), eqos->clock_sys),
                   "Failed to clean up the clock interface after a failed initialisation process");
    }

    if (eqos->reset_sys) {
        ZF_LOGF_IF(ps_free(&eqos->tx2_io_ops->malloc_ops, sizeof(*(eqos->reset_sys)), eqos->reset_sys),
                   "Failed to clean up the reset interface after a failed initialisation process");
    }

    if (eqos->gpio_sys) {
        ZF_LOGF_IF(ps_free(&eqos->tx2_io_ops->malloc_ops, sizeof(*(eqos->gpio_sys)), eqos->gpio_sys),
                   "Failed to clean up the gpio interface after a failed initialisation process");
    }

    return error;
}

void *tx2_initialise(uintptr_t base_addr, ps_io_ops_t *io_ops)
{
    struct eqos_priv *eqos;
    int ret;

    if (io_ops == NULL) {
        return NULL;
    }

    eqos = calloc(1, sizeof(struct eqos_priv));
    if (eqos == NULL) {
        free(eqos);
        return NULL;
    }
    eqos->tx2_io_ops = io_ops;

    /* initialise miiphy */
    miiphy_init();

    /* initialise phy */
    ret = phy_init();
    if (ret != 0) {
        ZF_LOGF("failed to initialise phy");
    }

    ret = tx2_initialise_hardware(eqos);
    if (ret) {
        return NULL;
    }

    /* initialise the phy reset gpio gpio */
    ret = eqos->gpio_sys->init(eqos->gpio_sys, GPIO_PM4, GPIO_DIR_OUT, &eqos->gpio);
    if (ret != 0) {
        ZF_LOGF("failed to init phy reset gpio pin");
    }

    eqos->config = &eqos_tegra186_config;

    eqos->regs = base_addr;

    /* allocate register structs and mdio */
    assert((eqos->regs >> 32) == 0);
    eqos->mac_regs = (void *)(eqos->regs + EQOS_MAC_REGS_BASE);
    eqos->mtl_regs = (void *)(eqos->regs + EQOS_MTL_REGS_BASE);
    eqos->dma_regs = (void *)(eqos->regs + EQOS_DMA_REGS_BASE);
    eqos->tegra186_regs = (void *)(eqos->regs + EQOS_TEGRA186_REGS_BASE);

    eqos->mii = mdio_alloc();
    if (!eqos->mii) {
        ZF_LOGF("Mdio alloc failed");
        goto err;
    }
    eqos->mii->read = eqos_mdio_read;
    eqos->mii->write = eqos_mdio_write;
    eqos->mii->priv = eqos;
    strcpy(eqos->mii->name, "mii\0");

    ret = mdio_register(eqos->mii);
    if (ret < 0) {
        ZF_LOGE("Mdio register failed");
        goto err_free_mdio;
    }

    return (void *)eqos;
err_free_mdio:
    mdio_free(eqos->mii);
err:
    ZF_LOGE("Tx2 initialise failed");
    return NULL;
}
