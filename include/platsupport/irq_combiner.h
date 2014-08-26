/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _PLATSUPPORT_IRQ_COMBINER_H_
#define _PLATSUPPORT_IRQ_COMBINER_H_

#include <platsupport/io.h>
#include <platsupport/plat/irq_combiner.h>

#define COMBINER_IRQ_FLAG         0x8000
#define IS_COMBINER_IRQ(x)        !!((x) & COMBINER_IRQ_FLAG)
#define COMBINER_IRQ(grp, idx)    (((grp) << 8) | (idx) | COMBINER_IRQ_FLAG)
#define COMBINER_IRQ_GET_GROUP(x) (((x) & ~COMBINER_IRQ_FLAG) >> 8)
#define COMBINER_IRQ_GET_INDEX(x) (((x) & ~COMBINER_IRQ_FLAG) & 0xff)

typedef struct irq_combiner irq_combiner_t;
typedef int combiner_irq_t;

struct irq_combiner {
    int (*is_pending)(irq_combiner_t* combiner, combiner_irq_t cirq);
    int (*is_enabled)(irq_combiner_t* combiner, combiner_irq_t cirq);
    int (*set_enabled)(irq_combiner_t* combiner, combiner_irq_t cirq, int v);
    uint32_t (*grp_pending)(irq_combiner_t* combiner, int group);
    void* priv;
};



/**
 * Initialise the IRQ combiner
 * @param[in]  id       The combiner device ID
 * @param[in]  io_ops   IO operations for accessing this device
 * @param[out] combiner An IRQ combiner structure to populate
 * @return              0 on success.
 */
int irq_combiner_init(enum irq_combiner_id id, ps_io_ops_t* io_ops, irq_combiner_t* combiner);

/**
 * Find the number of IRQ groups managed by the combiner. This will typically
 * be the number of IRQ groups.
 * @param[in] id        The combiner device ID
 * @return              The number of IRQ lines that this combiner manages.
 *                      -1 if the combiner id is invalid
 */
int irq_combiner_nirqs(enum irq_combiner_id id);

/**
 * Find the IRQ mapped to the requested group
 * @param[in] id        The combiner device ID
 * @param[in] group     The Combiner group number to query
 * @return              The IRQ number for the provided IRQ group
 *                      -1 if the combiner id is invalid
 */
int irq_combiner_irq(enum irq_combiner_id id, int group);

/**
 * Determine if a combiner IRQ is pending
 * @param[in] combiner A handle to an IRQ combiner
 * @param[in] cirq     A combiner IRQ id as constructed from COMBINER_IRQ(grp, idx)
 * @return             0 if the IRQ is not pending, 1 if the IRQ is pending, or
 *                     -1 on error.
 */
static inline int irq_combiner_is_pending(irq_combiner_t* combiner, combiner_irq_t cirq)
{
    assert(combiner);
    assert(combiner->is_pending);
    return combiner->is_pending(combiner, cirq);
}

/**
 * Determine if a combiner IRQ is enabled
 * @param[in] combiner A handle to an IRQ combiner
 * @param[in] cirq     A combiner IRQ id as constructed from COMBINER_IRQ(grp, idx)
 * @return             0 if the IRQ is not enabled, 1 if the IRQ is enabled, or
 *                     -1 on error.
 */
static inline int irq_combiner_is_enabled(irq_combiner_t* combiner, combiner_irq_t cirq)
{
    assert(combiner);
    assert(combiner->is_pending);
    return combiner->is_enabled(combiner, cirq);
}

/**
 * Enable a combiner IRQ index within a group. Does not enable the entire group
 * at the primary interrupt controller.
 * @param[in] combiner A handle to an IRQ combiner
 * @param[in] cirq     A combiner IRQ id as constructed from COMBINER_IRQ(grp, idx)
 * @return             0 on success.
 */
static inline int irq_combiner_enable_irq(irq_combiner_t* combiner, combiner_irq_t cirq)
{
    assert(combiner);
    assert(combiner->is_pending);
    return combiner->set_enabled(combiner, cirq, 1);
}


/**
 * Disable a combiner IRQ within a group. Does not disable the entire group
 * at the primary interrupt controller.
 * @param[in] combiner A handle to an IRQ combiner
 * @param[in] cirq     A combiner IRQ id as constructed from COMBINER_IRQ(grp, idx)
 * @return             0 on success.
 */
static inline int irq_combiner_disable_irq(irq_combiner_t* combiner, combiner_irq_t cirq)
{
    assert(combiner);
    assert(combiner->is_pending);
    return combiner->set_enabled(combiner, cirq, 0);
}

/**
 * Determine which IRQs in a Combiner group are pending
 * @param[in] combiner A handle to an IRQ combiner
 * @param[in] group    The Combiner group number to query
 * @return             A bitfield of pending IRQs for this group.
 */
static inline uint32_t irq_combiner_group_pending(irq_combiner_t* combiner, int group)
{
    assert(combiner);
    assert(combiner->is_pending);
    return combiner->grp_pending(combiner, group);
}

#endif /* _PLATSUPPORT_IRQ_COMBINER_H_ */
