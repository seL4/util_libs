/*
 * Configuration settings for the Freescale i.MX6Q Sabre Lite board.
 *
 * Copyright (C) 2010-2011 Freescale Semiconductor, Inc.
 * Copyright (C) 2020, HENSOLDT Cyber GmbH
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#pragma once

#include "imx-regs.h"
#include "gpio.h"

#define CONFIG_PHYLIB
#ifdef CONFIG_PLAT_IMX6SX
#define CONFIG_PHY_ATHEROS
#else
#define CONFIG_PHY_MICREL
#endif

int setup_iomux_enet(ps_io_ops_t *io_ops);
