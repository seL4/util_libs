/*
 * Copyright 2015, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef PLAT_SRC_H
#define PLAT_SRC_H

#include <platsupport/io.h>

enum src_id {
    SRC1,
    /* ---- */
    NSRC,
    SRC_DEFAULT = SRC1
};

enum src_rst_id {
    SRCRST_CORE3,
    SRCRST_CORE2,
    SRCRST_CORE1,
    SRCRST_CORE0,
    SRCRST_SW_IPU2,
    SRCRST_EIM,
    SRCRST_SW_OPEN_VG,
    SRCRST_SW_IPU1,
    SRCRST_SW_VPU,
    SRCRST_SW_GPU,
};

#endif /* PLAT_SRC_H */
