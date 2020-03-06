/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

static inline void wfi(void)
{
    asm volatile("mcr p15, 0, %0, c7, c0, 4" : : "r"(0) : "memory");
}

static inline void dsb(void)
{
    asm volatile("mcr p15, 0, %0, c7, c10, 4" : : "r"(0) : "memory");
}

static inline void dmb(void)
{
    asm volatile("mcr p15, 0, %0, c7, c10, 5" : : "r"(0) : "memory");
}

static inline void isb(void)
{
    asm volatile("mcr p15, 0, %0, c7, c5, 4" : : "r"(0) : "memory");
}

