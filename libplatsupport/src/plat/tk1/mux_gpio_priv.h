/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <platsupport/gpio.h>
#include <platsupport/plat/gpio.h>

/** Sets a pin to either GPIO or SFIO (special function I/O) mode.
 *
 * Normally this functionality is provided by a Mux driver and not by a GPIO
 * driver, but on the TK1, the GPIO controller actually controls this
 * functionality. So the Mux driver calls on this function.
 */
int
gpio_set_pad_mode(gpio_sys_t *gpio_sys,
                  enum gpio_pin gpio, enum gpio_pad_mode mode, enum gpio_dir dir);

/* These #defines here are register offsets within the pinmux_misc MMIO frame,
 * for the pingroups.
 *
 * The pingroups are basically hardware-defined groupings of the pins, and
 * setting attributes on these pingroups will affect all the pins which are part
 * the targeted group.
 */
#define TK1MUX_GROUP_OFFSET_GMACFG                     (0x900)
#define TK1MUX_GROUP_OFFSET_SDIO1CFG                   (0x8ec)
#define TK1MUX_GROUP_OFFSET_SDIO3CFG                   (0x8b0)
#define TK1MUX_GROUP_OFFSET_SDIO4CFG                   (0x9c4)
#define TK1MUX_GROUP_OFFSET_AOCFG0                     (0x9b0)
#define TK1MUX_GROUP_OFFSET_AOCFG1                     (0x868)
#define TK1MUX_GROUP_OFFSET_AOCFG2                     (0x86c)
#define TK1MUX_GROUP_OFFSET_AOCFG3                     (0x9a8)
#define TK1MUX_GROUP_OFFSET_AOCFG4                     (0x9c8)
#define TK1MUX_GROUP_OFFSET_CDEV1CFG                   (0x884)
#define TK1MUX_GROUP_OFFSET_CDEV2CFG                   (0x888)
#define TK1MUX_GROUP_OFFSET_CECCFG                     (0x938)
#define TK1MUX_GROUP_OFFSET_DAP1CFG                    (0x890)
#define TK1MUX_GROUP_OFFSET_DAP2CFG                    (0x894)
#define TK1MUX_GROUP_OFFSET_DAP3CFG                    (0x898)
#define TK1MUX_GROUP_OFFSET_DAP4CFG                    (0x89c)
#define TK1MUX_GROUP_OFFSET_DAP5CFG                    (0x998)
#define TK1MUX_GROUP_OFFSET_DBGCFG                     (0x8a0)
#define TK1MUX_GROUP_OFFSET_DDCCFG                     (0x8fc)
#define TK1MUX_GROUP_OFFSET_DEV3CFG                    (0x92c)
#define TK1MUX_GROUP_OFFSET_OWRCFG                     (0x920)
#define TK1MUX_GROUP_OFFSET_SPICFG                     (0x8b4)
#define TK1MUX_GROUP_OFFSET_UAACFG                     (0x8b8)
#define TK1MUX_GROUP_OFFSET_UABCFG                     (0x8bc)
#define TK1MUX_GROUP_OFFSET_UART2CFG                   (0x8c0)
#define TK1MUX_GROUP_OFFSET_UART3CFG                   (0x8c4)
#define TK1MUX_GROUP_OFFSET_UDACFG                     (0x924)
#define TK1MUX_GROUP_OFFSET_ATCFG1                     (0x870)
#define TK1MUX_GROUP_OFFSET_ATCFG2                     (0x874)
#define TK1MUX_GROUP_OFFSET_ATCFG3                     (0x878)
#define TK1MUX_GROUP_OFFSET_ATCFG4                     (0x87c)
#define TK1MUX_GROUP_OFFSET_ATCFG5                     (0x880)
#define TK1MUX_GROUP_OFFSET_ATCFG6                     (0x994)
#define TK1MUX_GROUP_OFFSET_GMECFG                     (0x910)
#define TK1MUX_GROUP_OFFSET_GMFCFG                     (0x914)
#define TK1MUX_GROUP_OFFSET_GMGCFG                     (0x918)
#define TK1MUX_GROUP_OFFSET_GMHCFG                     (0x91c)
#define TK1MUX_GROUP_OFFSET_HVCFG0                     (0x9b4)
#define TK1MUX_GROUP_OFFSET_GPVCFG                     (0x928)
#define TK1MUX_GROUP_OFFSET_USB_VBUS_EN_CFG            (0x99c)

/* These #defines are shorthand for the above ones so we don't have to use their
 * long names.
 */
#define GMACFG TK1MUX_GROUP_OFFSET_GMACFG
#define SDIO1CFG TK1MUX_GROUP_OFFSET_SDIO1CFG
#define SDIO3CFG TK1MUX_GROUP_OFFSET_SDIO3CFG
#define SDIO4CFG TK1MUX_GROUP_OFFSET_SDIO4CFG
#define AOCFG0 TK1MUX_GROUP_OFFSET_AOCFG0
#define AOCFG1 TK1MUX_GROUP_OFFSET_AOCFG1
#define AOCFG2 TK1MUX_GROUP_OFFSET_AOCFG2
#define AOCFG3 TK1MUX_GROUP_OFFSET_AOCFG3
#define AOCFG4 TK1MUX_GROUP_OFFSET_AOCFG4
#define CDEV1CFG TK1MUX_GROUP_OFFSET_CDEV1CFG
#define CDEV2CFG TK1MUX_GROUP_OFFSET_CDEV2CFG
#define CECCFG TK1MUX_GROUP_OFFSET_CECCFG
#define DAP1CFG TK1MUX_GROUP_OFFSET_DAP1CFG
#define DAP2CFG TK1MUX_GROUP_OFFSET_DAP2CFG
#define DAP3CFG TK1MUX_GROUP_OFFSET_DAP3CFG
#define DAP4CFG TK1MUX_GROUP_OFFSET_DAP4CFG
#define DAP5CFG TK1MUX_GROUP_OFFSET_DAP5CFG
#define DBGCFG TK1MUX_GROUP_OFFSET_DBGCFG
#define DDCCFG TK1MUX_GROUP_OFFSET_DDCCFG
#define DEV3CFG TK1MUX_GROUP_OFFSET_DEV3CFG
#define OWRCFG TK1MUX_GROUP_OFFSET_OWRCFG
#define SPICFG TK1MUX_GROUP_OFFSET_SPICFG
#define UAACFG TK1MUX_GROUP_OFFSET_UAACFG
#define UABCFG TK1MUX_GROUP_OFFSET_UABCFG
#define UART2CFG TK1MUX_GROUP_OFFSET_UART2CFG
#define UART3CFG TK1MUX_GROUP_OFFSET_UART3CFG
#define UDACFG TK1MUX_GROUP_OFFSET_UDACFG
#define ATCFG1 TK1MUX_GROUP_OFFSET_ATCFG1
#define ATCFG2 TK1MUX_GROUP_OFFSET_ATCFG2
#define ATCFG3 TK1MUX_GROUP_OFFSET_ATCFG3
#define ATCFG4 TK1MUX_GROUP_OFFSET_ATCFG4
#define ATCFG5 TK1MUX_GROUP_OFFSET_ATCFG5
#define ATCFG6 TK1MUX_GROUP_OFFSET_ATCFG6
#define GMECFG TK1MUX_GROUP_OFFSET_GMECFG
#define GMFCFG TK1MUX_GROUP_OFFSET_GMFCFG
#define GMGCFG TK1MUX_GROUP_OFFSET_GMGCFG
#define GMHCFG  TK1MUX_GROUP_OFFSET_GMHCFG
#define HVCFG0 TK1MUX_GROUP_OFFSET_HVCFG0
#define GPVCFG TK1MUX_GROUP_OFFSET_GPVCFG
#define USB_VBUS_EN_CFG TK1MUX_GROUP_OFFSET_USB_VBUS_EN_CFG

typedef struct mux_pin_group_mapping_ {
    uint16_t    mux_reg_index, group_offset;
} mux_pin_group_mapping_t;

enum mux_group_bitoffset {
    BITOFF_DRIVEUP,
    BITOFF_DRIVEDOWN,
    BITMASK_DRIVEUP,
    BITMASK_DRIVEDOWN
};

typedef struct mux_group_bitoff_mapping_ {
    uint16_t    group_offset;
    uint8_t     driveup_bitoff, drivedown_bitoff,
                driveup_mask, drivedown_mask;
} mux_group_bitoff_mapping_t;

extern const mux_group_bitoff_mapping_t mux_group_bitinfo_mapping_table[];
extern const mux_pin_group_mapping_t mux_pin_group_mapping_table[];

int array_size_pin_group_mapping_table(void);

/** Lookup the group configuration offset for a pin.
 * @param[in] mux_reg_index     Index of a mux pin. See tk1/mux.h
 * @return  -1 on error
 *          positive integer offset on success.
 */
static inline int tk1_mux_get_group_offset_for_pin(int mux_reg_index)
{
    for (int i = 0; i < array_size_pin_group_mapping_table(); i++) {
        if (mux_pin_group_mapping_table[i].mux_reg_index == mux_reg_index) {
            if (mux_pin_group_mapping_table[i].group_offset == 0) {
                /* If the pin has no group configuration reg, return error. */
                return -1;
            }

            return mux_pin_group_mapping_table[i].group_offset;
        }
    }
    return -1;
}

int array_size_bitinfo_mapping_table(void);

static inline int tk1_mux_get_bitinfo_for_group(int group_offset, int desired_bitinfo)
{
    for (int i = 0; i < array_size_bitinfo_mapping_table(); i++) {
        if (mux_group_bitinfo_mapping_table[i].group_offset == group_offset) {
            switch (desired_bitinfo) {
            case BITMASK_DRIVEDOWN:
                return mux_group_bitinfo_mapping_table[i].drivedown_mask;
            case BITMASK_DRIVEUP:
                return mux_group_bitinfo_mapping_table[i].driveup_mask;
            case BITOFF_DRIVEDOWN:
                return mux_group_bitinfo_mapping_table[i].drivedown_bitoff;
            case BITOFF_DRIVEUP:
                return mux_group_bitinfo_mapping_table[i].driveup_bitoff;
            default:
                ZF_LOGE("Invalid bitinfo %d requested for groupoffset %d.",
                        desired_bitinfo, group_offset);
                return -1;
            }
        }
    }
    return -1;
}
