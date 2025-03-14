/*
 * Copyright 2022, HENSOLDT Cyber GmbH
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <platsupport/io.h>
#include <platsupport/ltimer.h>

#include "../../ltimer.h"

int ltimer_sp804_init(ltimer_t *ltimer, const char *ftd_path, uint64_t freq,
                      ps_io_ops_t ops, ltimer_callback_fn_t callback,
                      void *callback_token);
