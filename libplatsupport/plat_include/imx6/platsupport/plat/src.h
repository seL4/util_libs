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
#ifndef PLAT_SRC_H
#define PLAT_SRC_H

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
    /* ---- */
    NSRCRST
};

#endif /* PLAT_SRC_H */
