/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _PLATSUPPORT_PLAT_CLOCK_H_
#define _PLATSUPPORT_PLAT_CLOCK_H_

#define PXO_HZ    27000000UL
#define CXO_HZ    19200000UL
#define WCNXO_HZ  48000000UL /* WLAN */
#define SLPXO_HZ     32768UL

#define TCXO_HZ   19200000UL /* Alias: TXCO (typo in docs) */


#define APQ8064_CLK_CTL0_PADDR 0x00900000
#define APQ8064_CLK_CTL1_PADDR 0x00901000
#define APQ8064_CLK_CTL2_PADDR 0x00902000
#define APQ8064_CLK_CTL3_PADDR 0x00903000

#define APQ8064_CLK_CTL_SIZE       0x1000
#define APQ8064_CLK_CTL0_SIZE  APQ8064_CLK_CTL_SIZE
#define APQ8064_CLK_CTL1_SIZE  APQ8064_CLK_CTL_SIZE
#define APQ8064_CLK_CTL2_SIZE  APQ8064_CLK_CTL_SIZE
#define APQ8064_CLK_CTL3_SIZE  APQ8064_CLK_CTL_SIZE

enum clk_id {
    CLK_MASTER,
    CLK_PXO,
    CLK_TCXO,
    CLK_WCNXO,
    CLK_SLPXO,
#if 0
    PLL0  GPLL0   PXO 800MHz
    PLL1  MMPLL0  PXO 1332MHz
    PLL2  MMPLL1  PXO 800MHz
    PLL3  QDSPLL  PXO 1200MHz
    PLL4  LPAPLL  PXO 393.2160MHz (491.52 ver2)
    PLL5  MPLL0   CXO 288MHz
    PLL8  SPPLL   PXO 384MHz
    PLL9  SCPLL0  PXO 2000MHz
    PLL10 SCPLL1  PXO 2000MHz
    PLL11 EBI1PLL PXO 1066MHz
    PLL12 SCL2PLL PXO 1700MHz
    PLL13 WCNPLL  WCNXO (CXO used if WCNXO absent) 960MHz
    PLL14 SP2PLL  PXO 480MHz
    PLL15 MMPLL3  PXO 975MHz
    PLL16 SCPLL2  PXO 2000MHz
    PLL17 SCPLL3  PXO 2000MHz

#endif
    NCLOCKS
};

enum clock_gate {
    NCLKGATES,
};

int apq_clock_sys_init(void* clk_ctl_base0, void* clk_ctl_base1,
                       void* clk_ctl_base2, void* clk_ctl_base3,
                       clock_sys_t* clk_sys);

#endif /* _PLATSUPPORT_PLAT_CLOCK_H_ */
