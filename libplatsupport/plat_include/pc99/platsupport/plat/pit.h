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

#include <autoconf.h>
#include <platsupport/timer.h>
#include <platsupport/io.h>

#ifdef CONFIG_IRQ_PIC
#define PIT_INTERRUPT       0
#else
#define PIT_INTERRUPT       2
#endif

/*
 * Get the pit interface. This may only be called once.
 *
 * @param io_port_ops io port operations. This is all the pit requires.
 * @return initialised interface, NULL on error.
 */
pstimer_t * pit_get_timer(ps_io_port_ops_t *io_port_ops);


#endif /* _PLATSUPPORT_PIT_H */

