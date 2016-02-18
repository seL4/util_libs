/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __PLATSUPPORT_PLAT_ZYNQ7000_DEVCFG_H
#define __PLATSUPPORT_PLAT_ZYNQ7000_DEVCFG_H

/* On the Zynq7000, the devcfg (Device Configuration Interface) is a
 * device made up of numerous configuration and status registers.
 *
 * For a full description of this device's registers, see
 * Zynq-7000 All Programable SoC Technical Reference Manual,
 * Appendix B.16: Device Configuration Interface (devcfg)
 */

#include <stdint.h>
#include <utils/arith.h>
#include <utils/attribute.h>
#include <platsupport/io.h>

struct devcfg_regs {
    uint32_t ctrl;              // 0x0000 Control
    uint32_t lock;              // 0x0004 Lock
    uint32_t cfg;               // 0x0008 Configuration
    uint32_t int_sts;           // 0x000c Interrupt Status
    uint32_t int_mask;          // 0x0010 Interrupt Mask
    uint32_t status;            // 0x0014 Status
    uint32_t dma_src_addr;      // 0x0018 DMA Source Address
    uint32_t dma_dst_addr;      // 0x001c DMA Destination Address
    uint32_t dma_src_len;       // 0x0020 DMA Source Length
    uint32_t dma_dest_len;      // 0x0024 DMA Destination Length
    PAD_STRUCT_BETWEEN(0x24, 0x2c, uint32_t);
    uint32_t multiboot_addr;    // 0x002c Multi Boot Address Pointer
    PAD_STRUCT_BETWEEN(0x2c, 0x34, uint32_t);
    uint32_t unlock;            // 0x0034 Unlock
    PAD_STRUCT_BETWEEN(0x34, 0x80, uint32_t);
    uint32_t mctrl;             // 0x0080 Miscellaneous Control
    PAD_STRUCT_BETWEEN(0x80, 0x100, uint32_t);
    uint32_t xadcif_cfg;        // 0x0100 XADC Interface Configuration
    uint32_t xadcif_int_sts;    // 0x0104 XADC Interface Interrupt Status
    uint32_t xadcif_int_mask;   // 0x0108 XADC Interface Interrupt Mask
    uint32_t xadcif_msts;       // 0x010c XADC Interface Miscellaneous Status
    uint32_t xadcif_cmdfifo;    // 0x0110 XADC Interface Command FIFO
    uint32_t xadcif_rdfifo;     // 0x0114 XADC Interface Data FIFO
    uint32_t xadcif_mctl;       // 0x0118 XADC Interface Miscellaneous Control
} PACKED;
typedef volatile struct devcfg_regs devcfg_regs_t;

/* Maps the devcfg registers into memory.
 * Returns 0 on success, -1 on error.
 */
int devcfg_init(ps_io_ops_t* ops);

/* Returns a pointer to the devcfg registers */
devcfg_regs_t* devcfg_get_regs(void);

#endif
