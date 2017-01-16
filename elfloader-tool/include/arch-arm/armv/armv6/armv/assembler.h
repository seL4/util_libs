/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */

#ifndef __ASSEMBLER_V6_H__
#define __ASSEMBLER_V6_H__

#include <mode/assembler.h>

/* This file contains useful macros for assembly code. */

#ifdef __ASSEMBLER__

#define      PIALL(reg)    p15, 0, reg, c7, c5, 4
#define        ISB(reg)    p15, 0, reg, c7, c5, 4
#define      DCALL(reg)    p15, 0, reg, c7, c10, 0
#define        DSB(reg)    p15, 0, reg, c7, c10, 4
#define     DCIALL(reg)    p15, 0, reg, c7, c14, 0

#else /* !__ASSEMBLER__ */
#warning "Including assembly-specific header in C code"
#endif

#endif /* __ASSEMBLER_V6_H__ */
