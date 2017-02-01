/*
 * Copyright 2016, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(D61_BSD)
 */

#ifndef _PLATSUPPORT_PLAT_CLOCK_H
#define _PLATSUPPORT_PLAT_CLOCK_H
#include <stdint.h>

/* Register information sourced from "NVIDIA Tegra K1 Mobile Processor TECHNICAL REFERENCE MANUAL" */

#define TK1_CLKCAR_PADDR 0x60006000

#define PLLC_START 0x80
#define PLLC_END 0x90
#define PLLM_START 0x90
#define PLLM_END 0xa0
#define PLLP_START 0xa0
#define PLLP_END 0xb0
#define PLLA_START 0xb0
#define PLLA_END 0xc0
#define PLLU_START 0xc0
#define PLLU_END 0xd0
#define PLLD_START 0xd0
#define PLLD_END 0xe0
#define PLLX_START 0xe0
#define PLLX_END 0xe8
#define PLLE_START 0xe8
#define PLLE_END 0xf0
#define DFLL_BASE_0 0x2f4
#define PLLE_AUX_0 0x48c
#define PLLD2_START 0x4b8
#define PLLD2_END 0x4c0
#define PLLREFE_START 0x4c4
#define PLLREFE_END 0x4d0
#define PLLC2_START 0x4e8
#define PLLC2_END 0x4fc
#define PLLC3_START 0x4fc
#define PLLC3_END 0x510
#define PLLX_1_START 0x510
#define PLLX_1_END 0x51c
#define PLLE_AUX1_0 0x524
#define PLLP_RESHIFT_0 0x528
#define PLLU_HW_PWRDN_CFG0_0 0x530
#define PLLX_2_START 0x548
#define PLLX_2_END 0x554
#define PLLD2_1_START 0x570
#define PLLD2_1_END 0x57c
#define PLLDP_START 0x590
#define PLLDP_END 0x5a4
#define PLLC4_START 0x5a4
#define PLLC4_END 0x5b8
#define PLLP_1_START 0x67c
#define PLLP_1_END 0x684




/* These are the input clk sources to the CAR module */
enum ext_clk_input_sources {
    PMIC,
    OSC,
};

enum clk_id {
    NCLOCKS
};

enum clock_gate {
    NCLKGATES
};

/* These are PLL clks that use the input clk sources. */
enum clk_sources {
    PLLM,
    PLLX,
    PLLC,
    PLLC2,
    PLLC3,
    PLLC4,
    PLLP,
    PLLA,
    PLLU,
    PLLD,
    PLLD2,
    refPLLe,
    PLLE,
    PLLDP,
    DFLLCPU,
    GPCPLL
};


/* Primary clocks */
enum primary_clk_srcs {
    dbg_oscout,
    ck32khz_IB,
    osc_div_clk,
    car_sclk,
    clk_m,
    car_clk_m,
    dfllCPU_out,
    pllC_out,
    pllC2_out,
    pllC3_out,
    pllC4_out,
    pllM_out,
    int_pllA_out,
    pllD_out,
    pllD2_out,
    pllDP_out,
    pllU,
    refPLLE_out,
    pllE_out0,
    pllP_out,
    pllX,
    //GPCPLL,
    clk_s,
    car_clk_s,
};

/* These go through a NV divider */
enum derived_clk_srcs {
    pllC_out1,
    pllM_out1,
    pllP_out1,
    pllP_out2,
    pllP_out3,
    pllP_out4,
    pllP_out5,
    pllA_out0,
};

/* Source clocks */
enum src_clocks {
    actmon_clk_t,
    adx0_r_clk,
    adx1_r_clk,
    amx0_r_clk,
    amx1_r_clk,
    audio_r_clk,
    cilab_clk_t,
    cilcd_clk_t,
    cile_clk_t,
    clk72mhz_clk,
    csite_clk_t,
    dam0_r_clk,
    dam1_r_clk,
    dam2_r_clk,
    display_clk_t,
    displayb_clk_t,
    dsia_lp_clk_t,
    dsib_lp_clk_t,
    dvfs_ref_r_clk,
    dvfs_soc_r_clk,
    emc_dll_clk_t,
    emc_latency_clk_t,
    entropy_r_clk,
    extperiph1_clk,
    extperiph2_clk,
    extperiph3_clk,
    hda_r_clk,
    hdmi_audio_clk_t,
    hdmi_clk_t,
    host1x_clk_t,
    hsi_clk_t,
    i2c1_r_clk,
    i2c2_r_clk,
    i2c3_r_clk,
    i2c4_r_clk,
    i2c5_r_clk,
    i2c6_r_clk,
    i2c_slow_clk,
    i2s0_r_clk,
    i2s1_r_clk,
    i2s2_r_clk,
    i2s3_r_clk,
    i2s4_r_clk,
    int_emc_clk,
    int_hda2codec_2x_clk,
    isp_r_clk_t,
    la_clk_t,
    lvds0_pad_clockin_t,
    mselect_clk_t,
    msenc_clk_t,
    nor_r_clk,
    owr_r_clk,
    pex_txclkref,
    pex_txclkref_grp0,
    pex_txclkref_grp1,
    pex_txclkref_grp2,
    pex_txclkref_tms,
    pwm_r_clk,
    sata_oob_clk_t,
    sclk_sel,
    sdmmc1_r_clk_t,
    sdmmc2_r_clk_t,
    sdmmc3_r_clk_t,
    sdmmc4_r_clk_t,
    se_clk_t,
    soc_therm_t,
    spdif_in_r_clk,
    spdif_out_r_clk,
    spi1_clk_t,
    spi2_clk_t,
    spi3_clk_t,
    spi4_clk_t,
    spi5_clk_t,
    spi6_clk_t,
    sys2hsio_sata_r_clk,
    traceclkin_clk_t,
    tsec_clk_t,
    tsensor_r_clk,
    uarta_r_clk,
    uartb_r_clk,
    uartc_r_clk,
    uartd_r_clk,
    vde_clk_t,
    vfir_clk_t,
    vi_clk_t,
    vi_sensor2_clk,
    vi_sensor_clk,
    vic_clk_t,
    xusb_120m_clk,
    xusb_core_clk,
    xusb_core_dev_clk,
    xusb_falcon_clk,
    xusb_fs_clk,
    NUM_OTHER_CLOCKS
};

/* The TK1 Clock and reset controller frame has lots of registers which can
    be broken up into the following categories:
     - CLK_PLL: Registers that controll the PLLs
     - CLK_ENBRST_DEVICES: Registers for enabling and resetting devices
                            Each device has a reset line and clk enable line
     - CLK_SOURCE: Source device regisers, one register for each src_clock
     - CLK_MISC: All other registers.  Does things like trimming and processor resets
     - CLK_RESERVED: No register at memory location
 */
typedef enum clk_register_type {
    CLK_RESERVED,
    CLK_PLL,
    CLK_ENBRST_DEVICES,
    CLK_SOURCE,
    CLK_MISC,
    NUM_REGISTER_TYPES
} clk_register_type_t;

/* The controller has 6 sets of enable and reset registers.  Each device that
    can be enabled/disabled/reset has a bit in one of these register banks */
typedef enum register_bank {
    REG_L,
    REG_H,
    REG_U,
    REG_V,
    REG_W,
    REG_X,
    NUM_REGISTER_BANKS
} register_bank_t;

/* The different types of CLK_ENBRST_DEVICES registers */
typedef enum register_access_type {
    ENB_SET,
    ENB_CLR,
    ENB_VAL,
    RST_SET,
    RST_CLR,
    RST_VAL,
    NUM_ACCESS_TYPES
} register_access_type_t;


struct enbrst_type {
    register_bank_t rb;
    register_access_type_t at;
};

struct source_type {
    enum src_clocks clks;
};

/* Clock register struct for storing register layout in the device frame.
    See clk_register_t tk1_clk_registers. */
typedef struct clk_register {
    enum clk_register_type reg_type;
    union {
        struct source_type st;
        struct enbrst_type eb;
    };
} clk_register_t;

extern const clk_register_t tk1_clk_registers[];

#endif /* _PLATSUPPORT_PLAT_CLOCK_H */
