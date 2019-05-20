/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
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
