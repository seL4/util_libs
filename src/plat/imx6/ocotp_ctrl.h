/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#ifndef _ETHDRIVER_IMX6_OCOTP_CONTROL_H_
#define _ETHDRIVER_IMX6_OCOTP_CONTROL_H_

#include <platsupport/io.h>

struct ocotp;

struct ocotp *ocotp_init(ps_io_mapper_t *io_mapper);
void ocotp_free(struct ocotp *ocotp, ps_io_mapper_t *io_mapper);

int ocotp_get_mac(struct ocotp* ocotp, unsigned char *mac);

#endif /* _ETHDRIVER_IMX6_OCOTP_CONTROL_H_ */
