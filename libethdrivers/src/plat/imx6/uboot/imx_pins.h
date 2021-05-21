/*
 * Copyright (C) 2011 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2020, HENSOLDT Cyber GmbH
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

#include <utils/util.h>
#include <stdint.h>

#define NO_PAD_CTRL             BIT(17)

#define PAD_CTL_HYS             BIT(16)


#ifdef CONFIG_PLAT_IMX6SX

// according to u-boot, this setting apply to all i.MX6 platforms

#define PAD_CTL_PUS_100K_DOWN   (0 << 14 | PAD_CTL_PUE)
#define PAD_CTL_PUS_47K_UP      (1 << 14 | PAD_CTL_PUE)
#define PAD_CTL_PUS_100K_UP     (2 << 14 | PAD_CTL_PUE)
#define PAD_CTL_PUS_22K_UP      (3 << 14 | PAD_CTL_PUE)

#define PAD_CTL_PUE             ( BIT(13) | PAD_CTL_PKE)

#else

// it's a bit unclear if this just works by chance or some bits do not really
// matter on some platforms

#define PAD_CTL_PUS_100K_DOWN   (0 << 14)
#define PAD_CTL_PUS_47K_UP      (1 << 14)
#define PAD_CTL_PUS_100K_UP     (2 << 14)
#define PAD_CTL_PUS_22K_UP      (3 << 14)

#define PAD_CTL_PUE             BIT(13)

#endif


#define PAD_CTL_PKE             BIT(12)

#define PAD_CTL_ODE             BIT(11)

// u-boot 2018.07; iomux-v3.h has a spacial handling for i.MX6 SoloLite only,
// #if defined(CONFIG_MX6SL)
// #define PAD_CTL_SPEED_LOW    (1 << 6)
// #else
#define PAD_CTL_SPEED_LOW       (0 << 6)
// #endif
#define PAD_CTL_SPEED_MED       (2 << 6)
#define PAD_CTL_SPEED_HIGH      (3 << 6)

#define PAD_CTL_DSE_DISABLE     (0 << 3)
#define PAD_CTL_DSE_240ohm      (1 << 3)
#define PAD_CTL_DSE_120ohm      (2 << 3)
#define PAD_CTL_DSE_80ohm       (3 << 3)
#define PAD_CTL_DSE_60ohm       (4 << 3)
#define PAD_CTL_DSE_48ohm       (5 << 3)
#define PAD_CTL_DSE_40ohm       (6 << 3)
#define PAD_CTL_DSE_34ohm       (7 << 3)

#define PAD_CTL_SRE_SLOW        (0 << 0)
#define PAD_CTL_SRE_FAST        (1 << 0)

#define NO_MUX_I                0
#define NO_PAD_I                0

typedef uint64_t iomux_v3_cfg_t;

#define MUX_CTRL_OFS_SHIFT          0
#define MUX_CTRL_OFS(x)             ((iomux_v3_cfg_t)(x) << MUX_CTRL_OFS_SHIFT)
#define MUX_CTRL_OFS_MASK           MUX_CTRL_OFS(0xfff)

#define MUX_PAD_CTRL_OFS_SHIFT      12
#define MUX_PAD_CTRL_OFS(x)         ((iomux_v3_cfg_t)(x) << MUX_PAD_CTRL_OFS_SHIFT)
#define MUX_PAD_CTRL_OFS_MASK       MUX_PAD_CTRL_OFS(0xfff)

#define MUX_SEL_INPUT_OFS_SHIFT     24
#define MUX_SEL_INPUT_OFS(x)        ((iomux_v3_cfg_t)(x) << MUX_SEL_INPUT_OFS_SHIFT)
#define MUX_SEL_INPUT_OFS_MASK      MUX_SEL_INPUT_OFS(0xfff)

#define MUX_MODE_SHIFT              36
#define MUX_MODE(x)                 ((iomux_v3_cfg_t)(x) << MUX_MODE_SHIFT)
#define MUX_MODE_MASK               MUX_MODE(0x3f)

#define MUX_PAD_CTRL_SHIFT          42
#define MUX_PAD_CTRL(x)             ((iomux_v3_cfg_t)(x) << MUX_PAD_CTRL_SHIFT)
#define MUX_PAD_CTRL_MASK           MUX_PAD_CTRL(0x3ffff)

#define MUX_SEL_INPUT_SHIFT         60
#define MUX_SEL_INPUT(x)            ((iomux_v3_cfg_t)(x) << MUX_SEL_INPUT_SHIFT)
#define MUX_SEL_INPUT_MASK          MUX_SEL_INPUT(0xf)

#define IOMUX_PAD( \
            pad_ctrl_ofs, \
            mux_ctrl_ofs, \
            mux_mode, \
            sel_input_ofs,  \
            sel_input, \
            pad_ctrl) \
    ( MUX_CTRL_OFS(mux_ctrl_ofs) | \
      MUX_PAD_CTRL_OFS(pad_ctrl_ofs) | \
      MUX_SEL_INPUT_OFS(sel_input_ofs) | \
      MUX_MODE(mux_mode) | \
      MUX_PAD_CTRL(pad_ctrl) | \
      MUX_SEL_INPUT(sel_input) )

#define IOMUX_CONFIG_SION       BIT(4)
