/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __PLATSUPPORT_PLAT_PMIC__
#define __PLATSUPPORT_PLAT_PMIC__

#include <platsupport/i2c.h>

#define MAX77686_BUSADDR  0x12

#define LDO_VDDIO         3
#define LDO_MMC2          4
#define LDO_MMC13         5
#define LDO_EMMC          20
#define LDO_TFLASH        21
#define LDO_VDDQLCD       25

typedef struct pmic {
    i2c_slave_t i2c_slave;
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
 * @param[out] pmic A pmic structure to initialise
 * @return          0 on success
 */
int pmic_init(i2c_bus_t* i2c, pmic_t* pmic);

/**
 * Returns the number of LDOs that the PMIC controls including the reserved LDO0
 * @param[in] pmic      A handle to the PMIC
 * @return              The number of LDOs that the PMIC controls,
 *                      or -1 on failure.
 */
int pmic_nldo(pmic_t* pmic);

/**
 * Configure a low dropout regulator
 * @param[in] pmic      A handle to the PMIC
 * @param[in] ldo       The LDO index. 0 is reserved to maintain a logical map.
 * @param[in] ldo_mode  The modes at which the LDO should be switched on
 * @param[in] mili_volt The number of millivolts that the LDO should drive at
 * @return              On success, returns the actual output millivolts of the
 *                      regulator. Otherwise, returns -1.
 */
int pmic_ldo_cfg(pmic_t* pmic, int ldo, enum ldo_mode ldo_mode, int milli_volt);

/**
 * Configure a low dropout regulator
 * @param[in] pmic      A handle to the PMIC
 * @param[in] ldo       The LDO index. 0 is reserved to maintain a logical map.
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

#endif /* __PLATSUPPORT_PLAT_PMIC__ */

