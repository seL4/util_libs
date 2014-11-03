/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __PLATSUPPORT_DELAY_H__
#define __PLATSUPPORT_DELAY_H__

#define ps_nsleep(ns) ps_nsleep((ns) / 1000 + 1)
#define ps_msleep(ms) ps_usleep((ms) * 1000)
#define ps_ssleep(s)  ps_msleep((s) * 1000)


#define ps_ndelay(ns) ps_ndelay((ns) / 1000 + 1)
#define ps_mdelay(ms) ps_udelay((ms) * 1000)
#define ps_sdelay(s)  ps_mdelay((s) * 1000)

/**
 * Delay execution for at least the given number of microseconds. A call
 * to this function may affect scheduling behaviour. This function will
 * be an alias of ps_udelay if it is not overloaded.
 * @param[in] us  The minimum number of microseconds to delay for
 */
void ps_usleep(long us);

/**
 * Delay execution for at least the given number of microseconds. This is
 * a trivial function which simply spins in a loop. The actual length of
 * the delay depends on the current threads remaining time slice and the
 * number and priority of other threads in the system.
 * The use of this function should be avoided and replaced with calls to
 * ps_usleep where possible.
 * @param[in] us  The minimum number of microseconds to delay for
 */
void ps_udelay(long us);

/**
 * Provide the current CPU frequency to the libplatsupport delay module.
 * @parma[in] hz  An upper bound estimate of the current cpu frequency
 *                to ensure that the delay requests to ps_udelay can be
 *                met. If this function has not yet been called, a
 *                default frequency will be used.
 */
void ps_cpufreq_hint(long hz);


#endif /* __PLATSUPPORT_DELAY_H__ */
