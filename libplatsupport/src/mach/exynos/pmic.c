/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#include <utils/util.h>
#include <platsupport/mach/pmic.h>
#include <string.h>

#define mV

#define REG_CHIPID        0x00
#define REG_RESET_DELAY   0x0A

#define MAX77686_CHIPID   0x02
#define MAX77802_CHIPID   0x06
#define MAXXXXXX_CHIPID   0x91

#define NLDO              (26 + 1 /* + reserved LDO0 */)
#define LDO_VMIN          800 mV
#define LDO_VSTEP          50 mV
#define LDO_VMASK         0x3F
#define LDO_MV(mv)        (( ((mv) + LDO_VSTEP / 2) - LDO_VMIN) / LDO_VSTEP)
#define LDO_GET_MV(reg)   (((reg) & LDO_VMASK) * LDO_VSTEP + LDO_VMIN)
#define LDOMODE_OFF       (0x0 << 6)
#define LDOMODE_STANDBY   (BIT(6))
#define LDOMODE_LOWPWR    (0x2 << 6)
#define LDOMODE_ON        (0x3 << 6)
#define LDOMODE_MASK      (0x3 << 6)

struct max77_config {
    int ctrl1_start;
    int ctrl2_start;
    int nldo;
};

static const struct max77_config max77802_cfg = {
    .ctrl1_start = 0x60,
    .ctrl2_start = 0x90,
    .nldo        = 35
};

static const struct max77_config max77686_cfg = {
    .ctrl1_start = 0x40,
    .ctrl2_start = 0x60,
    .nldo        = 26
};

static inline const struct max77_config*
pmic_get_priv(pmic_t* pmic) {
    return (const struct max77_config*)pmic->priv;
}

static int
pmic_reg_read(pmic_t* pmic, uint8_t reg, void* data, int count)
{
    return !(i2c_kvslave_read(&pmic->kvslave, reg, data, count) == count);
}

static int
pmic_reg_write(pmic_t* pmic, uint8_t reg, const void* data, int count)
{
    return !(i2c_kvslave_write(&pmic->kvslave, reg, data, count) == count);
}

static int
ldo_valid(pmic_t* pmic, int ldo)
{
    int nldo = pmic_nldo(pmic);
    return !(nldo < 0 || ldo <= 0 || ldo > nldo);
}

int
pmic_init(i2c_bus_t* i2c, int addr, pmic_t* pmic)
{
    uint16_t chip_id;
    int ret;
    ret = i2c_slave_init(i2c, addr,
                         I2C_SLAVE_ADDR_7BIT, I2C_SLAVE_SPEED_FAST,
                         0, &pmic->i2c_slave);
    if (ret) {
        ZF_LOGD("Failed to register I2C slave");
        return -1;
    }

    ret = i2c_kvslave_init(&pmic->i2c_slave, LITTLE8, LITTLE8, &pmic->kvslave);
    if (ret) {
        ZF_LOGE("Failed to initialize I2C KV-slave lib instance.");
        return -1;
    }

    /* Read the chip ID */
    if (pmic_reg_read(pmic, REG_CHIPID, &chip_id, 2)) {
        ZF_LOGD("Bus error");
        return -1;
    }
    /* Check the chip ID */
    switch (chip_id) {
    case MAX77686_CHIPID:
        pmic->priv = (void*)&max77686_cfg;
        break;
    case MAXXXXXX_CHIPID:
    case MAX77802_CHIPID:
        pmic->priv = (void*)&max77802_cfg;
        break;
    default:
        ZF_LOGD("Unidentified chip 0x%02x", chip_id);
        return -1;
    }

    ZF_LOGD("found chip ID 0x%x", chip_id);
    return 0;
}

int
pmic_nldo(pmic_t* pmic)
{
    const struct max77_config* cfg = pmic_get_priv(pmic);
    assert(cfg);
    return cfg->nldo;
}

int
pmic_ldo_cfg(pmic_t* pmic, int ldo, enum ldo_mode ldo_mode, int milli_volt)
{
    if (!ldo_valid(pmic, ldo)) {
        return -1;
    } else {
        const struct max77_config* cfg;
        uint8_t v;
        cfg = pmic_get_priv(pmic);
        assert(cfg);
        /* Generate the register data */
        v = LDO_MV(milli_volt);
        switch (ldo_mode) {
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
        if (pmic_reg_write(pmic, cfg->ctrl1_start + ldo - 1, &v, 1)) {
            return -1;
        } else {
            return LDO_GET_MV(v);
        }
    }
}

int
pmic_ldo_get_cfg(pmic_t* pmic, int ldo, enum ldo_mode* ldo_mode)
{
    if (!ldo_valid(pmic, ldo)) {
        return -1;
    } else {
        const struct max77_config* cfg;
        uint8_t v;
        cfg = pmic_get_priv(pmic);
        assert(cfg);
        /* Read in the register */
        if (pmic_reg_read(pmic, cfg->ctrl1_start + ldo - 1, &v, 1)) {
            return -1;
        }
        /* Decode the mode */
        if (ldo_mode != NULL) {
            switch (v & LDOMODE_MASK) {
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
pmic_get_reset_delay(pmic_t* pmic)
{
    uint8_t data;
    if (pmic_reg_read(pmic, REG_RESET_DELAY, &data, 1)) {
        return -1;
    } else {
        return 1000 * (data >> 1);
    }
}

int
pmic_set_reset_delay(pmic_t* pmic, int ms)
{
    uint8_t data;
    /* Clip ms value */
    if (ms < 0) {
        ms = 0;
    } else if (ms > 10000) {
        ms = 10000;
    }
    /* Write the data */
    data = (ms / 1000) << 1;
    if (pmic_reg_write(pmic, REG_RESET_DELAY, &data, 1)) {
        return -1;
    } else {
        return data;
    }
}

void
pmic_print_status(pmic_t* pmic)
{
    uint8_t data;
    int nldo;
    int err;
    int i;
    printf("### PMIC ###\n");
    err = pmic_reg_read(pmic, REG_CHIPID, &data, 1);
    assert(!err);
    if (err) {
        return;
    }
    switch (data) {
    case MAX77686_CHIPID:
        printf("MAX77686");
        break;
    case MAX77802_CHIPID:
        printf("MAX77802");
        break;
    case MAXXXXXX_CHIPID:
        printf("MAXXXXXx");
        break;
    default:
        printf("Unknown CHIP");
    }
    nldo = pmic_nldo(pmic);
    printf(": %d LDOs\n", nldo);
    for (i = 1; i < nldo; i++) {
        int mv, v;
        enum ldo_mode mode;
        mv = pmic_ldo_get_cfg(pmic, i, &mode);
        v = mv / 1000;
        mv -= v * 1000;
        printf("LDO%02d: %d.%03dV (", i, v, mv);
        switch (mode) {
        case LDO_OFF:
            printf("Off");
            break;
        case LDO_STANDBY:
            printf("Standby");
            break;
        case LDO_LOWPWR:
            printf("Low power");
            break;
        case LDO_ON:
            printf("On");
            break;
        default:
            printf("Unknown state");
        }
        printf(")\n");
    }
    printf("Reset delay: %d ms\n", pmic_get_reset_delay(pmic));
}
