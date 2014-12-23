/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef UTILS_FORMATS_H
#define UTILS_FORMATS_H

#include <autoconf.h>

#define COLOR_ERROR "\033[1;31m"
#define COLOR_NORMAL "\033[0m"

#ifdef CONFIG_X86_64
#define DFMT    "%ld"
#define XFMT    "%lx"
#else
#define DFMT    "%d"
#define XFMT    "%x"
#endif

#endif
