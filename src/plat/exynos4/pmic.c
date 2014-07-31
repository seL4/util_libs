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
#include <string.h>

#define mV

#define PMIC_DEBUG
#ifdef PMIC_DEBUG
#define dprintf(...) printf("PMIC:" __VA_ARGS__)
#else
#define dprintf(...) do{}while(0)
#endif


#define REG_CHIPID        0x00
#define REG_RESET_DELAY   0x0A
#define REG_VOUT_LDO1     0x40
#define REG_VOUT_LDO(x)   (REG_VOUT_LDO1 + ((x) - 1))

#define MAX77686_CHIPID   0x02
#define MAXXXXXX_CHIPID   0x91

#define NLDO              (26 + 1 /* + reserved LDO0 */)
#define LDO_VMIN          800 mV
#define LDO_VSTEP          50 mV
#define LDO_VMASK         0x3F
#define LDO_MV(mv)        (( ((mv) + LDO_VSTEP / 2) - LDO_VMIN) / LDO_VSTEP)
#define LDO_GET_MV(reg)   (((reg) & LDO_VMASK) * LDO_VSTEP + LDO_VMIN)
#define LDOMODE_OFF       (0x0 << 6)
#define LDOMODE_STANDBY   (0x1 << 6)
#define LDOMODE_LOWPWR    (0x2 << 6)
#define LDOMODE_ON        (0x3 << 6)
#define LDOMODE_MASK      (0x3 << 6)

static int
pmic_reg_read(pmic_t* pmic, uint8_t reg, void* data, int count){
    return !(i2c_kvslave_read(&pmic->i2c_slave, reg, data, count) == count);
}

static int
pmic_reg_write(pmic_t* pmic, uint8_t reg, const void* data, int count){
    return !(i2c_kvslave_write(&pmic->i2c_slave, reg, data, count) == count);
}

static int
ldo_valid(pmic_t* pmic, int ldo){
    int nldo = pmic_nldo(pmic);
    return !(nldo < 0 || ldo <= 0 || ldo >= nldo);
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
    if(pmic_reg_read(pmic, REG_CHIPID, &chip_id, 2)){
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

int
pmic_nldo(pmic_t* pmic){
    (void)pmic;
    return NLDO;
}


int
pmic_ldo_cfg(pmic_t* pmic, int ldo, enum ldo_mode ldo_mode, int milli_volt){
    if(!ldo_valid(pmic, ldo)){
        return -1;
    }else{
        uint8_t v;
        /* Generate the register data */
        v = LDO_MV(milli_volt);
        switch(ldo_mode){
        case LDO_OFF:
            v |= LDOMODE_OFF;
            break;
        case LDO_STANDBY:
            v |= LDOMODE_STANDBY;
            break;
        case LDO_LOWPWR:
            v |= LDOMODE_LOWPWR;
            break;
        case LDO_ON:
            v |= LDOMODE_ON;
            break;
        default:
            /* Invalid mode */
            return -1;
        }
        /* Write the register */
        if(pmic_reg_write(pmic, REG_VOUT_LDO(ldo), &v, 1)){
            return -1;
        }else{
            return LDO_GET_MV(v);
        }
    }
}

int
pmic_ldo_get_cfg(pmic_t* pmic, int ldo, enum ldo_mode* ldo_mode){
    if(!ldo_valid(pmic, ldo)){
        return -1;
    }else{
        uint8_t v;
        /* Read in the register */
        if(pmic_reg_read(pmic, REG_VOUT_LDO(ldo), &v, 1)){
            return -1;
        }
        /* Decode the mode */
        if(ldo_mode != NULL){
            switch(v & LDOMODE_MASK){
            case LDOMODE_OFF:
                *ldo_mode = LDO_OFF;
                break;
            case LDOMODE_STANDBY:
                *ldo_mode = LDO_STANDBY;
                break;
            case LDOMODE_LOWPWR:
                *ldo_mode = LDO_LOWPWR;
                break;
            case LDOMODE_ON:
                *ldo_mode = LDO_ON;
                break;
            default:
                /* Should never get here */
                return -1;
            }
        }
        return LDO_GET_MV(v);
    }
}


int
pmic_get_reset_delay(pmic_t* pmic){
    uint8_t data;
    if(pmic_reg_read(pmic, REG_RESET_DELAY, &data, 1)){
        return -1;
    }else{
        return 1000 * (data >> 1);
    }
}

int
pmic_set_reset_delay(pmic_t* pmic, int ms){
    uint8_t data;
    /* Clip ms value */
    if(ms < 0){
        ms = 0;
    }else if(ms > 0xff){
        ms = 0xff;
    }
    /* Write the data */
    data = (ms / 1000) << 1;
    if(pmic_reg_write(pmic, REG_RESET_DELAY, &data, 1)){
        return -1;
    }else{
        return data;
    }
}


