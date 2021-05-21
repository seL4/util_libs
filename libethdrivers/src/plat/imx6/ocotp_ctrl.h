/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2020, HENSOLDT Cyber GmbH
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <platsupport/io.h>

struct ocotp;

struct ocotp *ocotp_init(ps_io_mapper_t *io_mapper);
void ocotp_free(struct ocotp *ocotp, ps_io_mapper_t *io_mapper);

uint64_t ocotp_get_mac(struct ocotp *ocotp, unsigned int id);
