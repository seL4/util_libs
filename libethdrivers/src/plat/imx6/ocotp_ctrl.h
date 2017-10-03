/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */

#pragma once

#include <platsupport/io.h>

struct ocotp;

struct ocotp *ocotp_init(ps_io_mapper_t *io_mapper);
void ocotp_free(struct ocotp *ocotp, ps_io_mapper_t *io_mapper);

int ocotp_get_mac(struct ocotp* ocotp, unsigned char *mac);

