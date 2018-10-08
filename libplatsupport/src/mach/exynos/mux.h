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

#pragma once

#include <platsupport/mux.h>

/* Value encodings */
#define MUXVALUE_CPD(con, pud, drv) ((con) << 0 | (pud) << 4 | (drv) << 6)
#define MUXVALUE_CON(mval)          (((mval) >> 0) & 0xf)
#define MUXVALUE_PUD(mval)          (((mval) >> 4) & 0x3)
#define MUXVALUE_DRV(mval)          (((mval) >> 6) & 0x3)
/* PUD values */
#define PUD_NONE     0x0
#define PUD_PULLDOWN 0x1
#define PUD_PULLUP   0x3
/* DRV values */
#define DRV1X        0x0
#define DRV2X        0x2
#define DRV3X        0x1
#define DRV4X        0x3

struct mux_cfg {
    uint32_t con;
    uint32_t dat;
    uint32_t pud;
    uint32_t drv;
    uint32_t conpdn;
    uint32_t pudpdn;
    uint32_t res[2];
};
/* 448 */
struct mux_bank {
    struct mux_cfg gp[22];
    uint32_t res0[272];               /* 0x2C0 */
    uint32_t ext_int_con[22];         /* 0x700 */
    uint32_t res1[42];
    uint32_t ext_int_fltcon[22][2];   /* 0x800 */
    uint32_t res2[20];
    uint32_t ext_int_mask[22];        /* 0x900 */
    uint32_t res3[42];
    uint32_t ext_int_pend[22];        /* 0xA00 */
    uint32_t res4[42];
    uint32_t ext_int_grppri_xa;       /* 0xB00 */
    uint32_t ext_int_priority_xa;
    uint32_t ext_int_service_xa;
    uint32_t ext_int_service_pend_xa;
    uint32_t ext_int_grpfixpri_xa;
    uint32_t ext_int_fixpri[22];      /* 0xB14 */
    uint32_t res5[37];
    struct mux_cfg xgp[4];            /* 0xC00 */
    uint32_t res6[96];
    uint32_t ext_xint_con[4];        /* 0xE00 */
    uint32_t res7[28];
    uint32_t ext_xint_fltcon[4][2];  /* 0xE80 */
    uint32_t res8[24];
    uint32_t ext_xint_mask[4];       /* 0xF00 */
    uint32_t res9[12];
    uint32_t ext_xint_pend[4];       /* 0xF40 */
    uint32_t res10[44];
};

struct mux_feature_data {
    uint16_t port;
    uint8_t pin;
    uint8_t value;
};

extern struct mux_feature_data* feature_data[];

