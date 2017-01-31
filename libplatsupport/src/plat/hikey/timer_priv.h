/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(D61_BSD)
 */

#ifndef _HIKEY_TIMER_PRIV_H
#define _HIKEY_TIMER_PRIV_H

pstimer_t *hikey_dualtimer_get_timer(int timer_id, timer_config_t *config);
pstimer_t *hikey_rtc_get_timer(int timer_id, timer_config_t *config);
pstimer_t * hikey_vupcounter_get_timer(int rtc_id, int dualtimer_id,
                                       hikey_vupcounter_timer_config_t *config);

#define TCLR_AUTORELOAD BIT(6)

int dm_set_timeo(const pstimer_t *timer, uint64_t ns, int otherFlags);

#endif
