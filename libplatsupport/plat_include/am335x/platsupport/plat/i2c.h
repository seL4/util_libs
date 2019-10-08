/*
 * Copyright 2019, Data61
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

#include <utils/arith.h>

enum i2c_id {
    AM335X_I2C0,
    AM335X_I2C1,
    AM335X_I2C2,
    NI2C
};

#include <platsupport/i2c.h>

#define AM335X_I2C0_PADDR 0x44e0b000
#define AM335X_I2C1_PADDR 0x4802a000
#define AM335X_I2C2_PADDR 0x4819c000

#define AM335X_I2C0_IRQ 70
#define AM335X_I2C1_IRQ 71
#define AM335X_I2C2_IRQ 30

#define AM335X_I2C_SCLK 48000000
#define AM335X_I2C_MAX_FIFODEPTH 32

/* OMAP4 I2C device registers */
#define OMAP4_I2C_REVNB_LO        0x0
#define OMAP4_I2C_REVNB_HI        0x4
#define OMAP4_I2C_SYSC            0x10
#define OMAP4_I2C_IRQSTATUS_RAW   0x24
#define OMAP4_I2C_IRQSTATUS       0x28
#define OMAP4_I2C_IRQENABLE_SET   0x2C
#define OMAP4_I2C_IRQENABLE_CLR   0x30
#define OMAP4_I2C_WE              0x34
#define OMAP4_I2C_DMARXENABLE_SET 0x38
#define OMAP4_I2C_DMATXENABLE_SET 0x3C
#define OMAP4_I2C_DMARXENABLE_CLR 0x40
#define OMAP4_I2C_DMATXENABLE_CLR 0x44
#define OMAP4_I2C_DMARXWAKE_EN    0x48
#define OMAP4_I2C_DMATXWAKE_EN    0x4C
#define OMAP4_I2C_SYSS            0x90
#define OMAP4_I2C_BUF             0x94
#define OMAP4_I2C_CNT             0x98
#define OMAP4_I2C_DATA            0x9C
#define OMAP4_I2C_CON             0xA4
#define OMAP4_I2C_OA              0xA8
#define OMAP4_I2C_SA              0xAC
#define OMAP4_I2C_PSC             0xB0
#define OMAP4_I2C_SCLL            0xB4
#define OMAP4_I2C_SCLH            0xB8
#define OMAP4_I2C_SYSTEST         0xBC
#define OMAP4_I2C_BUFSTAT         0xC0
#define OMAP4_I2C_OA1             0xC4
#define OMAP4_I2C_OA2             0xC8
#define OMAP4_I2C_OA3             0xCC
#define OMAP4_I2C_ACTOA           0xD0
#define OMAP4_I2C_SBLOCK          0xD4

/* I2C_IRQSTATUS and I2C_IRQSTATUS_RAW fields */
#define IRQSTATUS_XDR   BIT(14)
#define IRQSTATUS_RDR   BIT(13)
#define IRQSTATUS_BB    BIT(12)
#define IRQSTATUS_ROVR  BIT(11)
#define IRQSTATUS_XUDF  BIT(10)
#define IRQSTATUS_AAS   BIT(9)
#define IRQSTATUS_BF    BIT(8)
#define IRQSTATUS_AERR  BIT(7)
#define IRQSTATUS_STC   BIT(6)
#define IRQSTATUS_GC    BIT(5)
#define IRQSTATUS_XRDY  BIT(4)
#define IRQSTATUS_RRDY  BIT(3)
#define IRQSTATUS_ARDY  BIT(2)
#define IRQSTATUS_NACK  BIT(1)
#define IRQSTATUS_AL    BIT(0)

/* I2C_IRQENABLE_SET fields */
#define IRQENABLE_XDR   BIT(14)
#define IRQENABLE_RDR   BIT(13)
/* no IRQENABLE_BB */
#define IRQENABLE_ROVR  BIT(11)
#define IRQENABLE_XUDF  BIT(10)
#define IRQENABLE_AAS   BIT(9)
#define IRQENABLE_BF    BIT(8)
#define IRQENABLE_AERR  BIT(7)
#define IRQENABLE_STC   BIT(6)
#define IRQENABLE_GC    BIT(5)
#define IRQENABLE_XRDY  BIT(4)
#define IRQENABLE_RRDY  BIT(3)
#define IRQENABLE_ARDY  BIT(2)
#define IRQENABLE_NACK  BIT(1)
#define IRQENABLE_AL    BIT(0)

/* I2C_BUF fields */
#define BUF_RXTRSH_OFFSET   8
#define BUF_RDMA_EN         BIT(15)
#define BUF_RXFIFO_CLR      BIT(14)
#define BUF_RXTRSH_MASK     (MASK(6) << BUF_RXTRSH_OFFSET)
#define BUF_RXDMA_EN        BIT(7)
#define BUF_TXFIFO_CLR      BIT(6)
#define BUF_TXTRSH_MASK     MASK(6)

/* I2C_CON fields */
#define CON_I2C_EN      BIT(15)
/* bit 14 reserved */
#define CON_OPMODE      (MASK(2) << 12)
#define CON_STB         BIT(11)
#define CON_MST         BIT(10)
#define CON_TRX         BIT(9)
#define CON_XSA         BIT(8)
#define CON_XOA0        BIT(7)
#define CON_XOA1        BIT(6)
#define CON_XOA2        BIT(5)
#define CON_XOA3        BIT(4)
/* bits 2-3 reserved */
#define CON_STP         BIT(1)
#define CON_STT         BIT(0)

int omap4_i2c_init(void *vaddr, int irq_id, ps_io_ops_t *io_ops, i2c_bus_t *i2c_bus);
