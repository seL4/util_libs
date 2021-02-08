/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <platsupport/time_manager.h>
#include <platsupport/ltimer.h>
#include <platsupport/io.h>

/* Initialise a local time manager with a specific ltimer. The time manager uses the ltimer to set
 * timeouts and to read the current time.
 *
 * tm_update or tm_update_with_time must be called on this implementation
 * when interrupts come in on the ltimer for it to be correct.
 *
 * This timer manager implementation is reentrant but not thread safe: for multi threaded
 * access it should be guarded with a lock externally.
 *
 * @param tm        memory to initialise the time manager in
 * @param ltimer    valid ltimer. Must remain valid for this time manager to work.
 * @param ops       io_ops for initialisation. Not stored after the initial function is called.
 * @param size      max number of registered timeouts this time manager will have to handle at once.
 * @return          0 no success, EINVAL if arguments invalid, ENOMEM if not enough memory.
 */
int tm_init(time_manager_t *tm, ltimer_t *ltimer, ps_io_ops_t *ops, int size);
