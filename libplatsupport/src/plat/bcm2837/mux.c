/*
 * Copyright 2016, Data61 CSIRO
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(D61_BSD)
 */

#include <stdint.h>
#include <platsupport/gpio.h>
#include "../../services.h"

//#define MUX_DEBUG
#ifdef MUX_DEBUG
#define DMUX(...) printf("MUX: " __VA_ARGS__)
#else
#define DMUX(...) do{}while(0)
#endif


int
imx6_mux_init(void* iomuxc, mux_sys_t* mux)
{
    return -1;
}


int
mux_sys_init(ps_io_ops_t* io_ops, mux_sys_t* mux)
{
    return -1;
}
