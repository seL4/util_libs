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

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <utils/util.h>
#include <utils/arith.h>
#include <utils/stringify.h>
#include <utils/attribute.h>

#include <platsupport/io.h>
#include <platsupport/mux.h>
#include <platsupport/gpio.h>
#include <platsupport/plat/gpio.h>

#include "mux_gpio_priv.h"
#include "../../services.h"

/** @file TK1 Mux driver.
 *
 * This file contains routines that manipulate the TK1 mux controller and set
 * up the signals on pins.
 *
 *  PREREQUISITES:
 * This driver currently assumes that the MMIO registers it accesses are mapped
 * as strongly ordered and uncached. The driver makes no attempts whatsoever at
 * managing the write buffer or managing ordering of reads and writes.
 */

#define MUX_REG_LOCK_SHIFT          (7)
#define MUX_REG_OPEN_DRAIN_SHIFT    (6)
#define MUX_REG_ENABLE_SHIFT        (5)
#define MUX_REG_TRISTATE_SHIFT      (4)
#define MUX_REG_PUPD_SHIFT          (2)
#define MUX_REG_PUPD_MASK           (0x3)
#define MUX_REG_SFIO_SELECT_SHIFT   (0)
#define MUX_REG_SFIO_SELECT_MASK    (0x3)

#define MUX_REG_PUPD_NORMAL         (0)
#define MUX_REG_PUPD_PULLDOWN       (1)
#define MUX_REG_PUPD_PULLUP         (2)

#define MUX_REG_TRISTATE_NORMAL     (0)
#define MUX_REG_TRISTATE_TRISTATE   (1)

/* Throughout this driver, the terms "pad" and "pin" are interchangeable.
 *
 * Each pin's input and output buffers are distinct and can be enabled
 * separately and operated concurrently (to support bidirectional signals).
 *
 * So we need to specify for each pin whether one, or both (or neither) of the
 * buffers needs to be enabled.
 *
 * Furthermore, each pin's output buffer can be driven by one of several output
 * drivers: push-pull, open-drain, high-z, weak-pullup, weak-pulldown.
 */
#define P_IN                            BIT(0)
#define P_PUSHPULL                      BIT(6)
#define P_BOTH                          (P_IN | P_PUSHPULL)
#define P_OPEN_DRAIN                    BIT(2)
#define P_TRISTATE                      BIT(3)
#define P_PULLUP                        BIT(4)
#define P_PULLDOWN                      BIT(5)

#define PINMAP_PINDESC(_gp, _mro, _msv, _in_out) \
    { \
        .gpio_pin = _gp, .mux_reg_index = _mro, .mux_sfio_value = _msv, \
        .flags = _in_out \
    }

#define PINMAP_NULLDESC PINMAP_PINDESC(-1, -1, 0xF, 0)

#define PINMAP_1PIN(_n, _gp0, _mro0, _msv0, _io0) \
    [_n] = { \
        .name = STRINGIFY(_n), \
        { \
            PINMAP_PINDESC(_gp0, _mro0, _msv0, _io0), \
            PINMAP_NULLDESC, PINMAP_NULLDESC, PINMAP_NULLDESC, PINMAP_NULLDESC, \
            PINMAP_NULLDESC, PINMAP_NULLDESC, PINMAP_NULLDESC \
        } \
    }

#define PINMAP_2PIN(_n, _gp0, _mro0, _msv0, _io0, _gp1, _mro1, _msv1, _io1) \
    [_n] = { \
        .name = STRINGIFY(_n), \
        { \
            PINMAP_PINDESC(_gp0, _mro0, _msv0, _io0), \
            PINMAP_PINDESC(_gp1, _mro1, _msv1, _io1), \
            PINMAP_NULLDESC, PINMAP_NULLDESC, PINMAP_NULLDESC, PINMAP_NULLDESC, \
            PINMAP_NULLDESC, PINMAP_NULLDESC \
        } \
    }

#define PINMAP_3PIN(_n, _gp0, _mro0, _msv0, _io0, _gp1, _mro1, _msv1, _io1, _gp2, _mro2, _msv2, _io2) \
    [_n] = { \
        .name = STRINGIFY(_n), \
        { \
            PINMAP_PINDESC(_gp0, _mro0, _msv0, _io0), \
            PINMAP_PINDESC(_gp1, _mro1, _msv1, _io1), \
            PINMAP_PINDESC(_gp2, _mro2, _msv2, _io2), \
            PINMAP_NULLDESC, PINMAP_NULLDESC, PINMAP_NULLDESC, PINMAP_NULLDESC, \
            PINMAP_NULLDESC \
        } \
    }

#define PINMAP_4PIN(_n, _gp0, _mro0, _msv0, _io0, _gp1, _mro1, _msv1, _io1, _gp2, _mro2, _msv2, _io2, _gp3, _mro3, _msv3, _io3) \
    [_n] = { \
        .name = STRINGIFY(_n), \
        { \
            PINMAP_PINDESC(_gp0, _mro0, _msv0, _io0), \
            PINMAP_PINDESC(_gp1, _mro1, _msv1, _io1), \
            PINMAP_PINDESC(_gp2, _mro2, _msv2, _io2), \
            PINMAP_PINDESC(_gp3, _mro3, _msv3, _io3), \
            PINMAP_NULLDESC, PINMAP_NULLDESC, PINMAP_NULLDESC, PINMAP_NULLDESC \
        } \
    }

typedef struct tk1_mux_feature_pinmap_ {
#ifdef CONFIG_DEBUG_BUILD
    const char  *name;
#endif
    struct tk1_mux_pin_desc {
        uint32_t    flags;
        int16_t     gpio_pin, mux_reg_index;
        uint8_t     mux_sfio_value;
    } pins[8];
} tk1_mux_feature_pinmap_t;

static inline const char *
get_feature_name_string(tk1_mux_feature_pinmap_t *var)
{
#ifdef CONFIG_DEBUG_BUILD
    return var->name;
#else
    return "<Unknown>";
#endif
}

/* Array of descriptors for each controller that we may want to enable.
 *
 * Each descriptor has a list of pins that the controller sends and receives
 * signals on. Each of the pins' required configuration parameters is also
 * given.
 *
 * Basically, to add support for a new mux feature, just fill out a new entry
 * in this list.
 */
tk1_mux_feature_pinmap_t    pinmaps[NMUX_FEATURES] = {
    /* UARTs are a 2 or 4 signal pinout: RTS, CTS, TX, RX. */
    PINMAP_2PIN(MUX_FEATURE_UARTA, GPIO_PS1, MUX_PAD_KB_ROW9_PS1, 3, P_BOTH, GPIO_PS2, MUX_PAD_KB_ROW10_PS2, 3, P_BOTH),
    PINMAP_4PIN(MUX_FEATURE_UARTB, GPIO_PC3, MUX_PAD_UART2_RXD_PC3, 0, P_BOTH,
        GPIO_PC2, MUX_PAD_UART2_TXD_PC2, 0, P_BOTH,
        GPIO_PJ6, MUX_PAD_UART2_RTS_N_PJ6, 1, P_BOTH,
        GPIO_PJ5, MUX_PAD_UART2_CTS_N_PJ5, 1, P_BOTH),
    PINMAP_4PIN(MUX_FEATURE_UARTC, GPIO_PW6, MUX_PAD_UART3_TXD_PW6, 0, P_BOTH,
        GPIO_PW7, MUX_PAD_UART3_RXD_PW7, 0, P_BOTH,
        GPIO_PA1, MUX_PAD_UART3_CTS_N_PA1, 0, P_BOTH,
        GPIO_PC0, MUX_PAD_UART3_RTS_N_PC0, 0, P_BOTH),
    PINMAP_4PIN(MUX_FEATURE_UARTD, GPIO_PJ7, MUX_PAD_PJ7, 0, P_BOTH,
        GPIO_PB0, MUX_PAD_PB0, 0, P_BOTH,
        GPIO_PB1, MUX_PAD_PB1, 0, P_BOTH,
        GPIO_PK7, MUX_PAD_PK7, 0, P_BOTH),

    /* SPI is a 4 signal pinout: CS, SCLK, MOSI, MISO.
     * SPI1 is a special case, though I don't recall why.
     */
    PINMAP_3PIN(MUX_FEATURE_SPI1,
        GPIO_PY0, MUX_PAD_ULPI_CLK_PY0, 0, P_BOTH,
        GPIO_PY1, MUX_PAD_ULPI_DIR_PY1, 0, P_BOTH,
        GPIO_PY2, MUX_PAD_ULPI_NXT_PY2, 0, P_BOTH),
    PINMAP_4PIN(MUX_FEATURE_SPI2, GPIO_PO5, MUX_PAD_ULPI_DATA4_PO5, 0, P_BOTH,
        GPIO_PO6, MUX_PAD_ULPI_DATA5_PO6, 0, P_BOTH,
        GPIO_PO7, MUX_PAD_ULPI_DATA6_PO7, 0, P_BOTH,
        GPIO_PO0, MUX_PAD_ULPI_DATA7_PO0, 0, P_BOTH),
    PINMAP_4PIN(MUX_FEATURE_SPI3, GPIO_PO1, MUX_PAD_ULPI_DATA0_PO1, 0, P_BOTH,
        GPIO_PO2, MUX_PAD_ULPI_DATA1_PO2, 0, P_BOTH,
        GPIO_PO3, MUX_PAD_ULPI_DATA2_PO3, 0, P_BOTH,
        GPIO_PO4, MUX_PAD_ULPI_DATA3_PO4, 0, P_BOTH),
    PINMAP_4PIN(MUX_FEATURE_SPI4, GPIO_PG4, MUX_PAD_PG4, 3, P_BOTH,
        GPIO_PG5, MUX_PAD_PG5, 3, P_BOTH,
        GPIO_PG6, MUX_PAD_PG6, 3, P_BOTH,
        GPIO_PG7, MUX_PAD_PG7, 3, P_BOTH),

    /* These are being mux settings to be used to configure the pins in GPIO
     * mode, which means that the SFIO function we select is irrelevant.
     *
     * Just enable both the input and output buffers, and select the SFIO
     * function that the firmware places the pin into by default upon #RESET.
     */
    PINMAP_1PIN(MUX_FEATURE_GPIO_PS3, GPIO_PS3, MUX_PAD_KB_ROW11_PS3, 0, P_BOTH),
    PINMAP_1PIN(MUX_FEATURE_GPIO_PS4, GPIO_PS4, MUX_PAD_KB_ROW12_PS4, 0, P_BOTH),
    PINMAP_1PIN(MUX_FEATURE_GPIO_PR0, GPIO_PR0, MUX_PAD_KB_ROW0_PR0, 0, P_BOTH),
    PINMAP_1PIN(MUX_FEATURE_GPIO_PR6, GPIO_PR6, MUX_PAD_KB_ROW6_PR6, 0, P_BOTH),

    PINMAP_1PIN(MUX_FEATURE_GPIO_PS5, GPIO_PS5, MUX_PAD_KB_ROW13_PS5, 0, P_BOTH),
    PINMAP_1PIN(MUX_FEATURE_GPIO_PT0, GPIO_PT0, MUX_PAD_KB_ROW16_PT0, 0, P_BOTH),
    PINMAP_1PIN(MUX_FEATURE_GPIO_PS6, GPIO_PS6, MUX_PAD_KB_ROW14_PS6, 0, P_BOTH),
    PINMAP_1PIN(MUX_FEATURE_GPIO_PS2, GPIO_PS2, MUX_PAD_KB_ROW10_PS2, 0, P_BOTH),

    PINMAP_1PIN(MUX_FEATURE_GPIO_PA3, GPIO_PA3, MUX_PAD_DAP2_SCLK_PA3, 0, P_BOTH),

    PINMAP_2PIN(MUX_FEATURE_I2C0, GPIO_PC4, MUX_PAD_GEN1_I2C_SCL_PC4, 0, P_IN | P_OPEN_DRAIN,
        GPIO_PC5, MUX_PAD_GEN1_I2C_SDA_PC5, 0, P_IN | P_OPEN_DRAIN),
    PINMAP_2PIN(MUX_FEATURE_I2C1, GPIO_PT5, MUX_PAD_GEN2_I2C_SCL_PT5, 0, P_IN | P_OPEN_DRAIN,
        GPIO_PT6, MUX_PAD_GEN2_I2C_SDA_PT6, 0, P_IN | P_OPEN_DRAIN),
    PINMAP_2PIN(MUX_FEATURE_I2C2, GPIO_PBB1, MUX_PAD_CAM_I2C_SCL_PBB1, 1, P_IN | P_OPEN_DRAIN,
        GPIO_PBB2, MUX_PAD_CAM_I2C_SDA_PBB2, 1, P_IN | P_OPEN_DRAIN),
    PINMAP_2PIN(MUX_FEATURE_I2C3, GPIO_PV4, MUX_PAD_DDC_SCL_PV4, 0, P_IN,
        GPIO_PV5, MUX_PAD_DDC_SDA_PV5, 0, P_IN),

    PINMAP_1PIN(MUX_FEATURE_GPIO_PC4, GPIO_PC4, MUX_PAD_GEN1_I2C_SCL_PC4, 0, P_BOTH),
    PINMAP_1PIN(MUX_FEATURE_GPIO_PC5, GPIO_PC5, MUX_PAD_GEN1_I2C_SDA_PC5, 0, P_BOTH),
    PINMAP_1PIN(MUX_FEATURE_GPIO_PBB1, GPIO_PBB1, MUX_PAD_GEN2_I2C_SCL_PT5, 0, P_BOTH),
    PINMAP_1PIN(MUX_FEATURE_GPIO_PBB2, GPIO_PBB2, MUX_PAD_GEN2_I2C_SDA_PT6, 0, P_BOTH),

    /* This is a configuration that overloads UARTB's RTS and CTS pins to allow
     * us to use them as GPIO outputs.
     *
     * I was using them so I could sample measure the IRQ incoming and ACK times
     * of the PPM IRQ in the quadcopter repo. This can be removed, but it also
     * doesn't harm anything by being here.
     */
    PINMAP_2PIN(MUX_FEATURE_PPM_MIRROR,
        GPIO_PJ6, MUX_PAD_UART2_RTS_N_PJ6, 1, P_PUSHPULL,
        GPIO_PJ5, MUX_PAD_UART2_CTS_N_PJ5, 1, P_PUSHPULL)
};

typedef struct tegra_mux_state {
    volatile uint32_t *pinmux_misc;
    volatile uint32_t *pinmux_aux;
    gpio_sys_t *gpio_sys;
} tegra_mux_state_t;

static const tegra_mux_state_t *
tk1_mux_get_priv(mux_sys_t *mux_sys)
{
    assert(mux_sys != NULL);
    return (const tegra_mux_state_t *)mux_sys->priv;
}

static volatile uint32_t *
tk1_mux_get_group_reg_handle_for_pin(mux_sys_t *ms, int mux_reg_index)
{
    volatile uint32_t   *ret;
    int                 group_offset;

    ret = tk1_mux_get_priv(ms)->pinmux_misc;
    assert(ret != NULL);
    group_offset = tk1_mux_get_group_offset_for_pin(mux_reg_index);
    assert(group_offset > 0);
    return &ret[group_offset / sizeof(uint32_t)];
}

static void
tk1_mux_set_drive_strength_for_pin(mux_sys_t *ms,
                                   int mux_reg_index,
                                   uint8_t drive_up_strength,
                                   uint8_t drive_down_strength)
{
    volatile uint32_t   *reg;
    int                 group_offset;
    uint32_t            drup_mask, drup_shift, drdown_mask, drdown_shift;

    reg = tk1_mux_get_group_reg_handle_for_pin(ms, mux_reg_index);
    group_offset = tk1_mux_get_group_offset_for_pin(mux_reg_index);
    if (group_offset < 0) {
        /* Return early if there is no group configuration register for this
         * pin.
         */
        return;
    }
    ZF_LOGD("Group off 0x%x: previous val: 0x%x.", group_offset, *reg);

    drup_shift = tk1_mux_get_bitinfo_for_group(group_offset, BITOFF_DRIVEUP);
    drup_mask = tk1_mux_get_bitinfo_for_group(group_offset, BITMASK_DRIVEUP);
    drdown_shift = tk1_mux_get_bitinfo_for_group(group_offset, BITOFF_DRIVEDOWN);
    drdown_mask = tk1_mux_get_bitinfo_for_group(group_offset, BITMASK_DRIVEDOWN);

    *reg &= ~(drup_mask << drup_shift);
    *reg &= ~(drdown_mask << drdown_shift);

    *reg |= (drive_up_strength & drup_mask) << drup_shift;
    *reg |= (drive_down_strength & drdown_mask) << drdown_shift;
    ZF_LOGD("Group off 0x%x: new val: 0x%x.", group_offset, *reg);
}

static int
tk1_mux_set_pin_params(mux_sys_t *mux, struct tk1_mux_pin_desc *desc,
                       enum mux_gpio_dir mux_gpio_dir)
{
    uint32_t regval, requiredval;
    const tegra_mux_state_t *s = tk1_mux_get_priv(mux);

    regval = s->pinmux_aux[desc->mux_reg_index] & 0xFF;

    /* There is a "lock" bit in each of the mux registers. This lock bit
     * prevents us from modifying the configuration of the pads.
     *
     * If the lock bit is UNSET, we can just reconfigure the pad.
     * If the lock bit is SET however, we can't reconfigure the pad.
     */
    requiredval = ((desc->mux_sfio_value & MUX_REG_SFIO_SELECT_MASK) << MUX_REG_SFIO_SELECT_SHIFT);

    /* Enable input buffer if P_IN */
    if (desc->flags & P_IN) {
        requiredval |= BIT(MUX_REG_ENABLE_SHIFT);
    }
    /* Set output driver to pushpull if P_PUSHPULL. */
    if (desc->flags & P_PUSHPULL) {
        if (desc->flags & P_OPEN_DRAIN) {
            ZF_LOGE("Can't enable both pushpull and open-drain output drivers.");
            return -1;
        }
        if (desc->flags & P_TRISTATE) {
            ZF_LOGE("Can't enable both pushpull and tristate output drivers.");
            return -1;
        }
        if (desc->flags & P_PULLUP || desc->flags & P_PULLDOWN) {
            ZF_LOGE("Can't enable both pushpull and pull-resistor output drivers.");
            return -1;
        }
        requiredval &= ~BIT(MUX_REG_TRISTATE_SHIFT);
    }
    if (desc->flags & P_TRISTATE) {
        if (desc->flags & P_OPEN_DRAIN) {
            ZF_LOGE("Can't enable both tristate and open-drain output drivers.");
            return -1;
        }
        if (desc->flags & P_PUSHPULL) {
            ZF_LOGE("Can't enable both tristate and pushpull output drivers.");
            return -1;
        }
        if (desc->flags & P_PULLUP || desc->flags & P_PULLDOWN) {
            ZF_LOGE("Can't enable both tristate and pull-resistor output drivers.");
            return -1;
        }
        requiredval |= BIT(MUX_REG_TRISTATE_TRISTATE);
    }
    if (desc->flags & P_PULLUP || desc->flags & P_PULLDOWN) {
        if ((desc->flags & P_PULLUP) && (desc->flags & P_PULLDOWN)) {
            ZF_LOGE("Can't enable both pullup and pulldown at once.");
            return -1;
        }
        if (desc->flags & P_OPEN_DRAIN) {
            ZF_LOGE("Can't enable both pullup and opendrain output drivers.");
            return -1;
        }
        if (desc->flags & P_TRISTATE) {
            ZF_LOGE("Can't enable both pullup and tristate output drivers.");
            return -1;
        }
        requiredval &= ~(MUX_REG_PUPD_MASK << MUX_REG_PUPD_SHIFT);
        requiredval |= ((desc->flags & P_PULLUP)
                        ? MUX_REG_PUPD_PULLUP
                        : MUX_REG_PUPD_PULLDOWN)
                            << MUX_REG_PUPD_SHIFT;
    }
    if (desc->flags & P_OPEN_DRAIN) {
        if (desc->flags & P_PUSHPULL) {
            ZF_LOGE("Can't enable both open-drain and pushpull output drivers.");
            return -1;
        }
        if (desc->flags & P_TRISTATE) {
            ZF_LOGE("Can't enable both open-drain and tristate output drivers.");
            return -1;
        }
        if (desc->flags & P_PULLUP || desc->flags & P_PULLDOWN) {
            ZF_LOGE("Can't enable both open-drain and pullup/pulldown output drivers.");
            return -1;
        }
        requiredval |= BIT(MUX_REG_OPEN_DRAIN_SHIFT);
    }

    if (regval == requiredval) {
        return 0;
    }

    if (regval & BIT(MUX_REG_LOCK_SHIFT)) {
        /* IF the lock bit is set, we can't change it */
        ZF_LOGE("Failed to change pin SFIO function to %d: Bit is locked.",
                desc->mux_sfio_value);
        return -1;
    }

    s->pinmux_aux[desc->mux_reg_index] = requiredval;
    /* Just in case the writes are being silently ignored. */
    assert(s->pinmux_aux[desc->mux_reg_index] == requiredval);
    return 0;
}

static bool
pin_is_pull_up_by_default(int mux_reg_index)
{
    /* This is a list of the pins whose default state on #RESET is Pull-up. */
    const uint16_t pull_up_pin_indexes[] = {
        MUX_PAD_ULPI_DATA0_PO1, MUX_PAD_ULPI_DATA1_PO2, MUX_PAD_ULPI_DATA2_PO3,
        MUX_PAD_ULPI_DATA3_PO4, MUX_PAD_ULPI_DATA4_PO5, MUX_PAD_ULPI_DATA5_PO6,
        MUX_PAD_ULPI_DATA6_PO7, MUX_PAD_ULPI_DATA7_PO0, MUX_PAD_SDMMC1_CMD_PZ1,
        MUX_PAD_SDMMC1_DAT3_PY4, MUX_PAD_SDMMC1_DAT2_PY5, MUX_PAD_SDMMC1_DAT1_PY6,
        MUX_PAD_SDMMC1_DAT0_PY7,
        MUX_PAD_UART2_RXD_PC3, MUX_PAD_UART2_TXD_PC2, MUX_PAD_UART2_RTS_N_PJ6,
        MUX_PAD_UART2_CTS_N_PJ5, MUX_PAD_UART3_TXD_PW6, MUX_PAD_UART3_RXD_PW7,
        MUX_PAD_UART3_RTS_N_PC0, MUX_PAD_UART3_RTS_N_PC0,
        MUX_PAD_PC7, MUX_PAD_PI5, MUX_PAD_PI7, MUX_PAD_PK0, MUX_PAD_PJ0,
        MUX_PAD_PJ2, MUX_PAD_PK3, MUX_PAD_PK4, MUX_PAD_PK2, MUX_PAD_PI3,
        MUX_PAD_PI6, MUX_PAD_PH4, MUX_PAD_PH6, MUX_PAD_PH7, MUX_PAD_PI0,
        MUX_PAD_PI1, MUX_PAD_PI2, MUX_PAD_SDMMC4_CMD_PT7, MUX_PAD_SDMMC4_DAT0_PAA0,
        MUX_PAD_SDMMC4_DAT1_PAA1, MUX_PAD_SDMMC4_DAT2_PAA2, MUX_PAD_SDMMC4_DAT3_PAA3,
        MUX_PAD_SDMMC4_DAT4_PAA4, MUX_PAD_SDMMC4_DAT5_PAA5, MUX_PAD_SDMMC4_DAT6_PAA6,
        MUX_PAD_SDMMC4_DAT7_PAA7,
        MUX_PAD_CAM_MCLK_PCC0,
        MUX_PAD_PCC1, MUX_PAD_PCC2, MUX_PAD_JTAG_RTCK,
        MUX_PAD_KB_COL0_PQ0, MUX_PAD_KB_COL1_PQ1, MUX_PAD_KB_COL2_PQ2,
        MUX_PAD_KB_COL3_PQ3, MUX_PAD_KB_COL4_PQ4, MUX_PAD_KB_COL5_PQ5,
        MUX_PAD_KB_COL6_PQ6, MUX_PAD_KB_COL7_PQ7,
        MUX_PAD_SPDIF_OUT_PK5,
        MUX_PAD_GPIO_X3_AUD_PX3,
        MUX_PAD_DVFS_CLK_PX2,
        MUX_PAD_GPIO_X5_AUD_PX5, MUX_PAD_GPIO_X6_AUD_PX6,
        MUX_PAD_SDMMC1_CMD_PZ1, MUX_PAD_SDMMC3_DAT0_PB7, MUX_PAD_SDMMC3_DAT1_PB6,
        MUX_PAD_SDMMC3_DAT2_PB5, MUX_PAD_SDMMC3_DAT3_PB4,
        MUX_PAD_SDMMC1_WP_N_PV3, MUX_PAD_SDMMC3_CD_N_PV2,
        MUX_PAD_GPIO_W2_AUD_PW2, MUX_PAD_GPIO_W3_AUD_PW3
    };

    for (int i = 0; i < ARRAY_SIZE(pull_up_pin_indexes); i++) {
        /* Each register is 32 bits, so indexes need to be multiplied by 4 to
         * get the byte-offset from the base.
         */
        if (pull_up_pin_indexes[i] == mux_reg_index * 4) {
            return true;
        }
    }
    return false;
}

static void
tk1_mux_set_pin_unused(volatile uint32_t *regs, uint16_t mux_reg_index,
                       bool enable_input_buffer)
{
    int pupd_val;

    /* TK1 TRM, sec 8.10.3 "Unused Pins":
     *  "For each unused MPIO, assert its tristate and disable its input "
     *  buffer. For pins whose internal pull-up is enabled during power-
     *  on-reset, assert the internal pull-up. Otherwise, assert the internal
     *  pull-down."
     *
     * Furthermore:
     *
     * TK1 TRM, sec 8.4.1 "Per Pad Options":
     *  "Tristate (high-z) option: Disables or enables the pad’s output driver.
     *  This setting overrides any other functional setting and also whether pad
     *  is selected for SFIO or GPIO. Can be used when the pad direction changes
     *  or the pad is assigned to different SFIO to avoid glitches."
     *
     * So this function will set a given pin to its unused state by enabling
     * its tristate mode (which is its output mode), and then either asserting
     * pull-up or pull-down.
     *
     * This function will be used both to disable pins that are not being used,
     * and also as an intermediate transition when changing the mux settings
     * for a pin (as advised by the manual, in sec 8.4.1).
     */

    /* See sec 8.10.3 above: if the pin is pull up on #RESET, then assert
     * pull-up, else assert pull-down.
     */
    if (pin_is_pull_up_by_default(mux_reg_index)) {
        pupd_val = MUX_REG_PUPD_PULLUP;
    } else {
        pupd_val = MUX_REG_PUPD_PULLDOWN;
    }

    regs[mux_reg_index] = 0
                       /* Tristate on */
                       | MUX_REG_TRISTATE_TRISTATE << MUX_REG_TRISTATE_SHIFT
                       /* Enable/disable the input buffer.
                        *
                        * We should disable it, but the problem is that if we
                        * disable the input buffer on the pin, and then later
                        * on somebody wants to use the pin as an GPIO input pin,
                        * it would mean that the GPIO driver would have to call
                        * down into the mux driver (this driver).
                        *
                        * But this driver already depends on the GPIO driver, so
                        * that would create a circular dependency.
                        *
                        * Thankfully, leaving the input buffer on shouldn't be
                        * detrimental because, according to the TRM sec 8.4.1,
                        * the tristate option overrides all other options.
                        */
                       | (!!enable_input_buffer) << MUX_REG_ENABLE_SHIFT
                       /* Assert either pull-up or pull-down. */
                       | pupd_val << MUX_REG_PUPD_SHIFT;
}

static int
tk1_mux_feature_enable(mux_sys_t* mux, enum mux_feature feat,
                       enum mux_gpio_dir mux_gpio_dir)
{
    int error;
    tk1_mux_feature_pinmap_t *map;
    const tegra_mux_state_t *s = tk1_mux_get_priv(mux);

    assert(feat < NMUX_FEATURES);

    map = &pinmaps[feat];
    for (int i = 0; i < ARRAY_SIZE(map->pins); i++) {
        /* Pin list is terminated by -1 as pin number. */
        if (map->pins[i].gpio_pin == -1) {
            break;
        }

        ZF_LOGD("Feature enable: feat %d, Mux pad index 0x%x(offset 0x%x), GPIO %d. Reg state before 0x%x.",
                feat,
                map->pins[i].mux_reg_index,
                map->pins[i].mux_reg_index * sizeof(uint32_t),
                map->pins[i].gpio_pin,
                s->pinmux_aux[map->pins[i].mux_reg_index]);

        if (mux_gpio_dir == MUX_DIR_NOT_A_GPIO) {
            /* According to TK1 TRM sec 8.4.1, we should set the pin to default
             * values (asserting tristate) when we're reconfiguring it to a
             * different SFIO, to prevent glitches during the configuration
             * transition.
             *
             * If the SFIO function being changed to is the same though,
             * we shouldn't call set_pin_unused() because that has to potential
             * to unnecessarily pull the pin up or down according to its
             * default #RESET voltage level.
             */
            if ((s->pinmux_aux[map->pins[i].mux_reg_index]
                & MUX_REG_SFIO_SELECT_MASK) != map->pins[i].mux_sfio_value) {
                tk1_mux_set_pin_unused(s->pinmux_aux,
                                       map->pins[i].mux_reg_index, true);
            }

            /* First, attempt to set the pin into SFIO mode IF it's going to be
             * used to bring out an SFIO signal.
             *
             * If it'll be used to bring out a GPIO signal, leave it alone in
             * whatever mode it was in, and let the GPIO driver set it up when
             * the user tells it to.
             */
            error = gpio_set_pad_mode(s->gpio_sys, map->pins[i].gpio_pin,
                                      SFIO_MODE,
                                      0 /* Doesn't matter for SFIO mode. */);
            if (error) {
                ZF_LOGE("Failed to set pin %d for feature %s. Aborting.",
                        i, get_feature_name_string(map));
                return -1;
            }
        }

        /* Next set up the rest of the parameters. */
        error = tk1_mux_set_pin_params(mux, &map->pins[i], mux_gpio_dir);
        if (error) {
            ZF_LOGE("Failed to pinmux params for pin %d of feature %s. "
                    "Aborting.",
                    i, get_feature_name_string(map));
            return -1;
        }

        /* Set the drive strength divisor to 0 for max drive strength. */
        tk1_mux_set_drive_strength_for_pin(mux,
                                           map->pins[i].mux_reg_index,
                                           0, 0);
        ZF_LOGD("Feature enable: feat %d, Mux pad index 0x%x(offset 0x%x), GPIO %d. Reg state AFTER 0x%x.",
                feat,
                map->pins[i].mux_reg_index,
                map->pins[i].mux_reg_index * sizeof(uint32_t),
                map->pins[i].gpio_pin,
                s->pinmux_aux[map->pins[i].mux_reg_index]);
    }

    return 0;
}

static int
tk1_mux_feature_disable(mux_sys_t* mux, enum mux_feature feat)
{
    tk1_mux_feature_pinmap_t *map;
    const tegra_mux_state_t *s = tk1_mux_get_priv(mux);

    assert(feat < NMUX_FEATURES);

    map = &pinmaps[feat];
    for (int i = 0; i < ARRAY_SIZE(map->pins); i++) {
        /* Pin list is terminated by -1 as pin number. */
        if (map->pins[i].gpio_pin == -1) {
            break;
        }

        tk1_mux_set_pin_unused(s->pinmux_aux, map->pins[i].mux_reg_index, false);
    }

    return 0;
}

static void
tk1_mux_set_all_pins_to_default_state(volatile uint32_t *mux_regs)
{
    /* TODO:
     * We should cycle through all valid pins here and call
     * tk1_mux_set_pin_unused() on each one.
     */
}

int
tegra_mux_init(volatile void *pinmux_misc, volatile void *pinmux_aux,
               ps_io_ops_t *io_ops,
               gpio_sys_t *gpio_sys, mux_sys_t *self)
{
    int error;
    tegra_mux_state_t *state;

    if (!gpio_sys_valid(gpio_sys)) {
        ZF_LOGE("Invalid GPIO driver instance handle.");
        return -EINVAL;
    };

    error = ps_malloc(&io_ops->malloc_ops, sizeof(*state), (void **)&state);
    if (error != 0 || state == NULL) {
        ZF_LOGE("Failed to alloc TK1 Mux instance internal state.");
        return -1;
    }

    state->pinmux_misc = pinmux_misc;
    state->pinmux_aux = pinmux_aux;
    state->gpio_sys = gpio_sys;

    self->priv = state;
    self->feature_enable = &tk1_mux_feature_enable;
    self->feature_disable = &tk1_mux_feature_disable;

    tk1_mux_set_all_pins_to_default_state(tk1_mux_get_priv(self)->pinmux_aux);

    ZF_LOGE("Mux misc for dbgcfg @vaddrs 0x%p, 0x%p: values 0x%x, 0x%x (offsets are 0x%x, 0x%x).",
        tk1_mux_get_group_reg_handle_for_pin(self, MUX_PAD_GEN1_I2C_SCL_PC4),
        tk1_mux_get_group_reg_handle_for_pin(self, MUX_PAD_CAM_I2C_SCL_PBB1),
        *tk1_mux_get_group_reg_handle_for_pin(self, MUX_PAD_GEN1_I2C_SCL_PC4),
        *tk1_mux_get_group_reg_handle_for_pin(self, MUX_PAD_CAM_I2C_SCL_PBB1),
        tk1_mux_get_group_offset_for_pin(MUX_PAD_GEN1_I2C_SCL_PC4),
        tk1_mux_get_group_offset_for_pin(MUX_PAD_CAM_I2C_SCL_PBB1));
    return 0;
}

int
mux_sys_init(ps_io_ops_t *io_ops, void *dependencies, mux_sys_t *mux)
{
    void *pinmux_misc_vaddr = NULL, *pinmux_aux_vaddr = NULL;
    gpio_sys_t *gpio_sys = (gpio_sys_t *)dependencies;

    pinmux_misc_vaddr = RESOURCE(io_ops, TK1_MUX_MISC);
    if (pinmux_misc_vaddr == NULL) {
        ZF_LOGE("Failed to map in pinmux_misc frame.");
        return -1;
    }
    pinmux_aux_vaddr = RESOURCE(io_ops, TK1_MUX_AUX);
    if (pinmux_aux_vaddr == NULL) {
        ZF_LOGE("Failed to map in pinmux_aux frame.");
        return -1;
    }

    return tegra_mux_init(pinmux_misc_vaddr, pinmux_aux_vaddr, io_ops,
                          gpio_sys, mux);
}
