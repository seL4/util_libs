/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef _PLATSUPPORT_PIT_H
#define _PLATSUPPORT_PIT_H

#include <platsupport/timer.h>
#include <platsupport/io.h>

#define PIT_INTERRUPT       0

/* 
 * Get the pit interface. This may only be called once.
 *
 * @param io_port_ops io port operations. This is all the pit requires.
 * @return initialised interface, NULL on error.
 */
pstimer_t * pit_get_timer(ps_io_port_ops_t *io_port_ops);


#endif /* _PLATSUPPORT_PIT_H */

