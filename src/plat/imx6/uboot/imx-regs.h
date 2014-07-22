/*
 * @TAG(OTHER_GPL)
 */

/*
 * Copyright (C) 2011 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __ASM_ARCH_MX6_IMX_REGS_H__
#define __ASM_ARCH_MX6_IMX_REGS_H__

#include <stdint.h>

#define ARCH_MXC

#define CONFIG_SYS_CACHELINE_SIZE	32

#define ROMCP_ARB_BASE_ADDR             0x00000000
#define ROMCP_ARB_END_ADDR              0x000FFFFF
#define CAAM_ARB_BASE_ADDR              0x00100000
#define CAAM_ARB_END_ADDR               0x00103FFF
#define APBH_DMA_ARB_BASE_ADDR          0x00110000
#define APBH_DMA_ARB_END_ADDR           0x00117FFF
#define HDMI_ARB_BASE_ADDR              0x00120000
#define HDMI_ARB_END_ADDR               0x00128FFF
#define GPU_3D_ARB_BASE_ADDR            0x00130000
#define GPU_3D_ARB_END_ADDR             0x00133FFF
#define GPU_2D_ARB_BASE_ADDR            0x00134000
#define GPU_2D_ARB_END_ADDR             0x00137FFF
#define DTCP_ARB_BASE_ADDR              0x00138000
#define DTCP_ARB_END_ADDR               0x0013BFFF

/* GPV - PL301 configuration ports */
#define GPV2_BASE_ADDR			0x00200000
#define GPV3_BASE_ADDR			0x00300000
#define GPV4_BASE_ADDR			0x00800000
#define IRAM_BASE_ADDR			0x00900000
#define SCU_BASE_ADDR                   0x00A00000
#define IC_INTERFACES_BASE_ADDR         0x00A00100
#define GLOBAL_TIMER_BASE_ADDR          0x00A00200
#define PRIVATE_TIMERS_WD_BASE_ADDR     0x00A00600
#define IC_DISTRIBUTOR_BASE_ADDR        0x00A01000
#define GPV0_BASE_ADDR                  0x00B00000
#define GPV1_BASE_ADDR                  0x00C00000
#define PCIE_ARB_BASE_ADDR              0x01000000
#define PCIE_ARB_END_ADDR               0x01FFFFFF

#define AIPS1_ARB_BASE_ADDR             0x02000000
#define AIPS1_ARB_END_ADDR              0x020FFFFF
#define AIPS2_ARB_BASE_ADDR             0x02100000
#define AIPS2_ARB_END_ADDR              0x021FFFFF
#define SATA_ARB_BASE_ADDR              0x02200000
#define SATA_ARB_END_ADDR               0x02203FFF
#define OPENVG_ARB_BASE_ADDR            0x02204000
#define OPENVG_ARB_END_ADDR             0x02207FFF
#define HSI_ARB_BASE_ADDR               0x02208000
#define HSI_ARB_END_ADDR                0x0220BFFF
#define IPU1_ARB_BASE_ADDR              0x02400000
#define IPU1_ARB_END_ADDR               0x027FFFFF
#define IPU2_ARB_BASE_ADDR              0x02800000
#define IPU2_ARB_END_ADDR               0x02BFFFFF
#define WEIM_ARB_BASE_ADDR              0x08000000
#define WEIM_ARB_END_ADDR               0x0FFFFFFF

#define MMDC0_ARB_BASE_ADDR             0x10000000
#define MMDC0_ARB_END_ADDR              0x7FFFFFFF
#define MMDC1_ARB_BASE_ADDR             0x80000000
#define MMDC1_ARB_END_ADDR              0xFFFFFFFF

#define IPU_SOC_BASE_ADDR		IPU1_ARB_BASE_ADDR
#define IPU_SOC_OFFSET			0x00200000

/* Defines for Blocks connected via AIPS (SkyBlue) */
#define ATZ1_BASE_ADDR              AIPS1_ARB_BASE_ADDR
#define ATZ2_BASE_ADDR              AIPS2_ARB_BASE_ADDR
#define AIPS1_BASE_ADDR             AIPS1_ON_BASE_ADDR
#define AIPS2_BASE_ADDR             AIPS2_ON_BASE_ADDR

#define SPDIF_BASE_ADDR             (ATZ1_BASE_ADDR + 0x04000)
#define ECSPI1_BASE_ADDR            (ATZ1_BASE_ADDR + 0x08000)
#define ECSPI2_BASE_ADDR            (ATZ1_BASE_ADDR + 0x0C000)
#define ECSPI3_BASE_ADDR            (ATZ1_BASE_ADDR + 0x10000)
#define ECSPI4_BASE_ADDR            (ATZ1_BASE_ADDR + 0x14000)
#define ECSPI5_BASE_ADDR            (ATZ1_BASE_ADDR + 0x18000)
#define UART1_BASE                  (ATZ1_BASE_ADDR + 0x20000)
#define ESAI1_BASE_ADDR             (ATZ1_BASE_ADDR + 0x24000)
#define SSI1_BASE_ADDR              (ATZ1_BASE_ADDR + 0x28000)
#define SSI2_BASE_ADDR              (ATZ1_BASE_ADDR + 0x2C000)
#define SSI3_BASE_ADDR              (ATZ1_BASE_ADDR + 0x30000)
#define ASRC_BASE_ADDR              (ATZ1_BASE_ADDR + 0x34000)
#define SPBA_BASE_ADDR              (ATZ1_BASE_ADDR + 0x3C000)
#define VPU_BASE_ADDR               (ATZ1_BASE_ADDR + 0x40000)
#define AIPS1_ON_BASE_ADDR          (ATZ1_BASE_ADDR + 0x7C000)

#define AIPS1_OFF_BASE_ADDR         (ATZ1_BASE_ADDR + 0x80000)
#define PWM1_BASE_ADDR              (AIPS1_OFF_BASE_ADDR + 0x0000)
#define PWM2_BASE_ADDR              (AIPS1_OFF_BASE_ADDR + 0x4000)
#define PWM3_BASE_ADDR              (AIPS1_OFF_BASE_ADDR + 0x8000)
#define PWM4_BASE_ADDR              (AIPS1_OFF_BASE_ADDR + 0xC000)
#define CAN1_BASE_ADDR              (AIPS1_OFF_BASE_ADDR + 0x10000)
#define CAN2_BASE_ADDR              (AIPS1_OFF_BASE_ADDR + 0x14000)
#define GPT1_BASE_ADDR              (AIPS1_OFF_BASE_ADDR + 0x18000)
#define GPIO1_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x1C000)
#define GPIO2_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x20000)
#define GPIO3_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x24000)
#define GPIO4_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x28000)
#define GPIO5_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x2C000)
#define GPIO6_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x30000)
#define GPIO7_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x34000)
#define KPP_BASE_ADDR               (AIPS1_OFF_BASE_ADDR + 0x38000)
#define WDOG1_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x3C000)
#define WDOG2_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x40000)
#define ANATOP_BASE_ADDR            (AIPS1_OFF_BASE_ADDR + 0x48000)
#define USB_PHY0_BASE_ADDR          (AIPS1_OFF_BASE_ADDR + 0x49000)
#define USB_PHY1_BASE_ADDR          (AIPS1_OFF_BASE_ADDR + 0x4a000)
#define CCM_BASE_ADDR               (AIPS1_OFF_BASE_ADDR + 0x44000)
#define SNVS_BASE_ADDR              (AIPS1_OFF_BASE_ADDR + 0x4C000)
#define EPIT1_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x50000)
#define EPIT2_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x54000)
#define SRC_BASE_ADDR               (AIPS1_OFF_BASE_ADDR + 0x58000)
#define GPC_BASE_ADDR               (AIPS1_OFF_BASE_ADDR + 0x5C000)
#define IOMUXC_BASE_ADDR            (AIPS1_OFF_BASE_ADDR + 0x60000)
#define DCIC1_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x64000)
#define DCIC2_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x68000)
#define DMA_REQ_PORT_HOST_BASE_ADDR (AIPS1_OFF_BASE_ADDR + 0x6C000)

#define AIPS2_ON_BASE_ADDR          (ATZ2_BASE_ADDR + 0x7C000)
#define AIPS2_OFF_BASE_ADDR         (ATZ2_BASE_ADDR + 0x80000)
#define CAAM_BASE_ADDR              (ATZ2_BASE_ADDR)
#define ARM_BASE_ADDR		    (ATZ2_BASE_ADDR + 0x40000)
#define USBOH3_PL301_BASE_ADDR      (AIPS2_OFF_BASE_ADDR + 0x0000)
#define USBOH3_USB_BASE_ADDR        (AIPS2_OFF_BASE_ADDR + 0x4000)
#define ENET_BASE_ADDR              (AIPS2_OFF_BASE_ADDR + 0x8000)
#define MLB_BASE_ADDR               (AIPS2_OFF_BASE_ADDR + 0xC000)
#define USDHC1_BASE_ADDR            (AIPS2_OFF_BASE_ADDR + 0x10000)
#define USDHC2_BASE_ADDR            (AIPS2_OFF_BASE_ADDR + 0x14000)
#define USDHC3_BASE_ADDR            (AIPS2_OFF_BASE_ADDR + 0x18000)
#define USDHC4_BASE_ADDR            (AIPS2_OFF_BASE_ADDR + 0x1C000)
#define I2C1_BASE_ADDR              (AIPS2_OFF_BASE_ADDR + 0x20000)
#define I2C2_BASE_ADDR              (AIPS2_OFF_BASE_ADDR + 0x24000)
#define I2C3_BASE_ADDR              (AIPS2_OFF_BASE_ADDR + 0x28000)
#define ROMCP_BASE_ADDR             (AIPS2_OFF_BASE_ADDR + 0x2C000)
#define MMDC_P0_BASE_ADDR           (AIPS2_OFF_BASE_ADDR + 0x30000)
#define MMDC_P1_BASE_ADDR           (AIPS2_OFF_BASE_ADDR + 0x34000)
#define WEIM_BASE_ADDR              (AIPS2_OFF_BASE_ADDR + 0x38000)
#define OCOTP_BASE_ADDR             (AIPS2_OFF_BASE_ADDR + 0x3C000)
#define CSU_BASE_ADDR               (AIPS2_OFF_BASE_ADDR + 0x40000)
#define IP2APB_PERFMON1_BASE_ADDR   (AIPS2_OFF_BASE_ADDR + 0x44000)
#define IP2APB_PERFMON2_BASE_ADDR   (AIPS2_OFF_BASE_ADDR + 0x48000)
#define IP2APB_PERFMON3_BASE_ADDR   (AIPS2_OFF_BASE_ADDR + 0x4C000)
#define IP2APB_TZASC1_BASE_ADDR     (AIPS2_OFF_BASE_ADDR + 0x50000)
#define IP2APB_TZASC2_BASE_ADDR     (AIPS2_OFF_BASE_ADDR + 0x54000)
#define AUDMUX_BASE_ADDR            (AIPS2_OFF_BASE_ADDR + 0x58000)
#define MIPI_CSI2_BASE_ADDR         (AIPS2_OFF_BASE_ADDR + 0x5C000)
#define MIPI_DSI_BASE_ADDR          (AIPS2_OFF_BASE_ADDR + 0x60000)
#define VDOA_BASE_ADDR              (AIPS2_OFF_BASE_ADDR + 0x64000)
#define UART2_BASE                  (AIPS2_OFF_BASE_ADDR + 0x68000)
#define UART3_BASE                  (AIPS2_OFF_BASE_ADDR + 0x6C000)
#define UART4_BASE                  (AIPS2_OFF_BASE_ADDR + 0x70000)
#define UART5_BASE                  (AIPS2_OFF_BASE_ADDR + 0x74000)
#define IP2APB_USBPHY1_BASE_ADDR    (AIPS2_OFF_BASE_ADDR + 0x78000)
#define IP2APB_USBPHY2_BASE_ADDR    (AIPS2_OFF_BASE_ADDR + 0x7C000)

#define CHIP_REV_1_0                 0x10
#define IRAM_SIZE                    0x00040000
#define IMX_IIM_BASE                 OCOTP_BASE_ADDR
#define FEC_QUIRK_ENET_MAC

#if !(defined(__KERNEL_STRICT_NAMES) || defined(__ASSEMBLY__))


/* System Reset Controller (SRC) */
struct src {
	uint32_t	scr;
	uint32_t	sbmr1;
	uint32_t	srsr;
	uint32_t	reserved1[2];
	uint32_t	sisr;
	uint32_t	simr;
	uint32_t     sbmr2;
	uint32_t     gpr1;
	uint32_t     gpr2;
	uint32_t     gpr3;
	uint32_t     gpr4;
	uint32_t     gpr5;
	uint32_t     gpr6;
	uint32_t     gpr7;
	uint32_t     gpr8;
	uint32_t     gpr9;
	uint32_t     gpr10;
};

/* OCOTP Registers */
struct ocotp_regs {
	uint32_t	reserved[0x198];
	uint32_t	gp1;	/* 0x660 */
};

/* GPR3 bitfields */
#define IOMUXC_GPR3_GPU_DBG_OFFSET		29
#define IOMUXC_GPR3_GPU_DBG_MASK		(3<<IOMUXC_GPR3_GPU_DBG_OFFSET)
#define IOMUXC_GPR3_BCH_WR_CACHE_CTL_OFFSET	28
#define IOMUXC_GPR3_BCH_WR_CACHE_CTL_MASK	(1<<IOMUXC_GPR3_BCH_WR_CACHE_CTL_OFFSET)
#define IOMUXC_GPR3_BCH_RD_CACHE_CTL_OFFSET	27
#define IOMUXC_GPR3_BCH_RD_CACHE_CTL_MASK	(1<<IOMUXC_GPR3_BCH_RD_CACHE_CTL_OFFSET)
#define IOMUXC_GPR3_uSDHCx_WR_CACHE_CTL_OFFSET	26
#define IOMUXC_GPR3_uSDHCx_WR_CACHE_CTL_MASK	(1<<IOMUXC_GPR3_uSDHCx_WR_CACHE_CTL_OFFSET)
#define IOMUXC_GPR3_uSDHCx_RD_CACHE_CTL_OFFSET	25
#define IOMUXC_GPR3_uSDHCx_RD_CACHE_CTL_MASK	(1<<IOMUXC_GPR3_uSDHCx_RD_CACHE_CTL_OFFSET)
#define IOMUXC_GPR3_OCRAM_CTL_OFFSET		21
#define IOMUXC_GPR3_OCRAM_CTL_MASK		(0xf<<IOMUXC_GPR3_OCRAM_CTL_OFFSET)
#define IOMUXC_GPR3_OCRAM_STATUS_OFFSET		17
#define IOMUXC_GPR3_OCRAM_STATUS_MASK		(0xf<<IOMUXC_GPR3_OCRAM_STATUS_OFFSET)
#define IOMUXC_GPR3_CORE3_DBG_ACK_EN_OFFSET	16
#define IOMUXC_GPR3_CORE3_DBG_ACK_EN_MASK	(1<<IOMUXC_GPR3_CORE3_DBG_ACK_EN_OFFSET)
#define IOMUXC_GPR3_CORE2_DBG_ACK_EN_OFFSET	15
#define IOMUXC_GPR3_CORE2_DBG_ACK_EN_MASK	(1<<IOMUXC_GPR3_CORE2_DBG_ACK_EN_OFFSET)
#define IOMUXC_GPR3_CORE1_DBG_ACK_EN_OFFSET	14
#define IOMUXC_GPR3_CORE1_DBG_ACK_EN_MASK	(1<<IOMUXC_GPR3_CORE1_DBG_ACK_EN_OFFSET)
#define IOMUXC_GPR3_CORE0_DBG_ACK_EN_OFFSET	13
#define IOMUXC_GPR3_CORE0_DBG_ACK_EN_MASK	(1<<IOMUXC_GPR3_CORE0_DBG_ACK_EN_OFFSET)
#define IOMUXC_GPR3_TZASC2_BOOT_LOCK_OFFSET	12
#define IOMUXC_GPR3_TZASC2_BOOT_LOCK_MASK	(1<<IOMUXC_GPR3_TZASC2_BOOT_LOCK_OFFSET)
#define IOMUXC_GPR3_TZASC1_BOOT_LOCK_OFFSET	11
#define IOMUXC_GPR3_TZASC1_BOOT_LOCK_MASK	(1<<IOMUXC_GPR3_TZASC1_BOOT_LOCK_OFFSET)
#define IOMUXC_GPR3_IPU_DIAG_OFFSET		10
#define IOMUXC_GPR3_IPU_DIAG_MASK		(1<<IOMUXC_GPR3_IPU_DIAG_OFFSET)

#define IOMUXC_GPR3_MUX_SRC_IPU1_DI0	0
#define IOMUXC_GPR3_MUX_SRC_IPU1_DI1	1
#define IOMUXC_GPR3_MUX_SRC_IPU2_DI0	2
#define IOMUXC_GPR3_MUX_SRC_IPU2_DI1	3

#define IOMUXC_GPR3_LVDS1_MUX_CTL_OFFSET	8
#define IOMUXC_GPR3_LVDS1_MUX_CTL_MASK		(3<<IOMUXC_GPR3_LVDS1_MUX_CTL_OFFSET)

#define IOMUXC_GPR3_LVDS0_MUX_CTL_OFFSET	6
#define IOMUXC_GPR3_LVDS0_MUX_CTL_MASK		(3<<IOMUXC_GPR3_LVDS0_MUX_CTL_OFFSET)

#define IOMUXC_GPR3_MIPI_MUX_CTL_OFFSET		4
#define IOMUXC_GPR3_MIPI_MUX_CTL_MASK		(3<<IOMUXC_GPR3_MIPI_MUX_CTL_OFFSET)

#define IOMUXC_GPR3_HDMI_MUX_CTL_OFFSET		2
#define IOMUXC_GPR3_HDMI_MUX_CTL_MASK		(3<<IOMUXC_GPR3_HDMI_MUX_CTL_OFFSET)


struct iomuxc {
	uint32_t gpr[14];
	uint32_t omux[5];
	/* mux and pad registers */
};

#define IOMUXC_GPR2_COUNTER_RESET_VAL_OFFSET		20
#define IOMUXC_GPR2_COUNTER_RESET_VAL_MASK		(3<<IOMUXC_GPR2_COUNTER_RESET_VAL_OFFSET)
#define IOMUXC_GPR2_LVDS_CLK_SHIFT_OFFSET		16
#define IOMUXC_GPR2_LVDS_CLK_SHIFT_MASK			(7<<IOMUXC_GPR2_LVDS_CLK_SHIFT_OFFSET)

#define IOMUXC_GPR2_BGREF_RRMODE_OFFSET			15
#define IOMUXC_GPR2_BGREF_RRMODE_MASK			(1<<IOMUXC_GPR2_BGREF_RRMODE_OFFSET)
#define IOMUXC_GPR2_BGREF_RRMODE_INTERNAL_RES		(1<<IOMUXC_GPR2_BGREF_RRMODE_OFFSET)
#define IOMUXC_GPR2_BGREF_RRMODE_EXTERNAL_RES		(0<<IOMUXC_GPR2_BGREF_RRMODE_OFFSET)
#define IOMUXC_GPR2_VSYNC_ACTIVE_HIGH	0
#define IOMUXC_GPR2_VSYNC_ACTIVE_LOW	1

#define IOMUXC_GPR2_DI1_VS_POLARITY_OFFSET		10
#define IOMUXC_GPR2_DI1_VS_POLARITY_MASK		(1<<IOMUXC_GPR2_DI1_VS_POLARITY_OFFSET)
#define IOMUXC_GPR2_DI1_VS_POLARITY_ACTIVE_HIGH		(IOMUXC_GPR2_VSYNC_ACTIVE_HIGH<<IOMUXC_GPR2_DI1_VS_POLARITY_OFFSET)
#define IOMUXC_GPR2_DI1_VS_POLARITY_ACTIVE_LOW		(IOMUXC_GPR2_VSYNC_ACTIVE_LOW<<IOMUXC_GPR2_DI1_VS_POLARITY_OFFSET)

#define IOMUXC_GPR2_DI0_VS_POLARITY_OFFSET		9
#define IOMUXC_GPR2_DI0_VS_POLARITY_MASK		(1<<IOMUXC_GPR2_DI0_VS_POLARITY_OFFSET)
#define IOMUXC_GPR2_DI0_VS_POLARITY_ACTIVE_HIGH		(IOMUXC_GPR2_VSYNC_ACTIVE_HIGH<<IOMUXC_GPR2_DI0_VS_POLARITY_OFFSET)
#define IOMUXC_GPR2_DI0_VS_POLARITY_ACTIVE_LOW		(IOMUXC_GPR2_VSYNC_ACTIVE_LOW<<IOMUXC_GPR2_DI0_VS_POLARITY_OFFSET)

#define IOMUXC_GPR2_BITMAP_SPWG	0
#define IOMUXC_GPR2_BITMAP_JEIDA	1

#define IOMUXC_GPR2_BIT_MAPPING_CH1_OFFSET		8
#define IOMUXC_GPR2_BIT_MAPPING_CH1_MASK		(1<<IOMUXC_GPR2_BIT_MAPPING_CH1_OFFSET)
#define IOMUXC_GPR2_BIT_MAPPING_CH1_JEIDA		(IOMUXC_GPR2_BITMAP_JEIDA<<IOMUXC_GPR2_BIT_MAPPING_CH1_OFFSET)
#define IOMUXC_GPR2_BIT_MAPPING_CH1_SPWG		(IOMUXC_GPR2_BITMAP_SPWG<<IOMUXC_GPR2_BIT_MAPPING_CH1_OFFSET)

#define IOMUXC_GPR2_DATA_WIDTH_18	0
#define IOMUXC_GPR2_DATA_WIDTH_24	1

#define IOMUXC_GPR2_DATA_WIDTH_CH1_OFFSET		7
#define IOMUXC_GPR2_DATA_WIDTH_CH1_MASK			(1<<IOMUXC_GPR2_DATA_WIDTH_CH1_OFFSET)
#define IOMUXC_GPR2_DATA_WIDTH_CH1_18BIT		(IOMUXC_GPR2_DATA_WIDTH_18<<IOMUXC_GPR2_DATA_WIDTH_CH1_OFFSET)
#define IOMUXC_GPR2_DATA_WIDTH_CH1_24BIT		(IOMUXC_GPR2_DATA_WIDTH_24<<IOMUXC_GPR2_DATA_WIDTH_CH1_OFFSET)

#define IOMUXC_GPR2_BIT_MAPPING_CH0_OFFSET		6
#define IOMUXC_GPR2_BIT_MAPPING_CH0_MASK		(1<<IOMUXC_GPR2_BIT_MAPPING_CH0_OFFSET)
#define IOMUXC_GPR2_BIT_MAPPING_CH0_JEIDA		(IOMUXC_GPR2_BITMAP_JEIDA<<IOMUXC_GPR2_BIT_MAPPING_CH0_OFFSET)
#define IOMUXC_GPR2_BIT_MAPPING_CH0_SPWG		(IOMUXC_GPR2_BITMAP_SPWG<<IOMUXC_GPR2_BIT_MAPPING_CH0_OFFSET)

#define IOMUXC_GPR2_DATA_WIDTH_CH0_OFFSET		5
#define IOMUXC_GPR2_DATA_WIDTH_CH0_MASK			(1<<IOMUXC_GPR2_DATA_WIDTH_CH0_OFFSET)
#define IOMUXC_GPR2_DATA_WIDTH_CH0_18BIT		(IOMUXC_GPR2_DATA_WIDTH_18<<IOMUXC_GPR2_DATA_WIDTH_CH0_OFFSET)
#define IOMUXC_GPR2_DATA_WIDTH_CH0_24BIT		(IOMUXC_GPR2_DATA_WIDTH_24<<IOMUXC_GPR2_DATA_WIDTH_CH0_OFFSET)

#define IOMUXC_GPR2_SPLIT_MODE_EN_OFFSET		4
#define IOMUXC_GPR2_SPLIT_MODE_EN_MASK			(1<<IOMUXC_GPR2_SPLIT_MODE_EN_OFFSET)

#define IOMUXC_GPR2_MODE_DISABLED	0
#define IOMUXC_GPR2_MODE_ENABLED_DI0	1
#define IOMUXC_GPR2_MODE_ENABLED_DI1	2

#define IOMUXC_GPR2_LVDS_CH1_MODE_OFFSET		2
#define IOMUXC_GPR2_LVDS_CH1_MODE_MASK			(3<<IOMUXC_GPR2_LVDS_CH1_MODE_OFFSET)
#define IOMUXC_GPR2_LVDS_CH1_MODE_DISABLED		(IOMUXC_GPR2_MODE_DISABLED<<IOMUXC_GPR2_LVDS_CH1_MODE_OFFSET)
#define IOMUXC_GPR2_LVDS_CH1_MODE_ENABLED_DI0		(IOMUXC_GPR2_MODE_ENABLED_DI0<<IOMUXC_GPR2_LVDS_CH1_MODE_OFFSET)
#define IOMUXC_GPR2_LVDS_CH1_MODE_ENABLED_DI1		(IOMUXC_GPR2_MODE_ENABLED_DI1<<IOMUXC_GPR2_LVDS_CH1_MODE_OFFSET)

#define IOMUXC_GPR2_LVDS_CH0_MODE_OFFSET		0
#define IOMUXC_GPR2_LVDS_CH0_MODE_MASK			(3<<IOMUXC_GPR2_LVDS_CH0_MODE_OFFSET)
#define IOMUXC_GPR2_LVDS_CH0_MODE_DISABLED		(IOMUXC_GPR2_MODE_DISABLED<<IOMUXC_GPR2_LVDS_CH0_MODE_OFFSET)
#define IOMUXC_GPR2_LVDS_CH0_MODE_ENABLED_DI0		(IOMUXC_GPR2_MODE_ENABLED_DI0<<IOMUXC_GPR2_LVDS_CH0_MODE_OFFSET)
#define IOMUXC_GPR2_LVDS_CH0_MODE_ENABLED_DI1		(IOMUXC_GPR2_MODE_ENABLED_DI1<<IOMUXC_GPR2_LVDS_CH0_MODE_OFFSET)

/* ECSPI registers */
struct cspi_regs {
	uint32_t rxdata;
	uint32_t txdata;
	uint32_t ctrl;
	uint32_t cfg;
	uint32_t intr;
	uint32_t dma;
	uint32_t stat;
	uint32_t period;
};

/*
 * CSPI register definitions
 */
#define MXC_ECSPI
#define MXC_CSPICTRL_EN		(1 << 0)
#define MXC_CSPICTRL_MODE	(1 << 1)
#define MXC_CSPICTRL_XCH	(1 << 2)
#define MXC_CSPICTRL_CHIPSELECT(x)	(((x) & 0x3) << 12)
#define MXC_CSPICTRL_BITCOUNT(x)	(((x) & 0xfff) << 20)
#define MXC_CSPICTRL_PREDIV(x)	(((x) & 0xF) << 12)
#define MXC_CSPICTRL_POSTDIV(x)	(((x) & 0xF) << 8)
#define MXC_CSPICTRL_SELCHAN(x)	(((x) & 0x3) << 18)
#define MXC_CSPICTRL_MAXBITS	0xfff
#define MXC_CSPICTRL_TC		(1 << 7)
#define MXC_CSPICTRL_RXOVF	(1 << 6)
#define MXC_CSPIPERIOD_32KHZ	(1 << 15)
#define MAX_SPI_BYTES	32

/* Bit position inside CTRL register to be associated with SS */
#define MXC_CSPICTRL_CHAN	18

/* Bit position inside CON register to be associated with SS */
#define MXC_CSPICON_POL		4
#define MXC_CSPICON_PHA		0
#define MXC_CSPICON_SSPOL	12
#define MXC_SPI_BASE_ADDRESSES \
	ECSPI1_BASE_ADDR, \
	ECSPI2_BASE_ADDR, \
	ECSPI3_BASE_ADDR, \
	ECSPI4_BASE_ADDR, \
	ECSPI5_BASE_ADDR

struct iim_regs {
	uint32_t	ctrl;
	uint32_t	ctrl_set;
	uint32_t     ctrl_clr;
	uint32_t	ctrl_tog;
	uint32_t	timing;
	uint32_t     rsvd0[3];
	uint32_t     data;
	uint32_t     rsvd1[3];
	uint32_t     read_ctrl;
	uint32_t     rsvd2[3];
	uint32_t     fuse_data;
	uint32_t     rsvd3[3];
	uint32_t     sticky;
	uint32_t     rsvd4[3];
	uint32_t     scs;
	uint32_t     scs_set;
	uint32_t     scs_clr;
	uint32_t     scs_tog;
	uint32_t     crc_addr;
	uint32_t     rsvd5[3];
	uint32_t     crc_value;
	uint32_t     rsvd6[3];
	uint32_t     version;
	uint32_t     rsvd7[0xdb];

	struct fuse_bank {
		uint32_t	fuse_regs[0x20];
	} bank[15];
};

struct fuse_bank4_regs {
	uint32_t	sjc_resp_low;
	uint32_t     rsvd0[3];
	uint32_t     sjc_resp_high;
	uint32_t     rsvd1[3];
	uint32_t	mac_addr_low;
	uint32_t     rsvd2[3];
	uint32_t     mac_addr_high;
	uint32_t	rsvd3[0x13];
};

struct aipstz_regs {
	uint32_t	mprot0;
	uint32_t	mprot1;
	uint32_t	rsvd[0xe];
	uint32_t	opacr0;
	uint32_t	opacr1;
	uint32_t	opacr2;
	uint32_t	opacr3;
	uint32_t	opacr4;
};

struct anatop_regs {
	uint32_t	pll_sys;		/* 0x000 */
	uint32_t	pll_sys_set;		/* 0x004 */
	uint32_t	pll_sys_clr;		/* 0x008 */
	uint32_t	pll_sys_tog;		/* 0x00c */
	uint32_t	usb1_pll_480_ctrl;	/* 0x010 */
	uint32_t	usb1_pll_480_ctrl_set;	/* 0x014 */
	uint32_t	usb1_pll_480_ctrl_clr;	/* 0x018 */
	uint32_t	usb1_pll_480_ctrl_tog;	/* 0x01c */
	uint32_t	usb2_pll_480_ctrl;	/* 0x020 */
	uint32_t	usb2_pll_480_ctrl_set;	/* 0x024 */
	uint32_t	usb2_pll_480_ctrl_clr;	/* 0x028 */
	uint32_t	usb2_pll_480_ctrl_tog;	/* 0x02c */
	uint32_t	pll_528;		/* 0x030 */
	uint32_t	pll_528_set;		/* 0x034 */
	uint32_t	pll_528_clr;		/* 0x038 */
	uint32_t	pll_528_tog;		/* 0x03c */
	uint32_t	pll_528_ss;		/* 0x040 */
	uint32_t	rsvd0[3];
	uint32_t	pll_528_num;		/* 0x050 */
	uint32_t	rsvd1[3];
	uint32_t	pll_528_denom;		/* 0x060 */
	uint32_t	rsvd2[3];
	uint32_t	pll_audio;		/* 0x070 */
	uint32_t	pll_audio_set;		/* 0x074 */
	uint32_t	pll_audio_clr;		/* 0x078 */
	uint32_t	pll_audio_tog;		/* 0x07c */
	uint32_t	pll_audio_num;		/* 0x080 */
	uint32_t	rsvd3[3];
	uint32_t	pll_audio_denom;	/* 0x090 */
	uint32_t	rsvd4[3];
	uint32_t	pll_video;		/* 0x0a0 */
	uint32_t	pll_video_set;		/* 0x0a4 */
	uint32_t	pll_video_clr;		/* 0x0a8 */
	uint32_t	pll_video_tog;		/* 0x0ac */
	uint32_t	pll_video_num;		/* 0x0b0 */
	uint32_t	rsvd5[3];
	uint32_t	pll_video_denom;	/* 0x0c0 */
	uint32_t	rsvd6[3];
	uint32_t	pll_mlb;		/* 0x0d0 */
	uint32_t	pll_mlb_set;		/* 0x0d4 */
	uint32_t	pll_mlb_clr;		/* 0x0d8 */
	uint32_t	pll_mlb_tog;		/* 0x0dc */
	uint32_t	pll_enet;		/* 0x0e0 */
	uint32_t	pll_enet_set;		/* 0x0e4 */
	uint32_t	pll_enet_clr;		/* 0x0e8 */
	uint32_t	pll_enet_tog;		/* 0x0ec */
	uint32_t	pfd_480;		/* 0x0f0 */
	uint32_t	pfd_480_set;		/* 0x0f4 */
	uint32_t	pfd_480_clr;		/* 0x0f8 */
	uint32_t	pfd_480_tog;		/* 0x0fc */
	uint32_t	pfd_528;		/* 0x100 */
	uint32_t	pfd_528_set;		/* 0x104 */
	uint32_t	pfd_528_clr;		/* 0x108 */
	uint32_t	pfd_528_tog;		/* 0x10c */
	uint32_t	reg_1p1;		/* 0x110 */
	uint32_t	reg_1p1_set;		/* 0x114 */
	uint32_t	reg_1p1_clr;		/* 0x118 */
	uint32_t	reg_1p1_tog;		/* 0x11c */
	uint32_t	reg_3p0;		/* 0x120 */
	uint32_t	reg_3p0_set;		/* 0x124 */
	uint32_t	reg_3p0_clr;		/* 0x128 */
	uint32_t	reg_3p0_tog;		/* 0x12c */
	uint32_t	reg_2p5;		/* 0x130 */
	uint32_t	reg_2p5_set;		/* 0x134 */
	uint32_t	reg_2p5_clr;		/* 0x138 */
	uint32_t	reg_2p5_tog;		/* 0x13c */
	uint32_t	reg_core;		/* 0x140 */
	uint32_t	reg_core_set;		/* 0x144 */
	uint32_t	reg_core_clr;		/* 0x148 */
	uint32_t	reg_core_tog;		/* 0x14c */
	uint32_t	ana_misc0;		/* 0x150 */
	uint32_t	ana_misc0_set;		/* 0x154 */
	uint32_t	ana_misc0_clr;		/* 0x158 */
	uint32_t	ana_misc0_tog;		/* 0x15c */
	uint32_t	ana_misc1;		/* 0x160 */
	uint32_t	ana_misc1_set;		/* 0x164 */
	uint32_t	ana_misc1_clr;		/* 0x168 */
	uint32_t	ana_misc1_tog;		/* 0x16c */
	uint32_t	ana_misc2;		/* 0x170 */
	uint32_t	ana_misc2_set;		/* 0x174 */
	uint32_t	ana_misc2_clr;		/* 0x178 */
	uint32_t	ana_misc2_tog;		/* 0x17c */
	uint32_t	tempsense0;		/* 0x180 */
	uint32_t	tempsense0_set;		/* 0x184 */
	uint32_t	tempsense0_clr;		/* 0x188 */
	uint32_t	tempsense0_tog;		/* 0x18c */
	uint32_t	tempsense1;		/* 0x190 */
	uint32_t	tempsense1_set;		/* 0x194 */
	uint32_t	tempsense1_clr;		/* 0x198 */
	uint32_t	tempsense1_tog;		/* 0x19c */
	uint32_t	usb1_vbus_detect;	/* 0x1a0 */
	uint32_t	usb1_vbus_detect_set;	/* 0x1a4 */
	uint32_t	usb1_vbus_detect_clr;	/* 0x1a8 */
	uint32_t	usb1_vbus_detect_tog;	/* 0x1ac */
	uint32_t	usb1_chrg_detect;	/* 0x1b0 */
	uint32_t	usb1_chrg_detect_set;	/* 0x1b4 */
	uint32_t	usb1_chrg_detect_clr;	/* 0x1b8 */
	uint32_t	usb1_chrg_detect_tog;	/* 0x1bc */
	uint32_t	usb1_vbus_det_stat;	/* 0x1c0 */
	uint32_t	usb1_vbus_det_stat_set;	/* 0x1c4 */
	uint32_t	usb1_vbus_det_stat_clr;	/* 0x1c8 */
	uint32_t	usb1_vbus_det_stat_tog;	/* 0x1cc */
	uint32_t	usb1_chrg_det_stat;	/* 0x1d0 */
	uint32_t	usb1_chrg_det_stat_set;	/* 0x1d4 */
	uint32_t	usb1_chrg_det_stat_clr;	/* 0x1d8 */
	uint32_t	usb1_chrg_det_stat_tog;	/* 0x1dc */
	uint32_t	usb1_loopback;		/* 0x1e0 */
	uint32_t	usb1_loopback_set;	/* 0x1e4 */
	uint32_t	usb1_loopback_clr;	/* 0x1e8 */
	uint32_t	usb1_loopback_tog;	/* 0x1ec */
	uint32_t	usb1_misc;		/* 0x1f0 */
	uint32_t	usb1_misc_set;		/* 0x1f4 */
	uint32_t	usb1_misc_clr;		/* 0x1f8 */
	uint32_t	usb1_misc_tog;		/* 0x1fc */
	uint32_t	usb2_vbus_detect;	/* 0x200 */
	uint32_t	usb2_vbus_detect_set;	/* 0x204 */
	uint32_t	usb2_vbus_detect_clr;	/* 0x208 */
	uint32_t	usb2_vbus_detect_tog;	/* 0x20c */
	uint32_t	usb2_chrg_detect;	/* 0x210 */
	uint32_t	usb2_chrg_detect_set;	/* 0x214 */
	uint32_t	usb2_chrg_detect_clr;	/* 0x218 */
	uint32_t	usb2_chrg_detect_tog;	/* 0x21c */
	uint32_t	usb2_vbus_det_stat;	/* 0x220 */
	uint32_t	usb2_vbus_det_stat_set;	/* 0x224 */
	uint32_t	usb2_vbus_det_stat_clr;	/* 0x228 */
	uint32_t	usb2_vbus_det_stat_tog;	/* 0x22c */
	uint32_t	usb2_chrg_det_stat;	/* 0x230 */
	uint32_t	usb2_chrg_det_stat_set;	/* 0x234 */
	uint32_t	usb2_chrg_det_stat_clr;	/* 0x238 */
	uint32_t	usb2_chrg_det_stat_tog;	/* 0x23c */
	uint32_t	usb2_loopback;		/* 0x240 */
	uint32_t	usb2_loopback_set;	/* 0x244 */
	uint32_t	usb2_loopback_clr;	/* 0x248 */
	uint32_t	usb2_loopback_tog;	/* 0x24c */
	uint32_t	usb2_misc;		/* 0x250 */
	uint32_t	usb2_misc_set;		/* 0x254 */
	uint32_t	usb2_misc_clr;		/* 0x258 */
	uint32_t	usb2_misc_tog;		/* 0x25c */
	uint32_t	digprog;		/* 0x260 */
	uint32_t	reserved1[7];
	uint32_t	digprog_sololite;	/* 0x280 */
};

#define ANATOP_PFD_480_PFD0_FRAC_SHIFT		0
#define ANATOP_PFD_480_PFD0_FRAC_MASK		(0x3f<<ANATOP_PFD_480_PFD0_FRAC_SHIFT)
#define ANATOP_PFD_480_PFD0_STABLE_SHIFT	6
#define ANATOP_PFD_480_PFD0_STABLE_MASK		(1<<ANATOP_PFD_480_PFD0_STABLE_SHIFT)
#define ANATOP_PFD_480_PFD0_CLKGATE_SHIFT	7
#define ANATOP_PFD_480_PFD0_CLKGATE_MASK	(1<<ANATOP_PFD_480_PFD0_CLKGATE_SHIFT)
#define ANATOP_PFD_480_PFD1_FRAC_SHIFT		8
#define ANATOP_PFD_480_PFD1_FRAC_MASK		(0x3f<<ANATOP_PFD_480_PFD1_FRAC_SHIFT)
#define ANATOP_PFD_480_PFD1_STABLE_SHIFT	14
#define ANATOP_PFD_480_PFD1_STABLE_MASK		(1<<ANATOP_PFD_480_PFD1_STABLE_SHIFT)
#define ANATOP_PFD_480_PFD1_CLKGATE_SHIFT	15
#define ANATOP_PFD_480_PFD1_CLKGATE_MASK	(0x3f<<ANATOP_PFD_480_PFD1_CLKGATE_SHIFT)
#define ANATOP_PFD_480_PFD2_FRAC_SHIFT		16
#define ANATOP_PFD_480_PFD2_FRAC_MASK		(1<<ANATOP_PFD_480_PFD2_FRAC_SHIFT)
#define ANATOP_PFD_480_PFD2_STABLE_SHIFT	22
#define ANATOP_PFD_480_PFD2_STABLE_MASK	(1<<ANATOP_PFD_480_PFD2_STABLE_SHIFT)
#define ANATOP_PFD_480_PFD2_CLKGATE_SHIFT	23
#define ANATOP_PFD_480_PFD2_CLKGATE_MASK	(0x3f<<ANATOP_PFD_480_PFD2_CLKGATE_SHIFT)
#define ANATOP_PFD_480_PFD3_FRAC_SHIFT		24
#define ANATOP_PFD_480_PFD3_FRAC_MASK		(1<<ANATOP_PFD_480_PFD3_FRAC_SHIFT)
#define ANATOP_PFD_480_PFD3_STABLE_SHIFT	30
#define ANATOP_PFD_480_PFD3_STABLE_MASK		(1<<ANATOP_PFD_480_PFD3_STABLE_SHIFT)
#define ANATOP_PFD_480_PFD3_CLKGATE_SHIFT	31

struct iomuxc_base_regs {
	uint32_t     gpr[14];        /* 0x000 */
	uint32_t     obsrv[5];       /* 0x038 */
	uint32_t     swmux_ctl[197]; /* 0x04c */
	uint32_t     swpad_ctl[250]; /* 0x360 */
	uint32_t     swgrp[26];      /* 0x748 */
	uint32_t     daisy[104];     /* 0x7b0..94c */
};

#define BP_OCOTP_CTRL_WR_UNLOCK		16
#define BM_OCOTP_CTRL_WR_UNLOCK		0xFFFF0000
#define BV_OCOTP_CTRL_WR_UNLOCK__KEY	0x3E77
#define BM_OCOTP_CTRL_RELOAD_SHADOWS	0x00000400
#define BM_OCOTP_CTRL_ERROR		0x00000200
#define BM_OCOTP_CTRL_BUSY		0x00000100
#define BP_OCOTP_CTRL_ADDR		0
#define BM_OCOTP_CTRL_ADDR		0x0000007F

#define BP_OCOTP_TIMING_STROBE_READ	16
#define BM_OCOTP_TIMING_STROBE_READ	0x003F0000
#define BP_OCOTP_TIMING_RELAX		12
#define BM_OCOTP_TIMING_RELAX		0x0000F000
#define BP_OCOTP_TIMING_STROBE_PROG     0
#define BM_OCOTP_TIMING_STROBE_PROG	0x00000FFF

#define BM_OCOTP_READ_CTRL_READ_FUSE	0x00000001

#endif /* __ASSEMBLER__*/
#endif /* __ASM_ARCH_MX6_IMX_REGS_H__ */
