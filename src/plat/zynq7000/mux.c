/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <stdint.h>
#include <platsupport/mux.h>
#include "../../services.h"

//#define MUX_DEBUG
#ifdef MUX_DEBUG
#define DMUX(...) printf("MUX: " __VA_ARGS__)
#else
#define DMUX(...) do{}while(0)
#endif

/* Zynq MUX is on the SLCR, along with just about everything else. */
struct slcr;

static struct zynq_mux {
    struct slcr* slcr;
} _mux;

static inline void set_mux_priv(mux_sys_t* mux, struct zynq_mux* zynq_mux)
{
    assert(mux != NULL);
    assert(zynq_mux != NULL);
    mux->priv = zynq_mux;
}

static int
zynq_mux_feature_enable(mux_sys_t* mux, enum mux_feature mux_feature)
{
    return 0;
}

static int
zynq_mux_init_common(mux_sys_t* mux)
{
    set_mux_priv(mux, &_mux);
    mux->feature_enable = &zynq_mux_feature_enable;
    return 0;
}

int
zynq_mux_init(mux_sys_t* mux)
{
    return zynq_mux_init_common(mux);
}

int
mux_sys_init(ps_io_ops_t* io_ops, mux_sys_t* mux)
{
    return zynq_mux_init_common(mux);
}


