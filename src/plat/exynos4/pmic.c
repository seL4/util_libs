/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */


#include <platsupport/plat/pmic.h>
#include <platsupport/plat/pmic_regs.h>
#include <string.h>

#define PMIC_DEBUG
#ifdef PMIC_DEBUG
#define dprintf(...) printf("PMIC:" __VA_ARGS__)
#else
#define dprintf(...) do{}while(0)
#endif


int
pmic_reg_read(pmic_t* pmic, uint8_t reg, void* data, int count){
    return i2c_kvslave_read(&pmic->i2c_slave, reg, data, count);
}

int
pmic_reg_write(pmic_t* pmic, uint8_t reg, const void* data, int count){
    return i2c_kvslave_write(&pmic->i2c_slave, reg, data, count);
}



int
pmic_init(i2c_bus_t* i2c, pmic_t* pmic){
    uint16_t chip_id;
    int ret;
    ret = i2c_kvslave_init(i2c, MAX77686_BUSADDR, LITTLE8, LITTLE8, &pmic->i2c_slave);
    if(ret){
        dprintf("Failed to register I2C slave\n");
        return -1;
    }
    /* Read the chip ID */
    ret = pmic_reg_read(pmic, REG_CHIPID, &chip_id, 2);
    if(ret != 2){
        dprintf("Bus error\n");
        return -1;
    }
    /* Check the chip ID */
    switch(chip_id){
    case MAX77686_CHIPID:
    case MAXXXXXX_CHIPID:
        dprintf("found chip ID 0x%x\n", chip_id);
        return 0;
    default:
        dprintf("Unidentified chip 0x%02x\n", chip_id);
        return -1;
    }
}

/***************
 *** Helpers ***
 ***************/
int
pmic_get_reboot_delay(pmic_t* pmic){
    uint8_t data;
    int ret = pmic_reg_read(pmic, PMICREG_RSTDELAY, &data, 1);
    return (ret == 1)? data : -1;
}

int
pmic_set_reboot_delay(pmic_t* pmic, int val){
    uint8_t data = (val < 0)? 0 : (val > 0xff)? 0xff : val;
    int ret = pmic_reg_write(pmic, PMICREG_RSTDELAY, &data, 1);
    return (ret == 1)? data : -1;
}


int
pmic_get_VDDQLCD(pmic_t* pmic){
    uint8_t data;
    int ret = pmic_reg_read(pmic, PMICREG_VDDQLCD, &data, 1);
    return (ret == 1)? data : -1;
}

int
pmic_set_VDDQLCD(pmic_t* pmic, int val){
    uint8_t data = (val < 0)? 0 : (val > 0xff)? 0xff : val;
    int ret = pmic_reg_write(pmic, PMICREG_VDDQLCD, &data, 1);
    return (ret == 1)? data : -1;
}

int
pmic_get_VDDQLCD2(pmic_t* pmic){
    uint8_t data;
    int ret = pmic_reg_read(pmic, PMICREG_VDDQLCD+0x20, &data, 1);
    return (ret == 1)? data : -1;
}


int
pmic_set_VDDQLCD2(pmic_t* pmic, int val){
    uint8_t data = (val < 0)? 0 : (val > 0xff)? 0xff : val;
    int ret = pmic_reg_write(pmic, PMICREG_VDDQLCD+ 0x20, &data, 1);
    return (ret == 1)? data : -1;
}

int
pmic_get_VDDIO(pmic_t* pmic){
    uint8_t data;
    int ret = pmic_reg_read(pmic, PMICREG_VDDIO, &data, 1);
    return (ret == 1)? data : -1;
}


int
pmic_set_VDDIO(pmic_t* pmic, int val){
    uint8_t data = (val < 0)? 0 : (val > 0xff)? 0xff : val;
    int ret = pmic_reg_write(pmic, PMICREG_VDDIO, &data, 1);
    return (ret == 1)? data : -1;
}

