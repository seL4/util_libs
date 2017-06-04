/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */
#ifndef SRC_H
#define SRC_H

#include <platsupport/io.h>
#include <platsupport/plat/src.h>

typedef struct src_dev {
    void* priv;
} src_dev_t;

/**
 * Initialise the system reset controller (SRC)
 * @param[in]  ops      Platform IO functions
 * @param[out] dev      A device structure to initialise
 * @return              0 on success
 */
int reset_controller_init(enum src_id, ps_io_ops_t* ops, src_dev_t* dev);

/**
 * Reset a subsubsystem.
 * On return, the subsystem will have completed the reset cycle
 * @param[in] dev  A handle to the system reset controller
 * @param[in] rst  A subsystem identifyer
 */
void reset_controller_assert_reset(src_dev_t* dev, enum src_rst_id);

#endif /* SRC_H */
