/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <platsupport/io.h>

#define TK1_I2C_SIZE    (0x1000)
/* Unfortunately, the SPI controllers are not in separate physical frames.
 * Four of them are in one frame and the other 2 are in another.
 */
#define TK1_I2C_PAGE0   (0x7000C000)
#define TK1_I2C_PAGE1   (0x7000D000)

#define TK1_I2C0_PADDR  (TK1_I2C_PAGE0)
#define TK1_I2C0_SIZE   (TK1_I2C_SIZE)
#define TK1_I2C1_PADDR  (TK1_I2C_PAGE0 + 0x400)
#define TK1_I2C1_SIZE   (TK1_I2C_SIZE)
#define TK1_I2C2_PADDR  (TK1_I2C_PAGE0 + 0x500)
#define TK1_I2C2_SIZE   (TK1_I2C_SIZE)
#define TK1_I2C3_PADDR  (TK1_I2C_PAGE0 + 0x700)
#define TK1_I2C3_SIZE   (TK1_I2C_SIZE)
#define TK1_I2C4_PADDR  (TK1_I2C_PAGE1)
#define TK1_I2C4_SIZE   (TK1_I2C_SIZE)
#define TK1_I2C5_PADDR  (TK1_I2C_PAGE1 + 0x100)
#define TK1_I2C5_SIZE   (TK1_I2C_SIZE)

#define TK1_I2C0_INTERRUPT   (70)
#define TK1_I2C1_INTERRUPT   (116)
#define TK1_I2C2_INTERRUPT   (124)
#define TK1_I2C3_INTERRUPT   (152)
#define TK1_I2C4_INTERRUPT   (85)
#define TK1_I2C5_INTERRUPT   (95)

enum i2c_id {
    TK1_I2C0,
    TK1_I2C1,
    TK1_I2C2,
    TK1_I2C3,
    TK1_I2C4,
    TK1_I2C5,
    NI2C
};

struct i2c_bus;
typedef struct i2c_bus i2c_bus_t;

/** Static initializer.
 *
 * @param[in] controller_id     0-based ID for the controller being initialized.
 *                              Use one of the IDs defined above in enum i2c_id.
 * @param[in] vaddr             Virtual address of the page that is mapped to
 *                              the MMIO registers for the desired device.
 * @param[in] ib                Pointer to an uninitialized i2c_bus_t instance
 *                              which will be initialized by this function.
 * @return 0 if successful.
 */
int tegra_i2c_init(int controller_id, void *vaddr,
                   ps_io_ops_t *io_ops, i2c_bus_t *ib);

