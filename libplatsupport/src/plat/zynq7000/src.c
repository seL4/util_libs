/*
 * Copyright 2015, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#include "src.h"

#define SLCR_PADDR                  0xF8000000  /* System Level Control Registers */
#define SLCR_SIZE                   0x1000

#define SLCR_LOCK_OFFSET            0x000       /* Offset of lock registers */
#define SLCR_CLK_OFFSET             0x100       /* Offset of clock registers */

#define LOCK_KEY   0x767B
#define UNLOCK_KEY 0xDF0D
#define SLCR_LOCKSTA_LOCKED BIT(0)

struct slcr_lock_regs {
    uint32_t scl;     /* 0x000 32 R/W 00000000h */
    uint32_t lock;    /* 0x004 32  W  00000000h */
    uint32_t unlock;  /* 0x008 32  W  00000000h */
    uint32_t locksta; /* 0x008 32  R  00000001h */
};
typedef volatile struct slcr_lock_regs slcr_lock_regs_t;

struct slcr_regs {
    slcr_lock_regs_t lock;
};
typedef volatile struct slcr_regs slcr_regs_t;

slcr_regs_t* slcr_regs;

static inline void
slcr_set_regs(src_dev_t* d, slcr_regs_t* r)
{
    d->priv = (void*)r;
}

static inline slcr_regs_t*
slcr_get_regs(src_dev_t* d)
{
    return (slcr_regs_t*)d->priv;
}

static inline int
reset_controller_unlock(src_dev_t* d)
{
    slcr_regs_t* r = slcr_get_regs(d);
    r->lock.unlock = UNLOCK_KEY;
    return !(r->lock.locksta & SLCR_LOCKSTA_LOCKED);
}

static inline int
reset_controller_lock(src_dev_t* d)
{
    slcr_regs_t* r = slcr_get_regs(d);
    r->lock.lock = LOCK_KEY;
    return !!(r->lock.locksta & SLCR_LOCKSTA_LOCKED);
}

void
reset_controller_assert_reset(src_dev_t* dev, enum src_rst_id id)
{
    (void)dev;
    (void)id;
}

void*
reset_controller_get_clock_regs(src_dev_t* dev)
{
    void* slcr = (void*)slcr_get_regs(dev);
    return (slcr + SLCR_CLK_OFFSET);
}

int
reset_controller_init(enum src_id id, ps_io_ops_t* ops, src_dev_t* dev)
{
    /* Input bounds check */
    if (id < 0 || id >= NSRC) {
        return -1;
    }
    /* Map in the slcr registers */
    slcr_regs = ps_io_map(&ops->io_mapper, SLCR_PADDR, SLCR_SIZE, 0, PS_MEM_NORMAL);
    if (slcr_regs == NULL) {
        return -1;
    }
    slcr_set_regs(dev, slcr_regs);
    /* Unlock the reset controller registers */
    reset_controller_unlock(dev);
    return 0;
}
