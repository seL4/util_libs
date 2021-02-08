/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

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

