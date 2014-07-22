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
#define MAX77686_CHIPID   0x02
/* This is the one in the server room... ? */
#define MAXXXXXX_CHIPID   0x91

#define VOLTAGE_MASK      0x3F

#define REG_CHIPID        0x00 /* 4 bytes */
#define REG_RESET_DELAY   0x0A
#define REG_VOUT_MMC2     0x43


typedef struct pmic {
    i2c_slave_t i2c_slave;
} pmic_t;

int pmic_init(i2c_bus_t* i2c, pmic_t* pmic);

int pmic_reg_read(pmic_t* pmic, uint8_t reg, void* data, int len);

int pmic_reg_write(pmic_t* pmic, uint8_t reg, const void* data, int len);

int pmic_get_reboot_delay(pmic_t* pmic);
int pmic_set_reboot_delay(pmic_t* pmic, int val);
int pmic_get_VDDQLCD(pmic_t* pmic);
int pmic_set_VDDQLCD(pmic_t* pmic, int val);
int pmic_get_VDDQLCD2(pmic_t* pmic);
int pmic_set_VDDQLCD2(pmic_t* pmic, int val);

int pmic_get_VDDIO(pmic_t* pmic);
int pmic_set_VDDIO(pmic_t* pmic, int val);
#endif /* __PLATSUPPORT_PLAT_PMIC__ */
