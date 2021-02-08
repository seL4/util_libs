/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <autoconf.h>
#include <utils/gen_config.h>

#define COLOR_ERROR "\033[1;31m"
#define COLOR_NORMAL "\033[0m"

#if defined(CONFIG_ARCH_X86_64) || defined(CONFIG_ARCH_AARCH64)
#define DFMT    "%ld"
#define XFMT    "%lx"
#else
#define DFMT    "%d"
#define XFMT    "%x"
#endif
