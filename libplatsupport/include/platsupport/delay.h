/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#define ps_ndelay(ns) ps_udelay((ns) / 1000 + 1)
#define ps_mdelay(ms) ps_udelay((ms) * 1000)
#define ps_sdelay(s)  ps_mdelay((s) * 1000)

/**
 * Delay execution for at least the given number of microseconds. This is
 * a trivial function which simply spins in a loop. The actual length of
 * the delay depends on the current threads remaining time slice and the
 * number and priority of other threads in the system.
 * The use of this function should be avoided and replaced with calls to
 * usleep(...) where possible.
 * @param[in] us  The minimum number of microseconds to delay for
 */
void ps_udelay(unsigned long us);

/**
 * Provide the current CPU frequency to the libplatsupport delay module.
 * @parma[in] hz  An upper bound estimate of the current cpu frequency
 *                to ensure that the delay requests to ps_udelay can be
 *                met. If this function has not yet been called, a
 *                default frequency will be used.
 */
void ps_cpufreq_hint(unsigned long hz);

