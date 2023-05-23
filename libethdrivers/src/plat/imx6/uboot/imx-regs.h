/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * Copyright (C) 2011 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2020, HENSOLDT Cyber GmbH
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

#pragma once

#include <autoconf.h>
#include <stdint.h>

#define ARCH_MXC

#define CONFIG_SYS_CACHELINE_SIZE       32

#define AIPS1_ARB_BASE_ADDR         0x02000000
#define ATZ1_BASE_ADDR              AIPS1_ARB_BASE_ADDR
#define AIPS1_OFF_BASE_ADDR         (ATZ1_BASE_ADDR + 0x80000)

#define GPIO1_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x1C000)
#define GPIO2_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x20000)
#define GPIO3_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x24000)
#define GPIO4_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x28000)
#define GPIO5_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x2C000)
#define GPIO6_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x30000)
#define GPIO7_BASE_ADDR             (AIPS1_OFF_BASE_ADDR + 0x34000)
