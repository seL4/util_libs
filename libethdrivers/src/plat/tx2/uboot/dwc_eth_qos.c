/*
 * @TAG(OTHER_GPL)
 */
// SPDX-License-Identifier: GPL-2.0
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

#define EQOS_MAC_REGS_BASE 0x000
struct eqos_mac_regs {
    uint32_t configuration;             /* 0x000 */
    uint32_t unused_004[(0x070 - 0x004) / 4];   /* 0x004 */
    uint32_t q0_tx_flow_ctrl;           /* 0x070 */
    uint32_t unused_070[(0x090 - 0x074) / 4];   /* 0x074 */
    uint32_t rx_flow_ctrl;              /* 0x090 */
    uint32_t unused_094;                /* 0x094 */
    uint32_t txq_prty_map0;             /* 0x098 */
    uint32_t unused_09c;                /* 0x09c */
    uint32_t rxq_ctrl0;             /* 0x0a0 */
    uint32_t rxq_ctrl1;                /* 0x0a4 */
    uint32_t rxq_ctrl2;             /* 0x0a8 */
    uint32_t unused_0ac[(0x0dc - 0x0ac) / 4];   /* 0x0ac */
    uint32_t us_tic_counter;            /* 0x0dc */
    uint32_t unused_0e0[(0x11c - 0x0e0) / 4];   /* 0x0e0 */
    uint32_t hw_feature0;               /* 0x11c */
    uint32_t hw_feature1;               /* 0x120 */
    uint32_t hw_feature2;               /* 0x124 */
    uint32_t unused_128[(0x200 - 0x128) / 4];   /* 0x128 */
    uint32_t mdio_address;              /* 0x200 */
    uint32_t mdio_data;             /* 0x204 */
    uint32_t unused_208[(0x300 - 0x208) / 4];   /* 0x208 */
    uint32_t address0_high;             /* 0x300 */
    uint32_t address0_low;              /* 0x304 */
};

#define EQOS_MAC_CONFIGURATION_GPSLCE           BIT(23)
#define EQOS_MAC_CONFIGURATION_CST          BIT(21)
#define EQOS_MAC_CONFIGURATION_ACS          BIT(20)
#define EQOS_MAC_CONFIGURATION_WD           BIT(19)
#define EQOS_MAC_CONFIGURATION_JD           BIT(17)
#define EQOS_MAC_CONFIGURATION_JE           BIT(16)
#define EQOS_MAC_CONFIGURATION_PS           BIT(15)
#define EQOS_MAC_CONFIGURATION_FES          BIT(14)
#define EQOS_MAC_CONFIGURATION_DM           BIT(13)
#define EQOS_MAC_CONFIGURATION_TE           BIT(1)
#define EQOS_MAC_CONFIGURATION_RE           BIT(0)

#define EQOS_MAC_Q0_TX_FLOW_CTRL_PT_SHIFT       16
#define EQOS_MAC_Q0_TX_FLOW_CTRL_PT_MASK        0xffff
#define EQOS_MAC_Q0_TX_FLOW_CTRL_TFE            BIT(1)

#define EQOS_MAC_RX_FLOW_CTRL_RFE           BIT(0)

#define EQOS_MAC_TXQ_PRTY_MAP0_PSTQ0_SHIFT      0
#define EQOS_MAC_TXQ_PRTY_MAP0_PSTQ0_MASK       0xff

#define EQOS_MAC_RXQ_CTRL0_RXQ0EN_SHIFT         0
#define EQOS_MAC_RXQ_CTRL0_RXQ0EN_MASK          3
#define EQOS_MAC_RXQ_CTRL0_RXQ0EN_NOT_ENABLED       0
#define EQOS_MAC_RXQ_CTRL0_RXQ0EN_ENABLED_DCB       2
#define EQOS_MAC_RXQ_CTRL0_RXQ0EN_ENABLED_AV        1

#define EQOS_MAC_RXQ_CTRL2_PSRQ0_SHIFT          0
#define EQOS_MAC_RXQ_CTRL2_PSRQ0_MASK           0xff

#define EQOS_MAC_HW_FEATURE1_TXFIFOSIZE_SHIFT       6
#define EQOS_MAC_HW_FEATURE1_TXFIFOSIZE_MASK        0x1f
#define EQOS_MAC_HW_FEATURE1_RXFIFOSIZE_SHIFT       0
#define EQOS_MAC_HW_FEATURE1_RXFIFOSIZE_MASK        0x1f

#define EQOS_MAC_MDIO_ADDRESS_PA_SHIFT          21
#define EQOS_MAC_MDIO_ADDRESS_RDA_SHIFT         16
#define EQOS_MAC_MDIO_ADDRESS_CR_SHIFT          8
#define EQOS_MAC_MDIO_ADDRESS_CR_20_35          2
#define EQOS_MAC_MDIO_ADDRESS_CR_250_300        5
#define EQOS_MAC_MDIO_ADDRESS_SKAP          BIT(4)
#define EQOS_MAC_MDIO_ADDRESS_GOC_SHIFT         2
#define EQOS_MAC_MDIO_ADDRESS_GOC_READ          3
#define EQOS_MAC_MDIO_ADDRESS_GOC_WRITE         1
#define EQOS_MAC_MDIO_ADDRESS_C45E          BIT(1)
#define EQOS_MAC_MDIO_ADDRESS_GB            BIT(0)

#define EQOS_MAC_MDIO_DATA_GD_MASK          0xffff

#define EQOS_MTL_REGS_BASE 0xd00
struct eqos_mtl_regs {
    uint32_t txq0_operation_mode;           /* 0xd00 */
    uint32_t unused_d04;                /* 0xd04 */
    uint32_t txq0_debug;                /* 0xd08 */
    uint32_t unused_d0c[(0xd18 - 0xd0c) / 4];   /* 0xd0c */
    uint32_t txq0_quantum_weight;           /* 0xd18 */
    uint32_t unused_d1c[(0xd30 - 0xd1c) / 4];   /* 0xd1c */
    uint32_t rxq0_operation_mode;           /* 0xd30 */
    uint32_t unused_d34;                /* 0xd34 */
    uint32_t rxq0_debug;                /* 0xd38 */
};

#define EQOS_MTL_TXQ0_OPERATION_MODE_TQS_SHIFT      16
#define EQOS_MTL_TXQ0_OPERATION_MODE_TQS_MASK       0x1ff
#define EQOS_MTL_TXQ0_OPERATION_MODE_TXQEN_SHIFT    2
#define EQOS_MTL_TXQ0_OPERATION_MODE_TXQEN_MASK     3
#define EQOS_MTL_TXQ0_OPERATION_MODE_TXQEN_ENABLED  2
#define EQOS_MTL_TXQ0_OPERATION_MODE_TSF        BIT(1)
#define EQOS_MTL_TXQ0_OPERATION_MODE_FTQ        BIT(0)

#define EQOS_MTL_RXQ0_OPERATION_MODE_RQS_SHIFT      20
#define EQOS_MTL_RXQ0_OPERATION_MODE_RQS_MASK       0x3ff
#define EQOS_MTL_RXQ0_OPERATION_MODE_RFD_SHIFT      14
#define EQOS_MTL_RXQ0_OPERATION_MODE_RFD_MASK       0x3f
#define EQOS_MTL_RXQ0_OPERATION_MODE_RFA_SHIFT      8
#define EQOS_MTL_RXQ0_OPERATION_MODE_RFA_MASK       0x3f
#define EQOS_MTL_RXQ0_OPERATION_MODE_EHFC       BIT(7)
#define EQOS_MTL_RXQ0_OPERATION_MODE_RSF        BIT(5)

#define EQOS_DMA_REGS_BASE 0x1000
struct eqos_dma_regs {
    uint32_t mode;                  /* 0x1000 */
    uint32_t sysbus_mode;               /* 0x1004 */
    uint32_t dma_control[(0x1100 - 0x1008) / 4];    /* 0x1008 */
    uint32_t ch0_control;               /* 0x1100 */
    uint32_t ch0_tx_control;            /* 0x1104 */
    uint32_t ch0_rx_control;            /* 0x1108 */
    uint32_t unused_110c;               /* 0x110c */
    uint32_t ch0_txdesc_list_haddress;      /* 0x1110 */
    uint32_t ch0_txdesc_list_address;       /* 0x1114 */
    uint32_t ch0_rxdesc_list_haddress;      /* 0x1118 */
    uint32_t ch0_rxdesc_list_address;       /* 0x111c */
    uint32_t ch0_txdesc_tail_pointer;       /* 0x1120 */
    uint32_t unused_1124;               /* 0x1124 */
    uint32_t ch0_rxdesc_tail_pointer;       /* 0x1128 */
    uint32_t ch0_txdesc_ring_length;        /* 0x112c */
    uint32_t ch0_rxdesc_ring_length;        /* 0x1130 */
    uint32_t ch0_dma_ie;                    /* 0x1134 */
};

#define EQOS_DMA_MODE_SWR               BIT(0)

#define EQOS_DMA_SYSBUS_MODE_RD_OSR_LMT_SHIFT       16
#define EQOS_DMA_SYSBUS_MODE_RD_OSR_LMT_MASK        0xf
#define EQOS_DMA_SYSBUS_MODE_EAME           BIT(11)
#define EQOS_DMA_SYSBUS_MODE_BLEN16         BIT(3)
#define EQOS_DMA_SYSBUS_MODE_BLEN8          BIT(2)
#define EQOS_DMA_SYSBUS_MODE_BLEN4          BIT(1)

#define EQOS_DMA_CH0_CONTROL_PBLX8          BIT(16)

#define EQOS_DMA_CH0_TX_CONTROL_TXPBL_SHIFT     16
#define EQOS_DMA_CH0_TX_CONTROL_TXPBL_MASK      0x3f
#define EQOS_DMA_CH0_TX_CONTROL_OSP         BIT(4)
#define EQOS_DMA_CH0_TX_CONTROL_ST          BIT(0)

#define EQOS_DMA_CH0_RX_CONTROL_RXPBL_SHIFT     16
#define EQOS_DMA_CH0_RX_CONTROL_RXPBL_MASK      0x3f
#define EQOS_DMA_CH0_RX_CONTROL_RBSZ_SHIFT      1
#define EQOS_DMA_CH0_RX_CONTROL_RBSZ_MASK       0x3fff
#define EQOS_DMA_CH0_RX_CONTROL_SR          BIT(0)
#define DWCEQOS_DMA_CH_CTRL_START           BIT(0)
/* These registers are Tegra186-specific */
#define EQOS_TEGRA186_REGS_BASE 0x8800
struct eqos_tegra186_regs {
    uint32_t sdmemcomppadctrl;          /* 0x8800 */
    uint32_t auto_cal_config;           /* 0x8804 */
    uint32_t unused_8808;               /* 0x8808 */
    uint32_t auto_cal_status;           /* 0x880c */
};

#define EQOS_SDMEMCOMPPADCTRL_PAD_E_INPUT_OR_E_PWRD BIT(31)

#define EQOS_AUTO_CAL_CONFIG_START          BIT(31)
#define EQOS_AUTO_CAL_CONFIG_ENABLE         BIT(29)
#define EQOS_AUTO_CAL_STATUS_ACTIVE         BIT(31)

#define EQOS_DESCRIPTOR_WORDS   4
#define EQOS_DESCRIPTOR_SIZE    (EQOS_DESCRIPTOR_WORDS * 4)
/* We assume ARCH_DMA_MINALIGN >= 16; 16 is the EQOS HW minimum */
#define EQOS_DESCRIPTOR_ALIGN   ARCH_DMA_MINALIGN
#define EQOS_DESCRIPTORS_NUM    (EQOS_DESCRIPTORS_TX + EQOS_DESCRIPTORS_RX)
#define EQOS_DESCRIPTORS_SIZE   EQOS_ALIGN(EQOS_DESCRIPTORS_NUM * \
                      EQOS_DESCRIPTOR_SIZE, ARCH_DMA_MINALIGN)
#define EQOS_BUFFER_ALIGN   ARCH_DMA_MINALIGN

//from linux
// #define EQOS_RX_BUFFER_SIZE  2048
#define EQOS_RX_BUFFER_SIZE (EQOS_DESCRIPTORS_RX * EQOS_MAX_PACKET_SIZE)

struct eqos_config {
    bool reg_access_always_ok;
    int mdio_wait;
    int swr_wait;
    int config_mac;
    int config_mac_mdio;
};

/* ARP hardware address length */
#define ARP_HLEN 6

struct eqos_priv {
    const struct eqos_config *config;
    uintptr_t regs;
    struct eqos_mac_regs *mac_regs;
    struct eqos_mtl_regs *mtl_regs;
    struct eqos_dma_regs *dma_regs;
    struct eqos_tegra186_regs *tegra186_regs;
    struct clock *clk_master_bus;
    struct clock *clk_rx;
    struct clock *clk_ptp_ref;
    struct clock *clk_tx;
    struct clock *clk_slave_bus;
    struct mii_dev *mii;
    struct phy_device *phy;
    uintptr_t last_rx_desc;
    uintptr_t last_tx_desc;
    unsigned char enetaddr[ARP_HLEN];
    bool reg_access_ok;
    ps_io_ops_t *tx2_io_ops;
    gpio_sys_t *gpio_sys;
    gpio_t gpio;
    reset_sys_t *reset_sys;
    clock_sys_t *clock_sys;
};

#define REG_DWCEQOS_DMA_CH0_STA          0x1160
#define DWCEQOS_DMA_IS_DC0IS             BIT(0)
#define DWCEQOS_DMA_IS_MTLIS             BIT(16)
#define DWCEQOS_DMA_IS_MACIS             BIT(17)
#define DWCEQOS_DMA_CH0_IS_TI            BIT(0)
#define DWCEQOS_DMA_CH0_IS_RI            BIT(6)
#define DWCEQOS_MAC_IS_MMC_INT           BIT(8)
#define DWCEQOS_MAC_IS_LPI_INT           BIT(5)

#define DWCEQOS_DMA_CH0_IE_NIE           BIT(15)
#define DWCEQOS_DMA_CH0_IE_AIE           BIT(14)
#define DWCEQOS_DMA_CH0_IE_RIE           BIT(6)
#define DWCEQOS_DMA_CH0_IE_TIE           BIT(0)
#define DWCEQOS_DMA_CH0_IE_FBEE          BIT(12)
#define DWCEQOS_DMA_CH0_IE_RBUE          BIT(7)

#define MAC_LPS_RES_WR_MASK_20 (uint32_t)(0xfffff)
#define  MAC_LPS_MASK_20 (uint32_t)(0xfff)
#define  MAC_LPS_MASK_10 (uint32_t)(0x3f)
#define MAC_LPS_RES_WR_MASK_10 (uint32_t)(0xffff03ff)
#define  MAC_LPS_MASK_4 (uint32_t)(0xf)
#define MAC_LPS_RES_WR_MASK_4 (uint32_t)(0xffffff0f)
#define MAC_LPS_PLSEN_MASK (uint32_t)(0x1)
#define MAC_LPS_PLSEN_WR_MASK (uint32_t)(0xfffbffff)

#define DWCEQOS_MAC_CFG_ACS              BIT(20)
#define DWCEQOS_MAC_CFG_JD               BIT(17)
#define DWCEQOS_MAC_CFG_JE               BIT(16)
#define DWCEQOS_MAC_CFG_PS               BIT(15)
#define DWCEQOS_MAC_CFG_FES              BIT(14)
/* full duplex mode? */
#define DWCEQOS_MAC_CFG_DM               BIT(13)
#define DWCEQOS_MAC_CFG_DO               BIT(10)
#define DWCEQOS_MAC_CFG_TE               BIT(1)
/* Check sum bit */
#define DWCEQOS_MAC_CFG_IPC              BIT(27)
#define DWCEQOS_MAC_CFG_RE               BIT(0)

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
}

void ack_rx(struct tx2_eth_data *dev)
{
    struct eqos_priv *eqos = (struct eqos_priv *)dev->eth_dev;
    uint32_t *dma_status = (uint32_t *)(eqos->regs + REG_DWCEQOS_DMA_CH0_STA);
    *dma_status |= DWCEQOS_DMA_CH0_IS_RI;

    uintptr_t last_rx_desc = (dev->rx_ring_phys + ((EQOS_DESCRIPTORS_RX) * (uintptr_t)(sizeof(struct eqos_desc))));
    eqos->dma_regs->ch0_rxdesc_tail_pointer = last_rx_desc;
}

int eqos_handle_irq(struct tx2_eth_data *dev, int irq)
{
    struct eqos_priv *eqos = (struct eqos_priv *)dev->eth_dev;

    uint32_t cause = eqos->dma_regs->dma_control[0];
    uint32_t *dma_status;
    int ret = 0;

    if (cause & DWCEQOS_DMA_IS_DC0IS) {
        dma_status = (uint32_t *)(eqos->regs + REG_DWCEQOS_DMA_CH0_STA);

        /* Transmit Interrupt currently polling tx so should never get here */
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
    }
    clk_gate_enable(eqos->clock_sys, CLK_AXI_CBB, CLKGATE_ON);


    eqos->clk_master_bus = clk_get_clock(eqos->clock_sys, CLK_GATE_EQOS_AXI);
    if (eqos->clk_master_bus == NULL) {
        ZF_LOGE("clk_get_clock failed CLK_MASTER_BUS");
    }
    clk_gate_enable(eqos->clock_sys, CLK_GATE_EQOS_AXI, CLKGATE_ON);

    eqos->clk_rx = clk_get_clock(eqos->clock_sys, CLK_EQOS_RX_INPUT);
    if (eqos->clk_rx == NULL) {
        ZF_LOGE("clk_get_clock failed CLK_RX");
    }
    clk_gate_enable(eqos->clock_sys, CLK_GATE_EQOS_RX, CLKGATE_ON);

    eqos->clk_ptp_ref = clk_get_clock(eqos->clock_sys, CLK_GATE_EQOS_PTP_REF);
    if (eqos->clk_ptp_ref == NULL) {
        ZF_LOGE("clk_get_clock failed CLK_PTP_REF");
    }
    clk_gate_enable(eqos->clock_sys, CLK_GATE_EQOS_PTP_REF, CLKGATE_ON);

    eqos->clk_tx = clk_get_clock(eqos->clock_sys, CLK_EQOS_TX);
    if (eqos->clk_tx == NULL) {
        ZF_LOGE("clk_get_clock failed CLK_TX");
    }
    clk_gate_enable(eqos->clock_sys, CLK_GATE_EQOS_TX, CLKGATE_ON);

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

static freq_t eqos_get_tick_clk_rate_tegra186(struct eqos_priv *eqos)
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
    ret = eqos_set_tx_clk_speed_tegra186(eqos);
    if (ret < 0) {
        ZF_LOGE("eqos_set_tx_clk_speed() failed: %d", ret);
        return ret;
    }

    return 0;
}

int eqos_send(struct tx2_eth_data *dev, void *packet, int length)
{
    struct eqos_priv *eqos = (struct eqos_priv *)dev->eth_dev;
    volatile struct eqos_desc *tx_desc;
    int i;

    tx_desc = &(dev->tx_ring[dev->tdt]);
    dev->tdt++;
    dev->tdt %= EQOS_DESCRIPTORS_TX;

    tx_desc->des0 = (uintptr_t)packet;
    tx_desc->des1 = 0;
    tx_desc->des2 = EQOS_DESC2_IOC | length;
    tx_desc->des3 = EQOS_DESC3_FD | EQOS_DESC3_LD | length;

    __sync_synchronize();

    tx_desc->des3 |= EQOS_DESC3_OWN;

    eqos->dma_regs->ch0_txdesc_tail_pointer = eqos->last_tx_desc;

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
    int ret, i;
    ulong rate;
    u32 val, tx_fifo_sz, rx_fifo_sz, tqs, rqs, pbl;
    uintptr_t last_rx_desc;

    eqos->reg_access_ok = true;
    uint32_t *dma_ie;

    ret = eqos_start_clks_tegra186(eqos);
    if (ret) {
        ZF_LOGF("clocks start failed");
    }

    ret = eqos_start_resets_tegra186(eqos);
    if (ret) {
        ZF_LOGF("clocks start failed");
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

    rate = eqos_get_tick_clk_rate_tegra186(eqos);
    val = (rate / 1000000) - 1;
    writel(val, &eqos->mac_regs->us_tic_counter);

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

    /* Virt channel stuff from the l4t driver
     * dma_ie = (eqos->regs + 0x8600);
     * *dma_ie = 3;
     * dma_ie = (eqos->regs + 0x8604);
     * *dma_ie = 3;
     */

    /* Configure MAC, not sure if L4T is the same */
    eqos->mac_regs->rxq_ctrl0 =
        (eqos->config->config_mac <<
         EQOS_MAC_RXQ_CTRL0_RXQ0EN_SHIFT);

    /* Set TX flow control parameters */
    /* Set Pause Time */
    eqos->mac_regs->q0_tx_flow_ctrl = (0xffff << EQOS_MAC_Q0_TX_FLOW_CTRL_PT_SHIFT);
    /* Assign priority for TX flow control */
    eqos->mac_regs->txq_prty_map0 =
        (EQOS_MAC_TXQ_PRTY_MAP0_PSTQ0_MASK <<
         EQOS_MAC_TXQ_PRTY_MAP0_PSTQ0_SHIFT);
    /* Assign priority for RX flow control */
    eqos->mac_regs->rxq_ctrl2 =
        (EQOS_MAC_RXQ_CTRL2_PSRQ0_MASK <<
         EQOS_MAC_RXQ_CTRL2_PSRQ0_SHIFT);

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
    eqos->mac_regs->configuration |= DWCEQOS_MAC_CFG_ACS | BIT(21)
                                     | DWCEQOS_MAC_CFG_TE | DWCEQOS_MAC_CFG_RE;

    /* Enable interrupts mac */
    dma_ie = (uint32_t *)(eqos->regs + 0x00b4);
    uint32_t mac_imr_val = 0;
    mac_imr_val = *dma_ie;
    mac_imr_val &= (uint32_t)(0x1008);
    mac_imr_val |= ((0x1) << 0) | ((0x1) << 1) | ((0x1) << 2) |
                   ((0x1) << 4) | ((0x1) << 5);
    *dma_ie = mac_imr_val;

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
    eqos->dma_regs->ch0_rx_control |= (8 << EQOS_DMA_CH0_RX_CONTROL_RXPBL_SHIFT);

    /* DMA performance configuration */
    val = (2 << EQOS_DMA_SYSBUS_MODE_RD_OSR_LMT_SHIFT) |
          EQOS_DMA_SYSBUS_MODE_EAME | EQOS_DMA_SYSBUS_MODE_BLEN16 |
          EQOS_DMA_SYSBUS_MODE_BLEN8 | EQOS_DMA_SYSBUS_MODE_BLEN4;
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
                                 DWCEQOS_DMA_CH0_IE_FBEE;

    udelay(100);

    eqos->dma_regs->ch0_tx_control = EQOS_DMA_CH0_TX_CONTROL_ST;
    eqos->dma_regs->ch0_rx_control = EQOS_DMA_CH0_RX_CONTROL_SR;

    eqos->last_rx_desc = (d->rx_ring_phys + ((EQOS_DESCRIPTORS_RX) * (uintptr_t)(sizeof(struct eqos_desc))));
    eqos->last_tx_desc = (d->tx_ring_phys + ((EQOS_DESCRIPTORS_TX) * (uintptr_t)(sizeof(struct eqos_desc))));

    eqos->dma_regs->ch0_rxdesc_tail_pointer = eqos->last_rx_desc;
    eqos->dma_regs->ch0_txdesc_tail_pointer = eqos->last_tx_desc;

    return 0;

err_shutdown_phy:
    phy_shutdown(eqos->phy);
err_stop_resets:
    // eqos_stop_resets_tegra186(dev);
err_stop_clks:
    // eqos_stop_clks_tegra186(dev);
err:
    ZF_LOGE("FAILED: %d", ret);
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
