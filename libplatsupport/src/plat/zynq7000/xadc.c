/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <stdint.h>
#include <stdbool.h>

#include <utils/arith.h>
#include <utils/attribute.h>
#include <utils/force.h>
#include <platsupport/io.h>

#include "devcfg.h"

#define XADC_MSTS_CFIFOE    BIT(10)     // XADC Interafce Command FIFO empty
#define XADCIF_CFG_ENABLE   BIT(31)     // XADC Interface Configuration Enable
#define XADCIF_MCTL_RESET   BIT(4)      // XADC Interface Miscellaneous Control Reset

/* Constants/macros for formatting commands for the XADC */
#define XADC_DATA_OFFSET    0
#define XADC_DATA_MASK      MASK(16)

#define XADC_ADDRESS_OFFSET 16
#define XADC_ADDRESS_MASK   MASK(10)
#define XADC_VALID_ADDRESS_MASK MASK(6)

#define XADC_COMMAND_OFFSET 26
#define XADC_COMMAND_MASK   MASK(4)

#define FORMAT_XADC_COMMAND(command, address, data) (\
        (((data) & XADC_DATA_MASK) << XADC_DATA_OFFSET) |\
        (((address) & XADC_ADDRESS_MASK) << XADC_ADDRESS_OFFSET) |\
        (((command) & XADC_COMMAND_MASK) << XADC_COMMAND_OFFSET))

#define XADC_COMMAND_NOOP   0
#define XADC_COMMAND_READ   1
#define XADC_COMMAND_WRITE  2

#define FORMAT_XADC_READ(address) FORMAT_XADC_COMMAND(XADC_COMMAND_READ, address, 0)
#define XADC_NOOP FORMAT_XADC_COMMAND(XADC_COMMAND_NOOP, 0, 0)

static bool initialized = false;

int xadc_init(ps_io_ops_t* ops) {
    if (initialized) {
        return 0;
    }

    int error = devcfg_init(ops);
    if (error != 0) {
        return error;
    }

    devcfg_regs_t* devcfg_regs = devcfg_get_regs();
    if (devcfg_regs == NULL) {
        return -1;
    }

    devcfg_regs->xadcif_cfg |= XADCIF_CFG_ENABLE;
    devcfg_regs->xadcif_mctl &= ~XADCIF_MCTL_RESET;

    initialized = true;

    return 0;
}

uint32_t xadc_read_register(uint32_t address) {
    devcfg_regs_t* devcfg_regs = devcfg_get_regs();

    // write the command
    devcfg_regs->xadcif_cmdfifo = FORMAT_XADC_READ(address & XADC_VALID_ADDRESS_MASK);

    // wait for the command to leave the fifo
    while (!(devcfg_regs->xadcif_msts & XADC_MSTS_CFIFOE));

    // do a dummy read
    FORCE_READ(&devcfg_regs->xadcif_rdfifo);

    // send a noop to shift the result into rdfifo
    devcfg_regs->xadcif_cmdfifo = XADC_NOOP;

    // read the result
    return devcfg_regs->xadcif_rdfifo;
}
