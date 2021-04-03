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
#include "miiphy.h"
#include "micrel.h"
#include "imx_board.h"
#include <platsupport/io.h>
#include <stdio.h>

#if defined(CONFIG_PLAT_IMX6)

#include "mx6x_pins.h"

#define IOMUXC_PADDR    0x020E0000
#define IOMUXC_SIZE     0x4000

#elif defined(CONFIG_PLAT_IMX8MQ_EVK)

#include "imx8mq_pins.h"

#define IOMUXC_PADDR    0x30330000
#define IOMUXC_SIZE     0x10000

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
    // Change ENET_TX to use internal clocks and not the external clocks
    *gpr1 = *gpr1 & ~(BIT(17) | BIT(13));

#elif defined(CONFIG_PLAT_IMX6)

    gpio_direction_output(IMX_GPIO_NR(3, 23), 0, io_ops);
    gpio_direction_output(IMX_GPIO_NR(6, 30), 1, io_ops);
    gpio_direction_output(IMX_GPIO_NR(6, 25), 1, io_ops);
    gpio_direction_output(IMX_GPIO_NR(6, 27), 1, io_ops);
    gpio_direction_output(IMX_GPIO_NR(6, 28), 1, io_ops);
    gpio_direction_output(IMX_GPIO_NR(6, 29), 1, io_ops);

#define ENET_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP | \
                        PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm | PAD_CTL_HYS)

    IMX_IOMUX_V3_SETUP_MULTIPLE_PADS(
        base,
        MX6Q_PAD_ENET_MDIO__ENET_MDIO       | MUX_PAD_CTRL(ENET_PAD_CTRL),
        MX6Q_PAD_ENET_MDC__ENET_MDC     | MUX_PAD_CTRL(ENET_PAD_CTRL),
        MX6Q_PAD_RGMII_TXC__ENET_RGMII_TXC  | MUX_PAD_CTRL(ENET_PAD_CTRL),
        MX6Q_PAD_RGMII_TD0__ENET_RGMII_TD0  | MUX_PAD_CTRL(ENET_PAD_CTRL),
        MX6Q_PAD_RGMII_TD1__ENET_RGMII_TD1  | MUX_PAD_CTRL(ENET_PAD_CTRL),
        MX6Q_PAD_RGMII_TD2__ENET_RGMII_TD2  | MUX_PAD_CTRL(ENET_PAD_CTRL),
        MX6Q_PAD_RGMII_TD3__ENET_RGMII_TD3  | MUX_PAD_CTRL(ENET_PAD_CTRL),
        MX6Q_PAD_RGMII_TX_CTL__RGMII_TX_CTL | MUX_PAD_CTRL(ENET_PAD_CTRL),
        MX6Q_PAD_ENET_REF_CLK__ENET_TX_CLK  | MUX_PAD_CTRL(ENET_PAD_CTRL),
        /* pin 35 - 1 (PHY_AD2) on reset */
        MX6Q_PAD_RGMII_RXC__GPIO_6_30       | MUX_PAD_CTRL(NO_PAD_CTRL),
        /* pin 32 - 1 - (MODE0) all */
        MX6Q_PAD_RGMII_RD0__GPIO_6_25       | MUX_PAD_CTRL(NO_PAD_CTRL),
        /* pin 31 - 1 - (MODE1) all */
        MX6Q_PAD_RGMII_RD1__GPIO_6_27       | MUX_PAD_CTRL(NO_PAD_CTRL),
        /* pin 28 - 1 - (MODE2) all */
        MX6Q_PAD_RGMII_RD2__GPIO_6_28       | MUX_PAD_CTRL(NO_PAD_CTRL),
        /* pin 27 - 1 - (MODE3) all */
        MX6Q_PAD_RGMII_RD3__GPIO_6_29       | MUX_PAD_CTRL(NO_PAD_CTRL),
        /* pin 33 - 1 - (CLK125_EN) 125Mhz clockout enabled */
        MX6Q_PAD_RGMII_RX_CTL__GPIO_6_24    | MUX_PAD_CTRL(NO_PAD_CTRL),
        /* pin 42 PHY nRST */
        MX6Q_PAD_EIM_D23__GPIO_3_23     | MUX_PAD_CTRL(NO_PAD_CTRL)
    );

    gpio_direction_output(IMX_GPIO_NR(6, 24), 1, io_ops);
    /* Need delay 10ms according to KSZ9021 spec */
    udelay(1000 * 10);
    gpio_set_value(IMX_GPIO_NR(3, 23), 1);

    IMX_IOMUX_V3_SETUP_MULTIPLE_PADS(
        base,
        MX6Q_PAD_RGMII_RXC__ENET_RGMII_RXC  | MUX_PAD_CTRL(ENET_PAD_CTRL),
        MX6Q_PAD_RGMII_RD0__ENET_RGMII_RD0  | MUX_PAD_CTRL(ENET_PAD_CTRL),
        MX6Q_PAD_RGMII_RD1__ENET_RGMII_RD1  | MUX_PAD_CTRL(ENET_PAD_CTRL),
        MX6Q_PAD_RGMII_RD2__ENET_RGMII_RD2  | MUX_PAD_CTRL(ENET_PAD_CTRL),
        MX6Q_PAD_RGMII_RD3__ENET_RGMII_RD3  | MUX_PAD_CTRL(ENET_PAD_CTRL),
        MX6Q_PAD_RGMII_RX_CTL__RGMII_RX_CTL | MUX_PAD_CTRL(ENET_PAD_CTRL),
    );

#else
#error "unsupported platform"
#endif

    if (unmapOnExit) {
        UNRESOURCE(&io_ops->io_mapper, IOMUXC, base);
    }
    return 0;
}
