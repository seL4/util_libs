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
#pragma once

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

/* This table has the bitshift and mask values for the pingroup drive-strength
 * bits. Each group has potentially different shifts and masks, so it's easier
 * to just use a table.
 */
const static mux_group_bitoff_mapping_t mux_group_bitinfo_mapping_table[] = {
    { TK1MUX_GROUP_OFFSET_GMACFG, 14, 20, 0x1F, 0x1F },
    { TK1MUX_GROUP_OFFSET_SDIO1CFG, 12, 20, 0x7F, 0x7F },
    { TK1MUX_GROUP_OFFSET_SDIO3CFG, 12, 20, 0x7F, 0x7F },
    { TK1MUX_GROUP_OFFSET_SDIO4CFG, 12, 20, 0x1F, 0x1F },
    { TK1MUX_GROUP_OFFSET_AOCFG0, 12, 20, 0x1F, 0x1F },
    { TK1MUX_GROUP_OFFSET_AOCFG1, 12, 20, 0x1F, 0x1F },
    { TK1MUX_GROUP_OFFSET_AOCFG2, 12, 20, 0x1F, 0x1F },
    { TK1MUX_GROUP_OFFSET_AOCFG3, 12, 0, 0x1F, 0 },
    { TK1MUX_GROUP_OFFSET_AOCFG4, 12, 20, 0x7F, 0x7F },
    { TK1MUX_GROUP_OFFSET_CDEV1CFG, 12, 20, 0x1F, 0x1F },
    { TK1MUX_GROUP_OFFSET_CDEV2CFG, 12, 20, 0x1F, 0x1F },
    { TK1MUX_GROUP_OFFSET_CECCFG, 12, 20, 0x1F, 0x1F },
    { TK1MUX_GROUP_OFFSET_DAP1CFG, 12, 20, 0x1F, 0x1F },
    { TK1MUX_GROUP_OFFSET_DAP2CFG, 12, 20, 0x1F, 0x1F },
    { TK1MUX_GROUP_OFFSET_DAP3CFG, 12, 20, 0x1F, 0x1F },
    { TK1MUX_GROUP_OFFSET_DAP4CFG, 12, 20, 0x1F, 0x1F },
    { TK1MUX_GROUP_OFFSET_DAP5CFG, 12, 20, 0x1F, 0x1F },
    { TK1MUX_GROUP_OFFSET_DBGCFG, 12, 20, 0x1F, 0x1F },
    { TK1MUX_GROUP_OFFSET_DDCCFG, 12, 20, 0x1F, 0x1F },
    { TK1MUX_GROUP_OFFSET_DEV3CFG, 12, 20, 0x1F, 0x1F },
    { TK1MUX_GROUP_OFFSET_OWRCFG, 12, 20, 0x1F, 0x1F },
    { TK1MUX_GROUP_OFFSET_SPICFG, 12, 20, 0x1F, 0x1F },
    { TK1MUX_GROUP_OFFSET_UAACFG, 12, 20, 0x1F, 0x1F },
    { TK1MUX_GROUP_OFFSET_UABCFG, 12, 20, 0x1F, 0x1F },
    { TK1MUX_GROUP_OFFSET_UART2CFG, 12, 20, 0x1F, 0x1F },
    { TK1MUX_GROUP_OFFSET_UART3CFG, 12, 20, 0x1F, 0x1F },
    { TK1MUX_GROUP_OFFSET_UDACFG, 12, 20, 0x1F, 0x1F },
    { TK1MUX_GROUP_OFFSET_ATCFG1, 12, 20, 0x7F, 0x7F },
    { TK1MUX_GROUP_OFFSET_ATCFG2, 12, 20, 0x7F, 0x7F },
    { TK1MUX_GROUP_OFFSET_ATCFG3, 12, 20, 0x7F, 0x7F },
    { TK1MUX_GROUP_OFFSET_ATCFG4, 12, 20, 0x7F, 0x7F },
    { TK1MUX_GROUP_OFFSET_ATCFG5, 14, 19, 0x1F, 0x1F },
    { TK1MUX_GROUP_OFFSET_ATCFG6, 12, 20, 0x7F, 0x7F },
    { TK1MUX_GROUP_OFFSET_GMECFG, 14, 19, 0x1F, 0x1F },
    { TK1MUX_GROUP_OFFSET_GMFCFG, 14, 19, 0x1F, 0x1F },
    { TK1MUX_GROUP_OFFSET_GMGCFG, 14, 19, 0x1F, 0x1F },
    { TK1MUX_GROUP_OFFSET_GMHCFG, 14, 19, 0x1F, 0x1F },
    { TK1MUX_GROUP_OFFSET_HVCFG0, 12, 0, 0x1F, 0 },
    { TK1MUX_GROUP_OFFSET_GPVCFG, 21, 20, 0x1F, 0x1F },
    { TK1MUX_GROUP_OFFSET_USB_VBUS_EN_CFG, 12, 20, 0x1F, 0x1F }
};

/* This table maps each mux pin to its group control register. Each pin is part
 * of a group, such that you can configure these groups with common settings.
 *
 * Later on, this table might be updated with wakeup information and pin type
 * information as well.
 */
const static mux_pin_group_mapping_t mux_pin_group_mapping_table[] = {
    { MUX_PAD_ULPI_DATA0_PO1, UAACFG },
    { MUX_PAD_ULPI_DATA1_PO2, UAACFG },
    { MUX_PAD_ULPI_DATA2_PO3, UAACFG },
    { MUX_PAD_ULPI_DATA3_PO4, UAACFG },
    { MUX_PAD_ULPI_DATA4_PO5, UABCFG },
    { MUX_PAD_ULPI_DATA5_PO6, UABCFG },
    { MUX_PAD_ULPI_DATA6_PO7, UABCFG },
    { MUX_PAD_ULPI_DATA7_PO0, UABCFG },
    { MUX_PAD_ULPI_CLK_PY0, UDACFG },
    { MUX_PAD_ULPI_DIR_PY1, UDACFG },
    { MUX_PAD_ULPI_NXT_PY2, UDACFG },
    { MUX_PAD_ULPI_STP_PY3, UDACFG },
    { MUX_PAD_DAP3_FS_PP0, DAP3CFG },
    { MUX_PAD_DAP3_DIN_PP1, DAP3CFG },
    { MUX_PAD_DAP3_DOUT_PP2, DAP3CFG },
    { MUX_PAD_DAP3_SCLK_PP3, DAP3CFG },
    { MUX_PAD_PV0, UABCFG },
    { MUX_PAD_PV1, UABCFG },
    { MUX_PAD_SDMMC1_CLK_PZ0, SDIO1CFG },
    { MUX_PAD_SDMMC1_CMD_PZ1, SDIO1CFG },
    { MUX_PAD_SDMMC1_DAT3_PY4, SDIO1CFG },
    { MUX_PAD_SDMMC1_DAT2_PY5, SDIO1CFG },
    { MUX_PAD_SDMMC1_DAT1_PY6, SDIO1CFG },
    { MUX_PAD_SDMMC1_DAT0_PY7, SDIO1CFG },
    { MUX_PAD_CLK2_OUT_PW5, CDEV2CFG },
    { MUX_PAD_CLK2_REQ_PCC5, CDEV2CFG },
    { MUX_PAD_HDMI_INT_PN7, HVCFG0 },
    { MUX_PAD_DDC_SCL_PV4, DDCCFG },
    { MUX_PAD_DDC_SDA_PV5, DDCCFG },
    { MUX_PAD_UART2_RXD_PC3, UART2CFG },
    { MUX_PAD_UART2_TXD_PC2, UART2CFG },
    { MUX_PAD_UART2_RTS_N_PJ6, UART2CFG },
    { MUX_PAD_UART2_CTS_N_PJ5, UART2CFG },
    { MUX_PAD_UART3_TXD_PW6, UART3CFG },
    { MUX_PAD_UART3_RXD_PW7, UART3CFG },
    { MUX_PAD_UART3_CTS_N_PA1, UART3CFG },
    { MUX_PAD_UART3_RTS_N_PC0, UART3CFG },
    { MUX_PAD_PU0, DBGCFG },
    { MUX_PAD_PU1, DBGCFG },
    { MUX_PAD_PU2, DBGCFG },
    { MUX_PAD_PU3, DBGCFG },
    { MUX_PAD_PU4, DBGCFG },
    { MUX_PAD_PU5, DBGCFG },
    { MUX_PAD_PU6, DBGCFG },
    { MUX_PAD_GEN1_I2C_SDA_PC5, TK1MUX_GROUP_OFFSET_DBGCFG },
    { MUX_PAD_GEN1_I2C_SCL_PC4, TK1MUX_GROUP_OFFSET_DBGCFG },
    { MUX_PAD_DAP4_FS_PP4, DAP4CFG },
    { MUX_PAD_DAP4_DIN_PP5, DAP4CFG },
    { MUX_PAD_DAP4_DOUT_PP6, DAP4CFG },
    { MUX_PAD_DAP4_SCLK_PP7, DAP4CFG },
    { MUX_PAD_CLK3_OUT_PEE0, DEV3CFG },
    { MUX_PAD_CLK3_REQ_PEE1, DEV3CFG },
    { MUX_PAD_PC7, ATCFG3 },
    { MUX_PAD_PI5, ATCFG6 },
    { MUX_PAD_PI7, ATCFG2 },
    { MUX_PAD_PK0, ATCFG2 },
    { MUX_PAD_PK1, ATCFG6 },
    { MUX_PAD_PJ0, ATCFG3 },
    { MUX_PAD_PJ2, ATCFG4 },
    { MUX_PAD_PK3, ATCFG6 },
    { MUX_PAD_PK4, ATCFG6 },
    { MUX_PAD_PK2, ATCFG2 },
    { MUX_PAD_PI3, ATCFG2 },
    { MUX_PAD_PI6, ATCFG6 },
    { MUX_PAD_PG0, ATCFG2 },
    { MUX_PAD_PG1, ATCFG2 },
    { MUX_PAD_PG2, ATCFG2 },
    { MUX_PAD_PG3, ATCFG2 },
    { MUX_PAD_PG4, ATCFG2 },
    { MUX_PAD_PG5, ATCFG2 },
    { MUX_PAD_PG6, ATCFG2 },
    { MUX_PAD_PG7, ATCFG2 },
    { MUX_PAD_PH0, ATCFG1 },
    { MUX_PAD_PH1, ATCFG1 },
    { MUX_PAD_PH2, ATCFG1 },
    { MUX_PAD_PH3, ATCFG1 },
    { MUX_PAD_PH4, ATCFG6 },
    { MUX_PAD_PH5, ATCFG6 },
    { MUX_PAD_PH6, ATCFG6 },
    { MUX_PAD_PH7, ATCFG6 },
    { MUX_PAD_PJ7, ATCFG4 },
    { MUX_PAD_PB0, ATCFG4 },
    { MUX_PAD_PB1, ATCFG4 },
    { MUX_PAD_PK7, ATCFG4 },
    { MUX_PAD_PI0, ATCFG2 },
    { MUX_PAD_PI1, ATCFG2 },
    { MUX_PAD_PI2, ATCFG6 },
    { MUX_PAD_PI4, ATCFG2 },
    { MUX_PAD_GEN2_I2C_SCL_PT5, ATCFG5 },
    { MUX_PAD_GEN2_I2C_SDA_PT6, ATCFG5 },
    { MUX_PAD_SDMMC4_CLK_PCC4, GMACFG },
    { MUX_PAD_SDMMC4_CMD_PT7, GMACFG },
    { MUX_PAD_SDMMC4_DAT0_PAA0, GMACFG },
    { MUX_PAD_SDMMC4_DAT1_PAA1, GMACFG },
    { MUX_PAD_SDMMC4_DAT2_PAA2, GMACFG },
    { MUX_PAD_SDMMC4_DAT3_PAA3, GMACFG },
    { MUX_PAD_SDMMC4_DAT4_PAA4, GMACFG },
    { MUX_PAD_SDMMC4_DAT5_PAA5, GMACFG },
    { MUX_PAD_SDMMC4_DAT6_PAA6, GMACFG },
    { MUX_PAD_SDMMC4_DAT7_PAA7, GMACFG },
    { MUX_PAD_CAM_MCLK_PCC0, GMGCFG },
    { MUX_PAD_PCC1, GMHCFG },
    { MUX_PAD_PBB0, GMECFG },
    { MUX_PAD_CAM_I2C_SCL_PBB1, TK1MUX_GROUP_OFFSET_GMECFG },
    { MUX_PAD_CAM_I2C_SDA_PBB2, TK1MUX_GROUP_OFFSET_GMECFG },
    { MUX_PAD_PBB3, GMECFG },
    { MUX_PAD_PBB4, GMFCFG },
    { MUX_PAD_PBB5, GMFCFG },
    { MUX_PAD_PBB6, GMFCFG },
    { MUX_PAD_PBB7, GMFCFG },
    { MUX_PAD_PCC2, GMECFG },
    { MUX_PAD_JTAG_RTCK, AOCFG0 },
    { MUX_PAD_PWR_I2C_SCL_PZ6, AOCFG1 },
    { MUX_PAD_PWR_I2C_SDA_PZ7, AOCFG1 },
    { MUX_PAD_KB_ROW0_PR0, AOCFG1 },
    { MUX_PAD_KB_ROW1_PR1, AOCFG1 },
    { MUX_PAD_KB_ROW2_PR2, AOCFG1 },
    { MUX_PAD_KB_ROW3_PR3, AOCFG1 },
    { MUX_PAD_KB_ROW4_PR4, AOCFG1 },
    { MUX_PAD_KB_ROW5_PR5, AOCFG1 },
    { MUX_PAD_KB_ROW6_PR6, AOCFG1 },
    { MUX_PAD_KB_ROW7_PR7, AOCFG1 },
    { MUX_PAD_KB_ROW8_PS0, AOCFG2 },
    { MUX_PAD_KB_ROW9_PS1, AOCFG2 },
    { MUX_PAD_KB_ROW10_PS2, AOCFG2 },
    { MUX_PAD_KB_ROW11_PS3, AOCFG2 },
    { MUX_PAD_KB_ROW12_PS4, AOCFG2 },
    { MUX_PAD_KB_ROW13_PS5, AOCFG2 },
    { MUX_PAD_KB_ROW14_PS6, AOCFG2 },
    { MUX_PAD_KB_ROW15_PS7, AOCFG2 },
    { MUX_PAD_KB_COL0_PQ0, AOCFG2 },
    { MUX_PAD_KB_COL1_PQ1, AOCFG2 },
    { MUX_PAD_KB_COL2_PQ2, AOCFG2 },
    { MUX_PAD_KB_COL3_PQ3, AOCFG2 },
    { MUX_PAD_KB_COL4_PQ4, AOCFG2 },
    { MUX_PAD_KB_COL5_PQ5, AOCFG2 },
    { MUX_PAD_KB_COL6_PQ6, AOCFG2 },
    { MUX_PAD_KB_COL7_PQ7, AOCFG2 },
    { MUX_PAD_CLK_32K_OUT_PA0, AOCFG2 },
    { MUX_PAD_CORE_PWR_REQ, 0 },
    { MUX_PAD_CPU_PWR_REQ, 0 },
    { MUX_PAD_PWR_INT_N, 0 },
    { MUX_PAD_CLK_32K_IN, AOCFG2 },
    { MUX_PAD_OWR, OWRCFG },
    { MUX_PAD_DAP1_FS_PN0, DAP1CFG },
    { MUX_PAD_DAP1_DIN_PN1, DAP1CFG },
    { MUX_PAD_DAP1_DOUT_PN2, DAP1CFG },
    { MUX_PAD_DAP1_SCLK_PN3, DAP1CFG },
    { MUX_PAD_DAP_MCLK1_REQ_PEE2, CDEV1CFG },
    { MUX_PAD_DAP_MCLK1_PW4, CDEV1CFG },
    { MUX_PAD_SPDIF_IN_PK6, DAP5CFG },
    { MUX_PAD_SPDIF_OUT_PK5, DAP5CFG },
    { MUX_PAD_DAP2_FS_PA2, DAP1CFG },
    { MUX_PAD_DAP2_DIN_PA4, DAP1CFG },
    { MUX_PAD_DAP2_DOUT_PA5, DAP1CFG },
    { MUX_PAD_DAP2_SCLK_PA3, DAP1CFG },
    { MUX_PAD_DVFS_PWM_PX0, SPICFG },
    { MUX_PAD_GPIO_X1_AUD_PX1, SPICFG },
    { MUX_PAD_GPIO_X3_AUD_PX3, SPICFG },
    { MUX_PAD_DVFS_CLK_PX2, SPICFG },
    { MUX_PAD_GPIO_X4_AUD_PX4, SPICFG },
    { MUX_PAD_GPIO_X5_AUD_PX5, SPICFG },
    { MUX_PAD_GPIO_X6_AUD_PX6, SPICFG },
    { MUX_PAD_GPIO_X7_AUD_PX7, SPICFG },
    { MUX_PAD_SDMMC3_CLK_PA6, SDIO3CFG },
    { MUX_PAD_SDMMC3_CMD_PA7, SDIO3CFG },
    { MUX_PAD_SDMMC3_DAT0_PB7, SDIO3CFG },
    { MUX_PAD_SDMMC3_DAT1_PB6, SDIO3CFG },
    { MUX_PAD_SDMMC3_DAT2_PB5, SDIO3CFG },
    { MUX_PAD_SDMMC3_DAT3_PB4, SDIO3CFG },
    { MUX_PAD_PEX_L0_RST_N_PDD1, GPVCFG },
    { MUX_PAD_PEX_L0_CLKREQ_N_PDD2, GPVCFG },
    { MUX_PAD_PEX_WAKE_N_PDD3, GPVCFG },
    { MUX_PAD_PEX_L1_RST_N_PDD5, GPVCFG },
    { MUX_PAD_PEX_L1_CLKREQ_N_PDD6, GPVCFG },
    { MUX_PAD_HDMI_CEC_PEE3, CECCFG },
    { MUX_PAD_SDMMC1_WP_N_PV3, SDIO4CFG },
    { MUX_PAD_SDMMC3_CD_N_PV2, AOCFG2 },
    { MUX_PAD_GPIO_W2_AUD_PW2, SPICFG },
    { MUX_PAD_GPIO_W3_AUD_PW3, SPICFG },
    { MUX_PAD_USB_VBUS_EN0_PN4, USB_VBUS_EN_CFG },
    { MUX_PAD_USB_VBUS_EN1_PN5, USB_VBUS_EN_CFG },
    { MUX_PAD_SDMMC3_CLK_LB_IN_PEE5, SDIO3CFG },
    { MUX_PAD_SDMMC3_CLK_LB_OUT_PEE4, SDIO3CFG },
    { MUX_PAD_GMI_CLK_LB, 0 },
    { MUX_PAD_RESET_OUT_N, 0 },
    { MUX_PAD_KB_ROW16_PT0, AOCFG2 },
    { MUX_PAD_KB_ROW17_PT1, AOCFG2 },
    { MUX_PAD_USB_VBUS_EN2_PFF1, GPVCFG },
    { MUX_PAD_PFF2, GPVCFG },
    { MUX_PAD_DP_HPD_PFF0, DAP5CFG }
};

/** Lookup the group configuration offset for a pin.
 * @param[in] mux_reg_index     Index of a mux pin. See tk1/mux.h
 * @return  -1 on error
 *          positive integer offset on success.
 */
static inline int
tk1_mux_get_group_offset_for_pin(int mux_reg_index)
{
    for (int i = 0; i < ARRAY_SIZE(mux_pin_group_mapping_table); i++) {
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

static inline int
tk1_mux_get_bitinfo_for_group(int group_offset, int desired_bitinfo)
{
    for (int i = 0; i < ARRAY_SIZE(mux_group_bitinfo_mapping_table); i++) {
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
