/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

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

