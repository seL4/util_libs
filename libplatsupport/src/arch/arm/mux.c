/*
 * Copyright 2018, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#include <utils/attribute.h>
#include <platsupport/mux.h>

/**
 * Weak symbol definition of mux_sys_init. Platforms should provide
 * their own symbol with an implementation
 */
WEAK int
mux_sys_init(ps_io_ops_t* io_ops UNUSED, void *dependencies UNUSED,
             mux_sys_t* mux UNUSED)
{
    return 0;
}
