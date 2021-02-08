/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>

#include <platsupport/i2c.h>
#include <platsupport/plat/pmic.h>

#define MAX77686_BUSADDR  0x12
#define MAX77802_BUSADDR  0x12

typedef struct pmic {
    i2c_slave_t i2c_slave;
    i2c_kvslave_t kvslave;
    void* priv;;
} pmic_t;

enum ldo_mode {
    LDO_OFF,
    LDO_STANDBY,
    LDO_LOWPWR,
    LDO_ON
};

/**
 * Initialise the PMIC
 * @param[in]  i2c  A handle to the I2C bus that the PMIC is attached to
 * @param[in]  addr The slave address of the device
 * @param[out] pmic A pmic structure to initialise
 * @return          0 on success
 */
int pmic_init(i2c_bus_t* i2c, int addr, pmic_t* pmic);

/**
 * Print the status of of the PMIC and its power rails
 */
void pmic_print_status(pmic_t* pmic);

/**
 * Returns the number of LDOs that the PMIC controls
 * @param[in] pmic      A handle to the PMIC
 * @return              The number of LDOs that the PMIC controls,
 *                      or -1 on failure.
 */
int pmic_nldo(pmic_t* pmic);

/**
 * Configure a low dropout regulator
 * @param[in] pmic      A handle to the PMIC
 * @param[in] ldo       The LDO number, starting from 1
 * @param[in] ldo_mode  The modes at which the LDO should be switched on
 * @param[in] mili_volt The number of millivolts that the LDO should drive at
 * @return              On success, returns the actual output millivolts of the
 *                      regulator. Otherwise, returns -1.
 */
int pmic_ldo_cfg(pmic_t* pmic, int ldo, enum ldo_mode ldo_mode, int milli_volt);

/**
 * Configure a low dropout regulator
 * @param[in] pmic      A handle to the PMIC
 * @param[in] ldo       The LDO number, starting from 1
 * @param[out] ldo_mode If ldo_mode is not NULL and the request is successful,
 *                      ldo_mode will contain the current operating mode
 *                      configuration of the LDO.
 * @return              On success, returns the actual output millivolts of the
 *                      regulator. Otherwise, returns -1.
 */
int pmic_ldo_get_cfg(pmic_t* pmic, int ldo, enum ldo_mode* ldo_mode);

/**
 * Configures the reset delay of the reset button.
 * @param[in] pmic A handle to the PMIC
 * @param[in] ms   The number of milli seconds that the delay should be
 *                 configured to.
 * @return         On success, returns the actual number of milli seconds
 *                 that the delay was programmed to be. Otherwise, returns -1.
 */
int pmic_set_reset_delay(pmic_t* pmic, int ms);

/**
 * Retrieve the configured reset delay
 * @param[in] pmic A handle to the PMIC
 * @return         On success, returns the reset delay in milli seconds.
 *                 Otherwise, returns -1.
 */
int pmic_get_reset_delay(pmic_t* pmic);

