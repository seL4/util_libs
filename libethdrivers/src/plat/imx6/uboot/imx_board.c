/*
 * based on u-boot-imx6/board/freescale/mx6qsabrelite/mx6qsabrelite.c
 *
 * Copyright (C) 2010-2011 Freescale Semiconductor, Inc.
 * Copyright (C) 2018 NXP
 * Copyright (C) 2020, HENSOLDT Cyber GmbH
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
#include "gpio.h"
#include "../io.h"
#include "imx_board.h"
#include <platsupport/io.h>
#include <stdio.h>


#if defined(CONFIG_PLAT_IMX6)

#if defined(CONFIG_PLAT_IMX6DQ)
#include "imx6dq_pins.h"
#elif defined(CONFIG_PLAT_IMX6SX)
#include "imx6sx_pins.h"
#else
#error "unknown i.MX6 SOC"
#endif

#define IOMUXC_PADDR    0x020E0000
#define IOMUXC_SIZE     0x4000

#if defined(CONFIG_PLAT_IMX6SX)
#define IOMUXC_GPR_PADDR    0x020E4000
#define IOMUXC_GPR_SIZE     0x4000
#endif

#elif defined(CONFIG_PLAT_IMX8MQ_EVK)

#include "imx8mq_pins.h"

#define IOMUXC_PADDR    0x30330000
#define IOMUXC_SIZE     0x10000

#elif defined(CONFIG_PLAT_IMX8MM_EVK)
#include "imx8mm_pins.h"

#define IOMUXC_PADDR    0x30330000 // from: root/arch/arm/include/asm/arch-imx8m/imx-regs-imx8mm.h
#define IOMUXC_SIZE     0x10000

#else
#error "unknown i.MX SOC"
#endif


/*
 * configure pads in the IOMUX
 */
static void imx_iomux_v3_setup_multiple_pads(
    void *base,
    iomux_v3_cfg_t const *pad_list,
    unsigned int count)
{
    for (unsigned int i = 0; i < count; i++) {
        iomux_v3_cfg_t pad = pad_list[i];

        // set mode
        uint32_t mux_ctrl_ofs = (pad & MUX_CTRL_OFS_MASK) >> MUX_CTRL_OFS_SHIFT;
        if (mux_ctrl_ofs) {
            uint32_t mux_mode = (pad & MUX_MODE_MASK) >> MUX_MODE_SHIFT;
            __raw_writel(mux_mode, base + mux_ctrl_ofs);
        }

        // set input selector
        uint32_t sel_input_ofs = (pad & MUX_SEL_INPUT_OFS_MASK) >> MUX_SEL_INPUT_OFS_SHIFT;
        if (sel_input_ofs) {
            uint32_t sel_input = (pad & MUX_SEL_INPUT_MASK) >> MUX_SEL_INPUT_SHIFT;
            __raw_writel(sel_input, base + sel_input_ofs);
        }

        // set pad control
        uint32_t pad_ctrl_ofs = (pad & MUX_PAD_CTRL_OFS_MASK) >> MUX_PAD_CTRL_OFS_SHIFT;
        if (pad_ctrl_ofs) {
            uint32_t pad_ctrl = (pad & MUX_PAD_CTRL_MASK) >> MUX_PAD_CTRL_SHIFT;
            if (!(pad_ctrl & NO_PAD_CTRL)) {
                __raw_writel(pad_ctrl, base + pad_ctrl_ofs);
            }
        }
    }
}

#define IMX_IOMUX_V3_SETUP_MULTIPLE_PADS(_base_, ...) \
    do { \
        iomux_v3_cfg_t const pad_cfg[] = { __VA_ARGS__ }; \
        imx_iomux_v3_setup_multiple_pads( \
            _base_, \
            pad_cfg, \
            ARRAY_SIZE(pad_cfg) ); \
    } while(0)



int setup_iomux_enet(ps_io_ops_t *io_ops)
{
    int ret;
    void *base;
    int unmapOnExit = 0;

    if (mux_sys_valid(&io_ops->mux_sys)) {
        base = mux_sys_get_vaddr(&io_ops->mux_sys);
    } else {
        base = RESOURCE(&io_ops->io_mapper, IOMUXC);
        unmapOnExit = 1;
    }
    if (!base) {
        ZF_LOGE("base is NULL");
        return 1;
    }

#if defined(CONFIG_PLAT_IMX8MQ_EVK)
    /* see the IMX8 reference manual for what the options mean,
     * Section 8.2.4 i.e. IOMUXC_SW_PAD_CTL_PAD_* registers */
    IMX_IOMUX_V3_SETUP_MULTIPLE_PADS(
        base,
        IMX8MQ_PAD_ENET_MDC__ENET_MDC | MUX_PAD_CTRL(0x3),
        IMX8MQ_PAD_ENET_MDIO__ENET_MDIO | MUX_PAD_CTRL(0x23),
        IMX8MQ_PAD_ENET_TD3__ENET_RGMII_TD3 | MUX_PAD_CTRL(0x1f),
        IMX8MQ_PAD_ENET_TD2__ENET_RGMII_TD2 | MUX_PAD_CTRL(0x1f),
        IMX8MQ_PAD_ENET_TD1__ENET_RGMII_TD1 | MUX_PAD_CTRL(0x1f),
        IMX8MQ_PAD_ENET_TD0__ENET_RGMII_TD0 | MUX_PAD_CTRL(0x1f),
        IMX8MQ_PAD_ENET_RD3__ENET_RGMII_RD3 | MUX_PAD_CTRL(0x91),
        IMX8MQ_PAD_ENET_RD2__ENET_RGMII_RD2 | MUX_PAD_CTRL(0x91),
        IMX8MQ_PAD_ENET_RD1__ENET_RGMII_RD1 | MUX_PAD_CTRL(0x91),
        IMX8MQ_PAD_ENET_RD0__ENET_RGMII_RD0 | MUX_PAD_CTRL(0x91),
        IMX8MQ_PAD_ENET_TXC__ENET_RGMII_TXC | MUX_PAD_CTRL(0x1f),
        IMX8MQ_PAD_ENET_RXC__ENET_RGMII_RXC | MUX_PAD_CTRL(0x91),
        IMX8MQ_PAD_ENET_TX_CTL__ENET_RGMII_TX_CTL | MUX_PAD_CTRL(0x1f),
        IMX8MQ_PAD_ENET_RX_CTL__ENET_RGMII_RX_CTL | MUX_PAD_CTRL(0x91),
        IMX8MQ_PAD_GPIO1_IO09__GPIO1_IO9 | MUX_PAD_CTRL(0x19)
    );
    gpio_direction_output(IMX_GPIO_NR(1, 9), 0, io_ops);
    udelay(500);
    gpio_direction_output(IMX_GPIO_NR(1, 9), 1, io_ops);

    uint32_t *gpr1 = base + 0x4;
    /* Change ENET_TX to use internal clocks and not the external clocks */
    *gpr1 = *gpr1 & ~(BIT(17) | BIT(13));

#elif defined(CONFIG_PLAT_IMX8MM_EVK)

    /* Does PHY need to go into reset before this? */ 
    /* Pad configuration below from device tree on u-boot */ 
    IMX_IOMUX_V3_SETUP_MULTIPLE_PADS(
        base,
        IMX8MM_PAD_SPDIF_TX_GPIO5_IO3 | MUX_PAD_CTRL(0x19), 
        IMX8MM_PAD_SAI3_MCLK_GPIO5_IO2 | MUX_PAD_CTRL(0x19),
        IMX8MM_PAD_ENET_MDC_ENET1_MDC | MUX_PAD_CTRL(0x3),
        IMX8MM_PAD_ENET_MDIO_ENET1_MDIO | MUX_PAD_CTRL(0x3),
        IMX8MM_PAD_ENET_TD3_ENET1_RGMII_TD3 | MUX_PAD_CTRL(0x1f),
        IMX8MM_PAD_ENET_TD2_ENET1_RGMII_TD2 | MUX_PAD_CTRL(0x1f),
        IMX8MM_PAD_ENET_TD1_ENET1_RGMII_TD1 | MUX_PAD_CTRL(0x1f),
        IMX8MM_PAD_ENET_TD0_ENET1_RGMII_TD0 | MUX_PAD_CTRL(0x1f),
        IMX8MM_PAD_ENET_RD3_ENET1_RGMII_RD3 | MUX_PAD_CTRL(0x91),
        IMX8MM_PAD_ENET_RD2_ENET1_RGMII_RD2 | MUX_PAD_CTRL(0x91),
        IMX8MM_PAD_ENET_RD1_ENET1_RGMII_RD1 | MUX_PAD_CTRL(0x91),
        IMX8MM_PAD_ENET_RD0_ENET1_RGMII_RD0 | MUX_PAD_CTRL(0x91),
        IMX8MM_PAD_ENET_TXC_ENET1_RGMII_TXC | MUX_PAD_CTRL(0x1f),
        IMX8MM_PAD_ENET_RXC_ENET1_RGMII_RXC | MUX_PAD_CTRL(0x91),
        IMX8MM_PAD_ENET_RX_CTL_ENET1_RGMII_RX_CTL | MUX_PAD_CTRL(0x91),
        IMX8MM_PAD_ENET_TX_CTL_ENET1_RGMII_TX_CTL  | MUX_PAD_CTRL(0x1f),
        IMX8MM_PAD_GPIO1_IO09_GPIO1_IO9 | MUX_PAD_CTRL(0x19) // ENET1_1588_EVENT0_OUT 
    );
    gpio_direction_output(IMX_GPIO_NR(1, 9), 0, io_ops);
    udelay(500);
    gpio_direction_output(IMX_GPIO_NR(1, 9), 1, io_ops);
 
    uint32_t *gpr1 = base + 0x4;
    /* Change ENET_TX to use internal clocks and not the external clocks */
    *gpr1 = *gpr1 & ~(BIT(17) | BIT(13));

#elif defined(CONFIG_PLAT_IMX6DQ)

#define ENET_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP | \
                        PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm | PAD_CTL_HYS)


    /* PHY to GPIO mapping for strapping */
#define GPIO_NR_PHY_NRST        IMX_GPIO_NR(3, 23) // EIM_D23      to PHY pin 42 nRST
#define GPIO_NR_PHY_AD2         IMX_GPIO_NR(6, 30) // RGMII_RXC    to PHY pin 35 AD2
#define GPIO_NR_PHY_MODE0       IMX_GPIO_NR(6, 25) // RGMII_RD0    to PHY pin 32 MODE0
#define GPIO_NR_PHY_MODE1       IMX_GPIO_NR(6, 27) // RGMII_RD1    to PHY pin 31 MODE1
#define GPIO_NR_PHY_MODE2       IMX_GPIO_NR(6, 28) // RGMII_RD2    to PHY pin 28 MODE2
#define GPIO_NR_PHY_MODE3       IMX_GPIO_NR(6, 29) // RGMII_RD3    to PHY pin 27 MODE3
#define GPIO_NR_PHY_CLK125_EN   IMX_GPIO_NR(6, 24) // RGMII_RX_CTL to PHY pin 33 CLK125_EN

    /* put PHY into reset */
    gpio_direction_output(GPIO_NR_PHY_NRST,      0, io_ops);
    /* PHYAD=b00100 */
    gpio_direction_output(GPIO_NR_PHY_AD2,       1, io_ops);
    /* MODE=b1111 (RGMII Mode, 10/100/1000 speed, half/full duplex) */
    gpio_direction_output(GPIO_NR_PHY_MODE0,     1, io_ops);
    gpio_direction_output(GPIO_NR_PHY_MODE1,     1, io_ops);
    gpio_direction_output(GPIO_NR_PHY_MODE2,     1, io_ops);
    gpio_direction_output(GPIO_NR_PHY_MODE3,     1, io_ops);
    // enable 125MHz clock output
    gpio_direction_output(GPIO_NR_PHY_CLK125_EN, 1, io_ops);

    /* set pad configuration (after we have set well-defined GPIOs above) */
    IMX_IOMUX_V3_SETUP_MULTIPLE_PADS(
        base,
        MX6Q_PAD_ENET_MDIO__ENET_MDIO       | MUX_PAD_CTRL(ENET_PAD_CTRL),
        MX6Q_PAD_ENET_MDC__ENET_MDC         | MUX_PAD_CTRL(ENET_PAD_CTRL),
        MX6Q_PAD_RGMII_TXC__ENET_RGMII_TXC  | MUX_PAD_CTRL(ENET_PAD_CTRL),
        MX6Q_PAD_RGMII_TD0__ENET_RGMII_TD0  | MUX_PAD_CTRL(ENET_PAD_CTRL),
        MX6Q_PAD_RGMII_TD1__ENET_RGMII_TD1  | MUX_PAD_CTRL(ENET_PAD_CTRL),
        MX6Q_PAD_RGMII_TD2__ENET_RGMII_TD2  | MUX_PAD_CTRL(ENET_PAD_CTRL),
        MX6Q_PAD_RGMII_TD3__ENET_RGMII_TD3  | MUX_PAD_CTRL(ENET_PAD_CTRL),
        MX6Q_PAD_RGMII_TX_CTL__RGMII_TX_CTL | MUX_PAD_CTRL(ENET_PAD_CTRL),
        MX6Q_PAD_ENET_REF_CLK__ENET_TX_CLK  | MUX_PAD_CTRL(ENET_PAD_CTRL),
        // pad configuration as GPIO for PHY setup
        MX6Q_PAD_RGMII_RXC__GPIO_6_30       | MUX_PAD_CTRL(NO_PAD_CTRL),
        MX6Q_PAD_RGMII_RD0__GPIO_6_25       | MUX_PAD_CTRL(NO_PAD_CTRL),
        MX6Q_PAD_RGMII_RD1__GPIO_6_27       | MUX_PAD_CTRL(NO_PAD_CTRL),
        MX6Q_PAD_RGMII_RD2__GPIO_6_28       | MUX_PAD_CTRL(NO_PAD_CTRL),
        MX6Q_PAD_RGMII_RD3__GPIO_6_29       | MUX_PAD_CTRL(NO_PAD_CTRL),
        MX6Q_PAD_RGMII_RX_CTL__GPIO_6_24    | MUX_PAD_CTRL(NO_PAD_CTRL),
        MX6Q_PAD_EIM_D23__GPIO_3_23         | MUX_PAD_CTRL(NO_PAD_CTRL),
    );

    /* Need delay 10ms according to KSZ9021 spec */
    udelay(1000 * 10);
    /* release PHY from reset */
    gpio_set_value(GPIO_NR_PHY_NRST, 1);
    /* KSZ9021 spec recommends to wait 100us before sending any commands */
    udelay(100);

    /* reconfigure pins from GPIO for PHY setup to ethernet usage */
    IMX_IOMUX_V3_SETUP_MULTIPLE_PADS(
        base,
        MX6Q_PAD_RGMII_RXC__ENET_RGMII_RXC  | MUX_PAD_CTRL(ENET_PAD_CTRL),
        MX6Q_PAD_RGMII_RD0__ENET_RGMII_RD0  | MUX_PAD_CTRL(ENET_PAD_CTRL),
        MX6Q_PAD_RGMII_RD1__ENET_RGMII_RD1  | MUX_PAD_CTRL(ENET_PAD_CTRL),
        MX6Q_PAD_RGMII_RD2__ENET_RGMII_RD2  | MUX_PAD_CTRL(ENET_PAD_CTRL),
        MX6Q_PAD_RGMII_RD3__ENET_RGMII_RD3  | MUX_PAD_CTRL(ENET_PAD_CTRL),
        MX6Q_PAD_RGMII_RX_CTL__RGMII_RX_CTL | MUX_PAD_CTRL(ENET_PAD_CTRL),
    );

#elif defined(CONFIG_PLAT_IMX6SX)

    /*
     * default pad configuration for these pins
     *     PAD_ENETx_MDC
     *     PAD_ENETx_MDIO
     *     PAD_ENETx_RX_CLK
     *     PAD_ENETx_TX_CLK
     *     PAD_RGMIIx_TD[0-3]
     *     PAD_RGMIIx_RXC
     *     PAD_RGMIIx_RX_CTL
     *     PAD_RGMIIx_RD[0-3]
     *     PAD_RGMIIx_TXC
     *     PAD_RGMIIx_TX_CTL
     *   is
     *     HYS=0, PUS=00 (100KOHM_PD), PUE=0, PKE=1, ODE=0,
     *     SPEED=10 (Medium 100 MHz), DSE=110 (40OHM), SRE=0
     *
     *  PHY connection
     *    ENETx_RX_CLK <--> PHY INT
     *    ENETx_TX_CLK <--> PHY CLK_25M
     *
     *  PHY reset
     *      ENET2_CRS -> nRST of PHY1
     *      ENET2_COL -> nRST of PHY2
     *    there is not external pull-up, so GPIO port must enable one
     *
     * PHY MDIO
     *     MDC and MDIO have an internal pull-up when acting as input
     *     MDIO is an OD-gate, needs an external 1.5k pull-up
     *     both PHYs are connected to MDIO of ENET1
     *
     * PHY strapping pins
     *   PHYADDRESS[4:0] = 00xxx
     *       RXD0 (PHYADDRESS0, weak pull-down)        <- RGMIIx_RD0
     *       RXD1 (PHYADDRESS1, weak pull-down)        <- RGMIIx_RD1
     *       LED_ACT (PHYADDRESS3, weak pull-up)       <- PHY1: floating, PHY2: 10k pull down
     *   mode[3:0] = 1100 (RGMII, PLLOFF, INT)
     *       RX_DV (MODE0, weak pull-down)             <- RGMIIx_RX_CTL
     *       RXD2 (MODE1, weak pull-down)              <- RGMIIx_RD2
     *       LED_1000 (MODE2, weak pull up)            <- floating
     *       RXD3 (MODE3, weak pull-down)              <- RGMIIx_RD3
     *   IO level 1.8V
     *       RX_CLK (0=1.5V, 1=1.8V, weak pull-down)   <- RGMIIx_RXC
     *
     * The values below  are from u-boot of boundary devices 2020.10. Note that
     * PUS=00 is PAD_CTL_PUS_100K_DOWN, ie that is used if nothing is set
     */

#define PAD_CTL_ENET        (PAD_CTL_SPEED_HIGH | PAD_CTL_DSE_48ohm | PAD_CTL_SRE_FAST)
#define PAD_CTL_ENET_MD     (PAD_CTL_SPEED_MED | PAD_CTL_DSE_120ohm | PAD_CTL_SRE_FAST)
#define PAD_CTL_GPIO        (PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm | PAD_CTL_SRE_SLOW)

#define PAD_CTL_GPIO_IN_WEAK_PD    ( PAD_CTL_GPIO | PAD_CTL_HYS)
#define PAD_CTL_GPIO_IN_WEAK_PU    ( PAD_CTL_GPIO | PAD_CTL_HYS | PAD_CTL_PUS_100K_UP )

    /* PHY to GPIO mapping. */
#define GPIO_NR_PHY1_NRST       IMX_GPIO_NR(2, 7)  // uses ENET2_CRS
#define GPIO_NR_PHY1_ADDR0      IMX_GPIO_NR(5, 0)  // uses RGMII1_RD0
#define GPIO_NR_PHY1_ADDR1      IMX_GPIO_NR(5, 1)  // uses RGMII1_RD0
#define GPIO_NR_PHY1_MODE0      IMX_GPIO_NR(5, 4)  // uses RGMII1_RX_CTL
#define GPIO_NR_PHY1_MODE1      IMX_GPIO_NR(5, 2)  // uses RGMII1_RD2
#define GPIO_NR_PHY1_MODE3      IMX_GPIO_NR(5, 3)  // uses RGMII1_RD3
#define GPIO_NR_PHY1_VIO        IMX_GPIO_NR(5, 5)  // uses RGMII1_RXC
#define GPIO_NR_PHY1_INT        IMX_GPIO_NR(2, 4)  // uses ENET1_RX_CLK
#define GPIO_NR_PHY1_CLK_25M    IMX_GPIO_NR(2, 5)  // uses ENET1_TX_CLK

#define GPIO_NR_PHY2_NRST       IMX_GPIO_NR(2, 6)  // uses ENET2_COL
#define GPIO_NR_PHY2_ADDR0      IMX_GPIO_NR(5, 12) // uses RGMII2_RD0
#define GPIO_NR_PHY2_ADDR1      IMX_GPIO_NR(5, 13) // uses RGMII2_RD0
#define GPIO_NR_PHY2_MODE0      IMX_GPIO_NR(5, 16) // uses RGMII2_RX_CTL
#define GPIO_NR_PHY2_MODE1      IMX_GPIO_NR(5, 14) // uses RGMII2_RD2
#define GPIO_NR_PHY2_MODE3      IMX_GPIO_NR(5, 15) // uses RGMII2_RD3
#define GPIO_NR_PHY2_VIO        IMX_GPIO_NR(5, 17) // uses RGMII2_RXC
#define GPIO_NR_PHY2_INT        IMX_GPIO_NR(2, 8)  // uses ENET2_RX_CLK
#define GPIO_NR_PHY2_CLK_25M    IMX_GPIO_NR(2, 9)  // uses ENET2_TX_CLK

    /* PYH#1 for enet1: put into reset */
    gpio_direction_output(GPIO_NR_PHY1_NRST,  0, io_ops);
    /* PHY_ADDR = b00100 (4) */
    gpio_direction_output(GPIO_NR_PHY1_ADDR0, 0, io_ops);
    gpio_direction_output(GPIO_NR_PHY1_ADDR1, 0, io_ops);
    /* MODE = b0011 (RGMII, PLLOFF, INT)*/
    gpio_direction_output(GPIO_NR_PHY1_MODE0, 0, io_ops);
    gpio_direction_output(GPIO_NR_PHY1_MODE1, 0, io_ops);
    gpio_direction_output(GPIO_NR_PHY1_MODE3, 1, io_ops);
    /* I/O voltage is 1.8V */
    gpio_direction_output(GPIO_NR_PHY1_VIO,   1, io_ops);

    /* get interrupt from PHY */
    gpio_direction_input(GPIO_NR_PHY1_INT, io_ops);
    /* get 25 MHz clock input from PHY */
    gpio_direction_input(GPIO_NR_PHY1_CLK_25M, io_ops);

    /* PYH#2 for enet2: put into reset */
    gpio_direction_output(GPIO_NR_PHY2_NRST,  0, io_ops);
    /* PHYADDR = b00101 (5) */
    gpio_direction_output(GPIO_NR_PHY2_ADDR0, 1, io_ops);
    gpio_direction_output(GPIO_NR_PHY2_ADDR1, 0, io_ops);
    /* MODE = b0011 (RGMII, PLLOFF, INT)*/
    gpio_direction_output(GPIO_NR_PHY2_MODE0, 0, io_ops);
    gpio_direction_output(GPIO_NR_PHY2_MODE1, 0, io_ops);
    gpio_direction_output(GPIO_NR_PHY2_MODE3, 1, io_ops);
    /* I/O voltage is 1.8V */
    gpio_direction_output(GPIO_NR_PHY2_VIO,   1, io_ops);

    /* get 25 MHz clock input from PHY */
    gpio_direction_input(GPIO_NR_PHY2_CLK_25M, io_ops);
    /* get interrupt from PHY */
    gpio_direction_input(GPIO_NR_PHY2_INT, io_ops);

    /* set pad configuration (after we have set GPIOs) */
    IMX_IOMUX_V3_SETUP_MULTIPLE_PADS(
        base,
        /* shared mdio */
        MX6SX_PAD_ENET1_MDC__ENET1_MDC        | MUX_PAD_CTRL(PAD_CTL_ENET_MD | PAD_CTL_PUS_100K_UP),
        MX6SX_PAD_ENET1_MDIO__ENET1_MDIO      | MUX_PAD_CTRL(PAD_CTL_ENET_MD),
        /* fec1 */
        MX6SX_PAD_RGMII1_TD0__ENET1_TX_DATA_0 | MUX_PAD_CTRL(PAD_CTL_ENET),
        MX6SX_PAD_RGMII1_TD1__ENET1_TX_DATA_1 | MUX_PAD_CTRL(PAD_CTL_ENET),
        MX6SX_PAD_RGMII1_TD2__ENET1_TX_DATA_2 | MUX_PAD_CTRL(PAD_CTL_ENET),
        MX6SX_PAD_RGMII1_TD3__ENET1_TX_DATA_3 | MUX_PAD_CTRL(PAD_CTL_ENET),
        MX6SX_PAD_RGMII1_TXC__ENET1_RGMII_TXC | MUX_PAD_CTRL(PAD_CTL_ENET),
        MX6SX_PAD_RGMII1_TX_CTL__ENET1_TX_EN  | MUX_PAD_CTRL(PAD_CTL_ENET),
        MX6SX_PAD_ENET1_RX_CLK__GPIO2_IO_4    | MUX_PAD_CTRL(PAD_CTL_GPIO_IN_WEAK_PU),
        MX6SX_PAD_ENET1_TX_CLK__GPIO2_IO_5    | MUX_PAD_CTRL(PAD_CTL_GPIO_IN_WEAK_PU),

        /* PHY Reset */
        MX6SX_PAD_ENET2_CRS__GPIO2_IO_7       | MUX_PAD_CTRL(PAD_CTL_GPIO),
        /* PHY strapping input */
        MX6SX_PAD_RGMII1_RD0__GPIO5_IO_0      | MUX_PAD_CTRL(PAD_CTL_GPIO),
        MX6SX_PAD_RGMII1_RD1__GPIO5_IO_1      | MUX_PAD_CTRL(PAD_CTL_GPIO),
        MX6SX_PAD_RGMII1_RD2__GPIO5_IO_2      | MUX_PAD_CTRL(PAD_CTL_GPIO),
        MX6SX_PAD_RGMII1_RD3__GPIO5_IO_3      | MUX_PAD_CTRL(PAD_CTL_GPIO),
        MX6SX_PAD_RGMII1_RX_CTL__GPIO5_IO_4   | MUX_PAD_CTRL(PAD_CTL_GPIO),
        MX6SX_PAD_RGMII1_RXC__GPIO5_IO_5      | MUX_PAD_CTRL(PAD_CTL_GPIO),

        /* fec2 */
        MX6SX_PAD_RGMII2_TD0__ENET2_TX_DATA_0 | MUX_PAD_CTRL(PAD_CTL_ENET),
        MX6SX_PAD_RGMII2_TD1__ENET2_TX_DATA_1 | MUX_PAD_CTRL(PAD_CTL_ENET),
        MX6SX_PAD_RGMII2_TD2__ENET2_TX_DATA_2 | MUX_PAD_CTRL(PAD_CTL_ENET),
        MX6SX_PAD_RGMII2_TD3__ENET2_TX_DATA_3 | MUX_PAD_CTRL(PAD_CTL_ENET),
        MX6SX_PAD_RGMII2_TXC__ENET2_RGMII_TXC | MUX_PAD_CTRL(PAD_CTL_ENET),
        MX6SX_PAD_RGMII2_TX_CTL__ENET2_TX_EN  | MUX_PAD_CTRL(PAD_CTL_ENET),
        MX6SX_PAD_ENET2_RX_CLK__GPIO2_IO_8    | MUX_PAD_CTRL(PAD_CTL_GPIO_IN_WEAK_PU),
        MX6SX_PAD_ENET2_TX_CLK__GPIO2_IO_9    | MUX_PAD_CTRL(PAD_CTL_GPIO_IN_WEAK_PU),

        /* PHY Reset */
        MX6SX_PAD_ENET2_COL__GPIO2_IO_6       | MUX_PAD_CTRL(PAD_CTL_GPIO),
        /* PHY strapping input */
        MX6SX_PAD_RGMII2_RD0__GPIO5_IO_12     | MUX_PAD_CTRL(PAD_CTL_GPIO),
        MX6SX_PAD_RGMII2_RD1__GPIO5_IO_13     | MUX_PAD_CTRL(PAD_CTL_GPIO),
        MX6SX_PAD_RGMII2_RD2__GPIO5_IO_14     | MUX_PAD_CTRL(PAD_CTL_GPIO),
        MX6SX_PAD_RGMII2_RD3__GPIO5_IO_15     | MUX_PAD_CTRL(PAD_CTL_GPIO),
        MX6SX_PAD_RGMII2_RX_CTL__GPIO5_IO_16  | MUX_PAD_CTRL(PAD_CTL_GPIO),
        MX6SX_PAD_RGMII2_RXC__GPIO5_IO_17     | MUX_PAD_CTRL(PAD_CTL_GPIO),
    );

    /* AR8035 PHY specs say clock must be active 1 ms before releasing reset */
    udelay(1000);

    /* take PHYs out of reset, it will sample the IO pins for strapping*/
    gpio_set_value(GPIO_NR_PHY1_NRST, 1);
    gpio_set_value(GPIO_NR_PHY2_NRST, 1);

    /* AR8035 PHY specs don't mention strap hold time, us ms should be safe */
    udelay(50);

    IMX_IOMUX_V3_SETUP_MULTIPLE_PADS(
        base,
        /* enet1 */
        MX6SX_PAD_RGMII1_RD0__ENET1_RX_DATA_0 | MUX_PAD_CTRL(PAD_CTL_ENET),
        MX6SX_PAD_RGMII1_RD1__ENET1_RX_DATA_1 | MUX_PAD_CTRL(PAD_CTL_ENET),
        MX6SX_PAD_RGMII1_RD2__ENET1_RX_DATA_2 | MUX_PAD_CTRL(PAD_CTL_ENET),
        MX6SX_PAD_RGMII1_RD3__ENET1_RX_DATA_3 | MUX_PAD_CTRL(PAD_CTL_ENET),
        MX6SX_PAD_RGMII1_RX_CTL__ENET1_RX_EN | MUX_PAD_CTRL(PAD_CTL_ENET),
        MX6SX_PAD_RGMII1_RXC__ENET1_RX_CLK | MUX_PAD_CTRL(PAD_CTL_ENET),
        /* enet2 */
        MX6SX_PAD_RGMII2_RD0__ENET2_RX_DATA_0 | MUX_PAD_CTRL(PAD_CTL_ENET),
        MX6SX_PAD_RGMII2_RD1__ENET2_RX_DATA_1 | MUX_PAD_CTRL(PAD_CTL_ENET),
        MX6SX_PAD_RGMII2_RD2__ENET2_RX_DATA_2 | MUX_PAD_CTRL(PAD_CTL_ENET),
        MX6SX_PAD_RGMII2_RD3__ENET2_RX_DATA_3 | MUX_PAD_CTRL(PAD_CTL_ENET),
        MX6SX_PAD_RGMII2_RX_CTL__ENET2_RX_EN | MUX_PAD_CTRL(PAD_CTL_ENET),
        MX6SX_PAD_RGMII2_RXC__ENET2_RX_CLK | MUX_PAD_CTRL(PAD_CTL_ENET),
    );

#else
#error "unsupported platform"
#endif

    if (unmapOnExit) {
        UNRESOURCE(&io_ops->io_mapper, IOMUXC, base);
    }
    return 0;
}
