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
    uint32_t ch0_dma_rx_int_wd_timer;                    /* 0x1138 */
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
#define DWCEQOS_DMA_CH0_IE_RWTE          BIT(9)

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
