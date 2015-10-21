/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef _PLATSUPPORT_PLAT_EGA_H
#define _PLATSUPPORT_PLAT_EGA_H

#include "../../chardev.h"

int text_ega_init(const struct dev_defn* defn, const ps_io_ops_t* ops, ps_chardevice_t* dev);

#endif /* _PLATSUPPORT_PLAT_EGA_H */
