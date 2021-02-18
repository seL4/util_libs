/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright 2021, Data61, CSIRO (ABN 41 687 119 230)
 */

#pragma once

#define CONFIG_PHY_REALTEK 1
#define CONFIG_SYS_HZ 1000 /* Current timer header uses milliseconds for get_timer */
#define CONFIG_ARM 1
#define CONFIG_ARCH_MESON 1
#define CONFIG_MESON_GXBB 1
#define CONFIG_TARGET_ODROID_C2 1
#define CONFIG_NET_RANDOM_ETHADDR 1
#define CONFIG_ETH_DESIGNWARE 1
#define CONFIG_PHYLIB 1
