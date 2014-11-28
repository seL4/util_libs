/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <stdint.h>
#include "mux.h"
#include <platsupport/gpio.h>
#include "../../services.h"

//#define MUX_DEBUG
#ifdef MUX_DEBUG
#define DMUX(...) printf("MUX: " __VA_ARGS__)
#else
#define DMUX(...) do{}while(0)
#endif


#define IMX6_IOMUXC_PADDR 0x020E0000
#define IMX6_IOMUXC_SIZE  0x1000

#define IOMUXC_MUXCTL_OFFSET        0x4C
#define IOMUXC_PADCTL_OFFSET        0x360

#define IOMUXC_MUXCTL_FORCE_INPUT   BIT(4)
#define IOMUXC_MUXCTL_MODE(x)       ((x) & 0x7)
#define IOMUXC_MUXCTL_MODE_MASK     IOMUXC_MUXCTL_MODE(0x7)

/* NOTE: Daisy field is only 1 but for some registers */
#define IOMUXC_IS_DAISY(x)   ((x) & 0x3)
#define IOMUXC_IS_DAISY_MASK IOMUXC_IS_DAISY(0x3)


struct imx6_iomuxc_regs {
    /*** GPR ***/
    uint32_t gpr0;                              /* +0x000 */
    uint32_t gpr1;                              /* +0x004 */
    uint32_t gpr2;                              /* +0x008 */
    uint32_t gpr3;                              /* +0x00C */
    uint32_t gpr4;                              /* +0x010 */
    uint32_t gpr5;                              /* +0x014 */
    uint32_t gpr6;                              /* +0x018 */
    uint32_t gpr7;                              /* +0x01C */
    uint32_t gpr8;                              /* +0x020 */
    uint32_t gpr9;                              /* +0x024 */
    uint32_t gpr10;                             /* +0x028 */
    uint32_t gpr11;                             /* +0x02C */
    uint32_t gpr12;                             /* +0x030 */
    uint32_t gpr13;                             /* +0x034 */
    uint32_t res0[2];
    /*** MUX control ***/
    uint32_t res1[3];
    uint32_t sw_mux_ctl_pad_sd2_data1;          /* +0x04C */
    uint32_t sw_mux_ctl_pad_sd2_data2;          /* +0x050 */
    uint32_t sw_mux_ctl_pad_sd2_data0;          /* +0x054 */
    uint32_t sw_mux_ctl_pad_rgmii_txc;          /* +0x058 */
    uint32_t sw_mux_ctl_pad_rgmii_td0;          /* +0x05C */
    uint32_t sw_mux_ctl_pad_rgmii_td1;          /* +0x060 */
    uint32_t sw_mux_ctl_pad_rgmii_td2;          /* +0x064 */
    uint32_t sw_mux_ctl_pad_rgmii_td3;          /* +0x068 */
    uint32_t sw_mux_ctl_pad_rgmii_rx_ctl;       /* +0x06C */
    uint32_t sw_mux_ctl_pad_rgmii_rd0;          /* +0x070 */
    uint32_t sw_mux_ctl_pad_rgmii_tx_ctl;       /* +0x074 */
    uint32_t sw_mux_ctl_pad_rgmii_rd1;          /* +0x078 */
    uint32_t sw_mux_ctl_pad_rgmii_rd2;          /* +0x07C */
    uint32_t sw_mux_ctl_pad_rgmii_rd3;          /* +0x080 */
    uint32_t sw_mux_ctl_pad_rgmii_rxc;          /* +0x084 */
    uint32_t sw_mux_ctl_pad_eim_addr25;         /* +0x088 */
    uint32_t sw_mux_ctl_pad_eim_eb2;            /* +0x08C */
    uint32_t sw_mux_ctl_pad_eim_data16;         /* +0x090 */
    uint32_t sw_mux_ctl_pad_eim_data17;         /* +0x094 */
    uint32_t sw_mux_ctl_pad_eim_data18;         /* +0x098 */
    uint32_t sw_mux_ctl_pad_eim_data19;         /* +0x09C */
    uint32_t sw_mux_ctl_pad_eim_data20;         /* +0x0A0 */
    uint32_t sw_mux_ctl_pad_eim_data21;         /* +0x0A4 */
    uint32_t sw_mux_ctl_pad_eim_data22;         /* +0x0A8 */
    uint32_t sw_mux_ctl_pad_eim_data23;         /* +0x0AC */
    uint32_t sw_mux_ctl_pad_eim_eb3;            /* +0x0B0 */
    uint32_t sw_mux_ctl_pad_eim_data24;         /* +0x0B4 */
    uint32_t sw_mux_ctl_pad_eim_data25;         /* +0x0B8 */
    uint32_t sw_mux_ctl_pad_eim_data26;         /* +0x0BC */
    uint32_t sw_mux_ctl_pad_eim_data27;         /* +0x0C0 */
    uint32_t sw_mux_ctl_pad_eim_data28;         /* +0x0C4 */
    uint32_t sw_mux_ctl_pad_eim_data29;         /* +0x0C8 */
    uint32_t sw_mux_ctl_pad_eim_data30;         /* +0x0CC */
    uint32_t sw_mux_ctl_pad_eim_data31;         /* +0x0D0 */
    uint32_t sw_mux_ctl_pad_eim_addr24;         /* +0x0D4 */
    uint32_t sw_mux_ctl_pad_eim_addr23;         /* +0x0D8 */
    uint32_t sw_mux_ctl_pad_eim_addr22;         /* +0x0DC */
    uint32_t sw_mux_ctl_pad_eim_addr21;         /* +0x0E0 */
    uint32_t sw_mux_ctl_pad_eim_addr20;         /* +0x0E4 */
    uint32_t sw_mux_ctl_pad_eim_addr19;         /* +0x0E8 */
    uint32_t sw_mux_ctl_pad_eim_addr18;         /* +0x0EC */
    uint32_t sw_mux_ctl_pad_eim_addr17;         /* +0x0F0 */
    uint32_t sw_mux_ctl_pad_eim_addr16;         /* +0x0F4 */
    uint32_t sw_mux_ctl_pad_eim_cs0;            /* +0x0F8 */
    uint32_t sw_mux_ctl_pad_eim_cs1;            /* +0x0FC */
    uint32_t sw_mux_ctl_pad_eim_oe;             /* +0x100 */
    uint32_t sw_mux_ctl_pad_eim_rw;             /* +0x104 */
    uint32_t sw_mux_ctl_pad_eim_lba;            /* +0x108 */
    uint32_t sw_mux_ctl_pad_eim_eb0;            /* +0x10C */
    uint32_t sw_mux_ctl_pad_eim_eb1;            /* +0x110 */
    uint32_t sw_mux_ctl_pad_eim_ad00;           /* +0x114 */
    uint32_t sw_mux_ctl_pad_eim_ad01;           /* +0x118 */
    uint32_t sw_mux_ctl_pad_eim_ad02;           /* +0x11C */
    uint32_t sw_mux_ctl_pad_eim_ad03;           /* +0x120 */
    uint32_t sw_mux_ctl_pad_eim_ad04;           /* +0x124 */
    uint32_t sw_mux_ctl_pad_eim_ad05;           /* +0x128 */
    uint32_t sw_mux_ctl_pad_eim_ad06;           /* +0x12C */
    uint32_t sw_mux_ctl_pad_eim_ad07;           /* +0x130 */
    uint32_t sw_mux_ctl_pad_eim_ad08;           /* +0x134 */
    uint32_t sw_mux_ctl_pad_eim_ad09;           /* +0x138 */
    uint32_t sw_mux_ctl_pad_eim_ad10;           /* +0x13C */
    uint32_t sw_mux_ctl_pad_eim_ad11;           /* +0x140 */
    uint32_t sw_mux_ctl_pad_eim_ad12;           /* +0x144 */
    uint32_t sw_mux_ctl_pad_eim_ad13;           /* +0x148 */
    uint32_t sw_mux_ctl_pad_eim_ad14;           /* +0x14C */
    uint32_t sw_mux_ctl_pad_eim_ad15;           /* +0x150 */
    uint32_t sw_mux_ctl_pad_eim_wait;           /* +0x154 */
    uint32_t sw_mux_ctl_pad_eim_bclk;           /* +0x158 */
    uint32_t sw_mux_ctl_pad_di0_disp_clk;       /* +0x15C */
    uint32_t sw_mux_ctl_pad_di0_pin15;          /* +0x160 */
    uint32_t sw_mux_ctl_pad_di0_pin02;          /* +0x164 */
    uint32_t sw_mux_ctl_pad_di0_pin03;          /* +0x168 */
    uint32_t sw_mux_ctl_pad_di0_pin04;          /* +0x16C */
    uint32_t sw_mux_ctl_pad_disp0_data00;       /* +0x170 */
    uint32_t sw_mux_ctl_pad_disp0_data01;       /* +0x174 */
    uint32_t sw_mux_ctl_pad_disp0_data02;       /* +0x178 */
    uint32_t sw_mux_ctl_pad_disp0_data03;       /* +0x17C */
    uint32_t sw_mux_ctl_pad_disp0_data04;       /* +0x180 */
    uint32_t sw_mux_ctl_pad_disp0_data05;       /* +0x184 */
    uint32_t sw_mux_ctl_pad_disp0_data06;       /* +0x188 */
    uint32_t sw_mux_ctl_pad_disp0_data07;       /* +0x18C */
    uint32_t sw_mux_ctl_pad_disp0_data08;       /* +0x190 */
    uint32_t sw_mux_ctl_pad_disp0_data09;       /* +0x194 */
    uint32_t sw_mux_ctl_pad_disp0_data10;       /* +0x198 */
    uint32_t sw_mux_ctl_pad_disp0_data11;       /* +0x19C */
    uint32_t sw_mux_ctl_pad_disp0_data12;       /* +0x1A0 */
    uint32_t sw_mux_ctl_pad_disp0_data13;       /* +0x1A4 */
    uint32_t sw_mux_ctl_pad_disp0_data14;       /* +0x1A8 */
    uint32_t sw_mux_ctl_pad_disp0_data15;       /* +0x1AC */
    uint32_t sw_mux_ctl_pad_disp0_data16;       /* +0x1B0 */
    uint32_t sw_mux_ctl_pad_disp0_data17;       /* +0x1B4 */
    uint32_t sw_mux_ctl_pad_disp0_data18;       /* +0x1B8 */
    uint32_t sw_mux_ctl_pad_disp0_data19;       /* +0x1BC */
    uint32_t sw_mux_ctl_pad_disp0_data20;       /* +0x1C0 */
    uint32_t sw_mux_ctl_pad_disp0_data21;       /* +0x1C4 */
    uint32_t sw_mux_ctl_pad_disp0_data22;       /* +0x1C8 */
    uint32_t sw_mux_ctl_pad_disp0_data23;       /* +0x1CC */
    uint32_t sw_mux_ctl_pad_enet_mdio;          /* +0x1D0 */
    uint32_t sw_mux_ctl_pad_enet_ref_clk;       /* +0x1D4 */
    uint32_t sw_mux_ctl_pad_enet_rx_er;         /* +0x1D8 */
    uint32_t sw_mux_ctl_pad_enet_crs_dv;        /* +0x1DC */
    uint32_t sw_mux_ctl_pad_enet_rx_data1;      /* +0x1E0 */
    uint32_t sw_mux_ctl_pad_enet_rx_data0;      /* +0x1E4 */
    uint32_t sw_mux_ctl_pad_enet_tx_en;         /* +0x1E8 */
    uint32_t sw_mux_ctl_pad_enet_tx_data1;      /* +0x1EC */
    uint32_t sw_mux_ctl_pad_enet_tx_data0;      /* +0x1F0 */
    uint32_t sw_mux_ctl_pad_enet_mdc;           /* +0x1F4 */
    uint32_t sw_mux_ctl_pad_key_col0;           /* +0x1F8 */
    uint32_t sw_mux_ctl_pad_key_row0;           /* +0x1FC */
    uint32_t sw_mux_ctl_pad_key_col1;           /* +0x200 */
    uint32_t sw_mux_ctl_pad_key_row1;           /* +0x204 */
    uint32_t sw_mux_ctl_pad_key_col2;           /* +0x208 */
    uint32_t sw_mux_ctl_pad_key_row2;           /* +0x20C */
    uint32_t sw_mux_ctl_pad_key_col3;           /* +0x210 */
    uint32_t sw_mux_ctl_pad_key_row3;           /* +0x214 */
    uint32_t sw_mux_ctl_pad_key_col4;           /* +0x218 */
    uint32_t sw_mux_ctl_pad_key_row4;           /* +0x21C */
    uint32_t sw_mux_ctl_pad_gpio00;             /* +0x220 */
    uint32_t sw_mux_ctl_pad_gpio01;             /* +0x224 */
    uint32_t sw_mux_ctl_pad_gpio09;             /* +0x228 */
    uint32_t sw_mux_ctl_pad_gpio03;             /* +0x22C */
    uint32_t sw_mux_ctl_pad_gpio06;             /* +0x230 */
    uint32_t sw_mux_ctl_pad_gpio02;             /* +0x234 */
    uint32_t sw_mux_ctl_pad_gpio04;             /* +0x238 */
    uint32_t sw_mux_ctl_pad_gpio05;             /* +0x23C */
    uint32_t sw_mux_ctl_pad_gpio07;             /* +0x240 */
    uint32_t sw_mux_ctl_pad_gpio08;             /* +0x244 */
    uint32_t sw_mux_ctl_pad_gpio16;             /* +0x248 */
    uint32_t sw_mux_ctl_pad_gpio17;             /* +0x24C */
    uint32_t sw_mux_ctl_pad_gpio18;             /* +0x250 */
    uint32_t sw_mux_ctl_pad_gpio19;             /* +0x254 */
    uint32_t sw_mux_ctl_pad_csi0_pixclk;        /* +0x258 */
    uint32_t sw_mux_ctl_pad_csi0_hsync;         /* +0x25C */
    uint32_t sw_mux_ctl_pad_csi0_data_en;       /* +0x260 */
    uint32_t sw_mux_ctl_pad_csi0_vsync;         /* +0x264 */
    uint32_t sw_mux_ctl_pad_csi0_data04;        /* +0x268 */
    uint32_t sw_mux_ctl_pad_csi0_data05;        /* +0x26C */
    uint32_t sw_mux_ctl_pad_csi0_data06;        /* +0x270 */
    uint32_t sw_mux_ctl_pad_csi0_data07;        /* +0x274 */
    uint32_t sw_mux_ctl_pad_csi0_data08;        /* +0x278 */
    uint32_t sw_mux_ctl_pad_csi0_data09;        /* +0x27C */
    uint32_t sw_mux_ctl_pad_csi0_data10;        /* +0x280 */
    uint32_t sw_mux_ctl_pad_csi0_data11;        /* +0x284 */
    uint32_t sw_mux_ctl_pad_csi0_data12;        /* +0x288 */
    uint32_t sw_mux_ctl_pad_csi0_data13;        /* +0x28C */
    uint32_t sw_mux_ctl_pad_csi0_data14;        /* +0x290 */
    uint32_t sw_mux_ctl_pad_csi0_data15;        /* +0x294 */
    uint32_t sw_mux_ctl_pad_csi0_data16;        /* +0x298 */
    uint32_t sw_mux_ctl_pad_csi0_data17;        /* +0x29C */
    uint32_t sw_mux_ctl_pad_csi0_data18;        /* +0x2A0 */
    uint32_t sw_mux_ctl_pad_csi0_data19;        /* +0x2A4 */
    uint32_t sw_mux_ctl_pad_sd3_data7;          /* +0x2A8 */
    uint32_t sw_mux_ctl_pad_sd3_data6;          /* +0x2AC */
    uint32_t sw_mux_ctl_pad_sd3_data5;          /* +0x2B0 */
    uint32_t sw_mux_ctl_pad_sd3_data4;          /* +0x2B4 */
    uint32_t sw_mux_ctl_pad_sd3_cmd;            /* +0x2B8 */
    uint32_t sw_mux_ctl_pad_sd3_clk;            /* +0x2BC */
    uint32_t sw_mux_ctl_pad_sd3_data0;          /* +0x2C0 */
    uint32_t sw_mux_ctl_pad_sd3_data1;          /* +0x2C4 */
    uint32_t sw_mux_ctl_pad_sd3_data2;          /* +0x2C8 */
    uint32_t sw_mux_ctl_pad_sd3_data3;          /* +0x2CC */
    uint32_t sw_mux_ctl_pad_sd3_reset;          /* +0x2D0 */
    uint32_t sw_mux_ctl_pad_nand_cle;           /* +0x2D4 */
    uint32_t sw_mux_ctl_pad_nand_ale;           /* +0x2D8 */
    uint32_t sw_mux_ctl_pad_nand_wp_b;          /* +0x2DC */
    uint32_t sw_mux_ctl_pad_nand_ready;         /* +0x2E0 */
    uint32_t sw_mux_ctl_pad_nand_cs0_b;         /* +0x2E4 */
    uint32_t sw_mux_ctl_pad_nand_cs1_b;         /* +0x2E8 */
    uint32_t sw_mux_ctl_pad_nand_cs2_b;         /* +0x2EC */
    uint32_t sw_mux_ctl_pad_nand_cs3_b;         /* +0x2F0 */
    uint32_t sw_mux_ctl_pad_sd4_cmd;            /* +0x2F4 */
    uint32_t sw_mux_ctl_pad_sd4_clk;            /* +0x2F8 */
    uint32_t sw_mux_ctl_pad_nand_data00;        /* +0x2FC */
    uint32_t sw_mux_ctl_pad_nand_data01;        /* +0x300 */
    uint32_t sw_mux_ctl_pad_nand_data02;        /* +0x304 */
    uint32_t sw_mux_ctl_pad_nand_data03;        /* +0x308 */
    uint32_t sw_mux_ctl_pad_nand_data04;        /* +0x30C */
    uint32_t sw_mux_ctl_pad_nand_data05;        /* +0x310 */
    uint32_t sw_mux_ctl_pad_nand_data06;        /* +0x314 */
    uint32_t sw_mux_ctl_pad_nand_data07;        /* +0x318 */
    uint32_t sw_mux_ctl_pad_sd4_data0;          /* +0x31C */
    uint32_t sw_mux_ctl_pad_sd4_data1;          /* +0x320 */
    uint32_t sw_mux_ctl_pad_sd4_data2;          /* +0x324 */
    uint32_t sw_mux_ctl_pad_sd4_data3;          /* +0x328 */
    uint32_t sw_mux_ctl_pad_sd4_data4;          /* +0x32C */
    uint32_t sw_mux_ctl_pad_sd4_data5;          /* +0x330 */
    uint32_t sw_mux_ctl_pad_sd4_data6;          /* +0x334 */
    uint32_t sw_mux_ctl_pad_sd4_data7;          /* +0x338 */
    uint32_t sw_mux_ctl_pad_sd1_data1;          /* +0x33C */
    uint32_t sw_mux_ctl_pad_sd1_data0;          /* +0x340 */
    uint32_t sw_mux_ctl_pad_sd1_data3;          /* +0x344 */
    uint32_t sw_mux_ctl_pad_sd1_cmd;            /* +0x348 */
    uint32_t sw_mux_ctl_pad_sd1_data2;          /* +0x34C */
    uint32_t sw_mux_ctl_pad_sd1_clk;            /* +0x350 */
    uint32_t sw_mux_ctl_pad_sd2_clk;            /* +0x354 */
    uint32_t sw_mux_ctl_pad_sd2_cmd;            /* +0x358 */
    uint32_t sw_mux_ctl_pad_sd2_data3;          /* +0x35C */
    /*** Pad Control ***/
    uint32_t sw_pad_ctl_pad_sd2_data1;          /* +0x360 */
    uint32_t sw_pad_ctl_pad_sd2_data2;          /* +0x364 */
    uint32_t sw_pad_ctl_pad_sd2_data0;          /* +0x368 */
    uint32_t sw_pad_ctl_pad_rgmii_txc;          /* +0x36C */
    uint32_t sw_pad_ctl_pad_rgmii_td0;          /* +0x370 */
    uint32_t sw_pad_ctl_pad_rgmii_td1;          /* +0x374 */
    uint32_t sw_pad_ctl_pad_rgmii_td2;          /* +0x378 */
    uint32_t sw_pad_ctl_pad_rgmii_td3;          /* +0x37C */
    uint32_t sw_pad_ctl_pad_rgmii_rx_ctl;       /* +0x380 */
    uint32_t sw_pad_ctl_pad_rgmii_rd0;          /* +0x384 */
    uint32_t sw_pad_ctl_pad_rgmii_tx_ctl;       /* +0x388 */
    uint32_t sw_pad_ctl_pad_rgmii_rd1;          /* +0x38C */
    uint32_t sw_pad_ctl_pad_rgmii_rd2;          /* +0x390 */
    uint32_t sw_pad_ctl_pad_rgmii_rd3;          /* +0x394 */
    uint32_t sw_pad_ctl_pad_rgmii_rxc;          /* +0x398 */
    uint32_t sw_pad_ctl_pad_eim_addr25;         /* +0x39C */
    uint32_t sw_pad_ctl_pad_eim_eb2;            /* +0x3A0 */
    uint32_t sw_pad_ctl_pad_eim_data16;         /* +0x3A4 */
    uint32_t sw_pad_ctl_pad_eim_data17;         /* +0x3A8 */
    uint32_t sw_pad_ctl_pad_eim_data18;         /* +0x3AC */
    uint32_t sw_pad_ctl_pad_eim_data19;         /* +0x3B0 */
    uint32_t sw_pad_ctl_pad_eim_data20;         /* +0x3B4 */
    uint32_t sw_pad_ctl_pad_eim_data21;         /* +0x3B8 */
    uint32_t sw_pad_ctl_pad_eim_data22;         /* +0x3BC */
    uint32_t sw_pad_ctl_pad_eim_data23;         /* +0x3C0 */
    uint32_t sw_pad_ctl_pad_eim_eb3;            /* +0x3C4 */
    uint32_t sw_pad_ctl_pad_eim_data24;         /* +0x3C8 */
    uint32_t sw_pad_ctl_pad_eim_data25;         /* +0x3CC */
    uint32_t sw_pad_ctl_pad_eim_data26;         /* +0x3D0 */
    uint32_t sw_pad_ctl_pad_eim_data27;         /* +0x3D4 */
    uint32_t sw_pad_ctl_pad_eim_data28;         /* +0x3D8 */
    uint32_t sw_pad_ctl_pad_eim_data29;         /* +0x3DC */
    uint32_t sw_pad_ctl_pad_eim_data30;         /* +0x3E0 */
    uint32_t sw_pad_ctl_pad_eim_data31;         /* +0x3E4 */
    uint32_t sw_pad_ctl_pad_eim_addr24;         /* +0x3E8 */
    uint32_t sw_pad_ctl_pad_eim_addr23;         /* +0x3EC */
    uint32_t sw_pad_ctl_pad_eim_addr22;         /* +0x3F0 */
    uint32_t sw_pad_ctl_pad_eim_addr21;         /* +0x3F4 */
    uint32_t sw_pad_ctl_pad_eim_addr20;         /* +0x3F8 */
    uint32_t sw_pad_ctl_pad_eim_addr19;         /* +0x3FC */
    uint32_t sw_pad_ctl_pad_eim_addr18;         /* +0x400 */
    uint32_t sw_pad_ctl_pad_eim_addr17;         /* +0x404 */
    uint32_t sw_pad_ctl_pad_eim_addr16;         /* +0x408 */
    uint32_t sw_pad_ctl_pad_eim_cs0;            /* +0x40C */
    uint32_t sw_pad_ctl_pad_eim_cs1;            /* +0x410 */
    uint32_t sw_pad_ctl_pad_eim_oe;             /* +0x414 */
    uint32_t sw_pad_ctl_pad_eim_rw;             /* +0x418 */
    uint32_t sw_pad_ctl_pad_eim_lba;            /* +0x41C */
    uint32_t sw_pad_ctl_pad_eim_eb0;            /* +0x420 */
    uint32_t sw_pad_ctl_pad_eim_eb1;            /* +0x424 */
    uint32_t sw_pad_ctl_pad_eim_ad00;           /* +0x428 */
    uint32_t sw_pad_ctl_pad_eim_ad01;           /* +0x42C */
    uint32_t sw_pad_ctl_pad_eim_ad02;           /* +0x430 */
    uint32_t sw_pad_ctl_pad_eim_ad03;           /* +0x434 */
    uint32_t sw_pad_ctl_pad_eim_ad04;           /* +0x438 */
    uint32_t sw_pad_ctl_pad_eim_ad05;           /* +0x43C */
    uint32_t sw_pad_ctl_pad_eim_ad06;           /* +0x440 */
    uint32_t sw_pad_ctl_pad_eim_ad07;           /* +0x444 */
    uint32_t sw_pad_ctl_pad_eim_ad08;           /* +0x448 */
    uint32_t sw_pad_ctl_pad_eim_ad09;           /* +0x44C */
    uint32_t sw_pad_ctl_pad_eim_ad10;           /* +0x450 */
    uint32_t sw_pad_ctl_pad_eim_ad11;           /* +0x454 */
    uint32_t sw_pad_ctl_pad_eim_ad12;           /* +0x458 */
    uint32_t sw_pad_ctl_pad_eim_ad13;           /* +0x45C */
    uint32_t sw_pad_ctl_pad_eim_ad14;           /* +0x460 */
    uint32_t sw_pad_ctl_pad_eim_ad15;           /* +0x464 */
    uint32_t sw_pad_ctl_pad_eim_wait;           /* +0x468 */
    uint32_t sw_pad_ctl_pad_eim_bclk;           /* +0x46C */
    uint32_t sw_pad_ctl_pad_di0_disp_clk;       /* +0x470 */
    uint32_t sw_pad_ctl_pad_di0_pin15;          /* +0x474 */
    uint32_t sw_pad_ctl_pad_di0_pin02;          /* +0x478 */
    uint32_t sw_pad_ctl_pad_di0_pin03;          /* +0x47C */
    uint32_t sw_pad_ctl_pad_di0_pin04;          /* +0x480 */
    uint32_t sw_pad_ctl_pad_disp0_data00;       /* +0x484 */
    uint32_t sw_pad_ctl_pad_disp0_data01;       /* +0x488 */
    uint32_t sw_pad_ctl_pad_disp0_data02;       /* +0x48C */
    uint32_t sw_pad_ctl_pad_disp0_data03;       /* +0x490 */
    uint32_t sw_pad_ctl_pad_disp0_data04;       /* +0x494 */
    uint32_t sw_pad_ctl_pad_disp0_data05;       /* +0x498 */
    uint32_t sw_pad_ctl_pad_disp0_data06;       /* +0x49C */
    uint32_t sw_pad_ctl_pad_disp0_data07;       /* +0x4A0 */
    uint32_t sw_pad_ctl_pad_disp0_data08;       /* +0x4A4 */
    uint32_t sw_pad_ctl_pad_disp0_data09;       /* +0x4A8 */
    uint32_t sw_pad_ctl_pad_disp0_data10;       /* +0x4AC */
    uint32_t sw_pad_ctl_pad_disp0_data11;       /* +0x4B0 */
    uint32_t sw_pad_ctl_pad_disp0_data12;       /* +0x4B4 */
    uint32_t sw_pad_ctl_pad_disp0_data13;       /* +0x4B8 */
    uint32_t sw_pad_ctl_pad_disp0_data14;       /* +0x4BC */
    uint32_t sw_pad_ctl_pad_disp0_data15;       /* +0x4C0 */
    uint32_t sw_pad_ctl_pad_disp0_data16;       /* +0x4C4 */
    uint32_t sw_pad_ctl_pad_disp0_data17;       /* +0x4C8 */
    uint32_t sw_pad_ctl_pad_disp0_data18;       /* +0x4CC */
    uint32_t sw_pad_ctl_pad_disp0_data19;       /* +0x4D0 */
    uint32_t sw_pad_ctl_pad_disp0_data20;       /* +0x4D4 */
    uint32_t sw_pad_ctl_pad_disp0_data21;       /* +0x4D8 */
    uint32_t sw_pad_ctl_pad_disp0_data22;       /* +0x4DC */
    uint32_t sw_pad_ctl_pad_disp0_data23;       /* +0x4E0 */
    uint32_t sw_pad_ctl_pad_enet_mdio;          /* +0x4E4 */
    uint32_t sw_pad_ctl_pad_enet_ref_clk;       /* +0x4E8 */
    uint32_t sw_pad_ctl_pad_enet_rx_er;         /* +0x4EC */
    uint32_t sw_pad_ctl_pad_enet_crs_dv;        /* +0x4F0 */
    uint32_t sw_pad_ctl_pad_enet_rx_data1;      /* +0x4F4 */
    uint32_t sw_pad_ctl_pad_enet_rx_data0;      /* +0x4F8 */
    uint32_t sw_pad_ctl_pad_enet_tx_en;         /* +0x4FC */
    uint32_t sw_pad_ctl_pad_enet_tx_data1;      /* +0x500 */
    uint32_t sw_pad_ctl_pad_enet_tx_data0;      /* +0x504 */
    uint32_t sw_pad_ctl_pad_enet_mdc;           /* +0x508 */
    uint32_t sw_pad_ctl_pad_dram_sdqs5_p;       /* +0x50C */
    uint32_t sw_pad_ctl_pad_dram_dqm5;          /* +0x510 */
    uint32_t sw_pad_ctl_pad_dram_dqm4;          /* +0x514 */
    uint32_t sw_pad_ctl_pad_dram_sdqs4_p;       /* +0x518 */
    uint32_t sw_pad_ctl_pad_dram_sdqs3_p;       /* +0x51C */
    uint32_t sw_pad_ctl_pad_dram_dqm3;          /* +0x520 */
    uint32_t sw_pad_ctl_pad_dram_sdqs2_p;       /* +0x524 */
    uint32_t sw_pad_ctl_pad_dram_dqm2;          /* +0x528 */
    uint32_t sw_pad_ctl_pad_dram_addr00;        /* +0x52C */
    uint32_t sw_pad_ctl_pad_dram_addr01;        /* +0x530 */
    uint32_t sw_pad_ctl_pad_dram_addr02;        /* +0x534 */
    uint32_t sw_pad_ctl_pad_dram_addr03;        /* +0x538 */
    uint32_t sw_pad_ctl_pad_dram_addr04;        /* +0x53C */
    uint32_t sw_pad_ctl_pad_dram_addr05;        /* +0x540 */
    uint32_t sw_pad_ctl_pad_dram_addr06;        /* +0x544 */
    uint32_t sw_pad_ctl_pad_dram_addr07;        /* +0x548 */
    uint32_t sw_pad_ctl_pad_dram_addr08;        /* +0x54C */
    uint32_t sw_pad_ctl_pad_dram_addr09;        /* +0x550 */
    uint32_t sw_pad_ctl_pad_dram_addr10;        /* +0x554 */
    uint32_t sw_pad_ctl_pad_dram_addr11;        /* +0x558 */
    uint32_t sw_pad_ctl_pad_dram_addr12;        /* +0x55C */
    uint32_t sw_pad_ctl_pad_dram_addr13;        /* +0x560 */
    uint32_t sw_pad_ctl_pad_dram_addr14;        /* +0x564 */
    uint32_t sw_pad_ctl_pad_dram_addr15;        /* +0x568 */
    uint32_t sw_pad_ctl_pad_dram_cas;           /* +0x56C */
    uint32_t sw_pad_ctl_pad_dram_cs0;           /* +0x570 */
    uint32_t sw_pad_ctl_pad_dram_cs1;           /* +0x574 */
    uint32_t sw_pad_ctl_pad_dram_ras;           /* +0x578 */
    uint32_t sw_pad_ctl_pad_dram_reset;         /* +0x57C */
    uint32_t sw_pad_ctl_pad_dram_sdba0;         /* +0x580 */
    uint32_t sw_pad_ctl_pad_dram_sdba1;         /* +0x584 */
    uint32_t sw_pad_ctl_pad_dram_sdclk0_p;      /* +0x588 */
    uint32_t sw_pad_ctl_pad_dram_sdba2;         /* +0x58C */
    uint32_t sw_pad_ctl_pad_dram_sdcke0;        /* +0x590 */
    uint32_t sw_pad_ctl_pad_dram_sdclk1_p;      /* +0x594 */
    uint32_t sw_pad_ctl_pad_dram_sdcke1;        /* +0x598 */
    uint32_t sw_pad_ctl_pad_dram_odt0;          /* +0x59C */
    uint32_t sw_pad_ctl_pad_dram_odt1;          /* +0x5A0 */
    uint32_t sw_pad_ctl_pad_dram_sdwe;          /* +0x5A4 */
    uint32_t sw_pad_ctl_pad_dram_sdqs0_p;       /* +0x5A8 */
    uint32_t sw_pad_ctl_pad_dram_dqm0;          /* +0x5AC */
    uint32_t sw_pad_ctl_pad_dram_sdqs1_p;       /* +0x5B0 */
    uint32_t sw_pad_ctl_pad_dram_dqm1;          /* +0x5B4 */
    uint32_t sw_pad_ctl_pad_dram_sdqs6_p;       /* +0x5B8 */
    uint32_t sw_pad_ctl_pad_dram_dqm6;          /* +0x5BC */
    uint32_t sw_pad_ctl_pad_dram_sdqs7_p;       /* +0x5C0 */
    uint32_t sw_pad_ctl_pad_dram_dqm7;          /* +0x5C4 */
    uint32_t sw_pad_ctl_pad_key_col0;           /* +0x5C8 */
    uint32_t sw_pad_ctl_pad_key_row0;           /* +0x5CC */
    uint32_t sw_pad_ctl_pad_key_col1;           /* +0x5D0 */
    uint32_t sw_pad_ctl_pad_key_row1;           /* +0x5D4 */
    uint32_t sw_pad_ctl_pad_key_col2;           /* +0x5D8 */
    uint32_t sw_pad_ctl_pad_key_row2;           /* +0x5DC */
    uint32_t sw_pad_ctl_pad_key_col3;           /* +0x5E0 */
    uint32_t sw_pad_ctl_pad_key_row3;           /* +0x5E4 */
    uint32_t sw_pad_ctl_pad_key_col4;           /* +0x5E8 */
    uint32_t sw_pad_ctl_pad_key_row4;           /* +0x5EC */
    uint32_t sw_pad_ctl_pad_mux00;             /* +0x5F0 */
    uint32_t sw_pad_ctl_pad_mux01;             /* +0x5F4 */
    uint32_t sw_pad_ctl_pad_mux09;             /* +0x5F8 */
    uint32_t sw_pad_ctl_pad_mux03;             /* +0x5FC */
    uint32_t sw_pad_ctl_pad_mux06;             /* +0x600 */
    uint32_t sw_pad_ctl_pad_mux02;             /* +0x604 */
    uint32_t sw_pad_ctl_pad_mux04;             /* +0x608 */
    uint32_t sw_pad_ctl_pad_mux05;             /* +0x60C */
    uint32_t sw_pad_ctl_pad_mux07;             /* +0x610 */
    uint32_t sw_pad_ctl_pad_mux08;             /* +0x614 */
    uint32_t sw_pad_ctl_pad_mux16;             /* +0x618 */
    uint32_t sw_pad_ctl_pad_mux17;             /* +0x61C */
    uint32_t sw_pad_ctl_pad_mux18;             /* +0x620 */
    uint32_t sw_pad_ctl_pad_mux19;             /* +0x624 */
    uint32_t sw_pad_ctl_pad_csi0_pixclk;        /* +0x628 */
    uint32_t sw_pad_ctl_pad_csi0_hsync;         /* +0x62C */
    uint32_t sw_pad_ctl_pad_csi0_data_en;       /* +0x630 */
    uint32_t sw_pad_ctl_pad_csi0_vsync;         /* +0x634 */
    uint32_t sw_pad_ctl_pad_csi0_data04;        /* +0x638 */
    uint32_t sw_pad_ctl_pad_csi0_data05;        /* +0x63C */
    uint32_t sw_pad_ctl_pad_csi0_data06;        /* +0x640 */
    uint32_t sw_pad_ctl_pad_csi0_data07;        /* +0x644 */
    uint32_t sw_pad_ctl_pad_csi0_data08;        /* +0x648 */
    uint32_t sw_pad_ctl_pad_csi0_data09;        /* +0x64C */
    uint32_t sw_pad_ctl_pad_csi0_data10;        /* +0x650 */
    uint32_t sw_pad_ctl_pad_csi0_data11;        /* +0x654 */
    uint32_t sw_pad_ctl_pad_csi0_data12;        /* +0x658 */
    uint32_t sw_pad_ctl_pad_csi0_data13;        /* +0x65C */
    uint32_t sw_pad_ctl_pad_csi0_data14;        /* +0x660 */
    uint32_t sw_pad_ctl_pad_csi0_data15;        /* +0x664 */
    uint32_t sw_pad_ctl_pad_csi0_data16;        /* +0x668 */
    uint32_t sw_pad_ctl_pad_csi0_data17;        /* +0x66C */
    uint32_t sw_pad_ctl_pad_csi0_data18;        /* +0x670 */
    uint32_t sw_pad_ctl_pad_csi0_data19;        /* +0x674 */
    uint32_t sw_pad_ctl_pad_jtag_tms;           /* +0x678 */
    uint32_t sw_pad_ctl_pad_jtag_mod;           /* +0x67C */
    uint32_t sw_pad_ctl_pad_jtag_trstb;         /* +0x680 */
    uint32_t sw_pad_ctl_pad_jtag_tdi;           /* +0x684 */
    uint32_t sw_pad_ctl_pad_jtag_tck;           /* +0x688 */
    uint32_t sw_pad_ctl_pad_jtag_tdo;           /* +0x68C */
    uint32_t sw_pad_ctl_pad_sd3_data7;          /* +0x690 */
    uint32_t sw_pad_ctl_pad_sd3_data6;          /* +0x694 */
    uint32_t sw_pad_ctl_pad_sd3_data5;          /* +0x698 */
    uint32_t sw_pad_ctl_pad_sd3_data4;          /* +0x69C */
    uint32_t sw_pad_ctl_pad_sd3_cmd;            /* +0x6A0 */
    uint32_t sw_pad_ctl_pad_sd3_clk;            /* +0x6A4 */
    uint32_t sw_pad_ctl_pad_sd3_data0;          /* +0x6A8 */
    uint32_t sw_pad_ctl_pad_sd3_data1;          /* +0x6AC */
    uint32_t sw_pad_ctl_pad_sd3_data2;          /* +0x6B0 */
    uint32_t sw_pad_ctl_pad_sd3_data3;          /* +0x6B4 */
    uint32_t sw_pad_ctl_pad_sd3_reset;          /* +0x6B8 */
    uint32_t sw_pad_ctl_pad_nand_cle;           /* +0x6BC */
    uint32_t sw_pad_ctl_pad_nand_ale;           /* +0x6C0 */
    uint32_t sw_pad_ctl_pad_nand_wp_b;          /* +0x6C4 */
    uint32_t sw_pad_ctl_pad_nand_ready;         /* +0x6C8 */
    uint32_t sw_pad_ctl_pad_nand_cs0_b;         /* +0x6CC */
    uint32_t sw_pad_ctl_pad_nand_cs1_b;         /* +0x6D0 */
    uint32_t sw_pad_ctl_pad_nand_cs2_b;         /* +0x6D4 */
    uint32_t sw_pad_ctl_pad_nand_cs3_b;         /* +0x6D8 */
    uint32_t sw_pad_ctl_pad_sd4_cmd;            /* +0x6DC */
    uint32_t sw_pad_ctl_pad_sd4_clk;            /* +0x6E0 */
    uint32_t sw_pad_ctl_pad_nand_data00;        /* +0x6E4 */
    uint32_t sw_pad_ctl_pad_nand_data01;        /* +0x6E8 */
    uint32_t sw_pad_ctl_pad_nand_data02;        /* +0x6EC */
    uint32_t sw_pad_ctl_pad_nand_data03;        /* +0x6F0 */
    uint32_t sw_pad_ctl_pad_nand_data04;        /* +0x6F4 */
    uint32_t sw_pad_ctl_pad_nand_data05;        /* +0x6F8 */
    uint32_t sw_pad_ctl_pad_nand_data06;        /* +0x6FC */
    uint32_t sw_pad_ctl_pad_nand_data07;        /* +0x700 */
    uint32_t sw_pad_ctl_pad_sd4_data0;          /* +0x704 */
    uint32_t sw_pad_ctl_pad_sd4_data1;          /* +0x708 */
    uint32_t sw_pad_ctl_pad_sd4_data2;          /* +0x70C */
    uint32_t sw_pad_ctl_pad_sd4_data3;          /* +0x710 */
    uint32_t sw_pad_ctl_pad_sd4_data4;          /* +0x714 */
    uint32_t sw_pad_ctl_pad_sd4_data5;          /* +0x718 */
    uint32_t sw_pad_ctl_pad_sd4_data6;          /* +0x71C */
    uint32_t sw_pad_ctl_pad_sd4_data7;          /* +0x720 */
    uint32_t sw_pad_ctl_pad_sd1_data1;          /* +0x724 */
    uint32_t sw_pad_ctl_pad_sd1_data0;          /* +0x728 */
    uint32_t sw_pad_ctl_pad_sd1_data3;          /* +0x72C */
    uint32_t sw_pad_ctl_pad_sd1_cmd;            /* +0x730 */
    uint32_t sw_pad_ctl_pad_sd1_data2;          /* +0x734 */
    uint32_t sw_pad_ctl_pad_sd1_clk;            /* +0x738 */
    uint32_t sw_pad_ctl_pad_sd2_clk;            /* +0x73C */
    uint32_t sw_pad_ctl_pad_sd2_cmd;            /* +0x740 */
    uint32_t sw_pad_ctl_pad_sd2_data3;          /* +0x744 */
    /*** Group ***/
    uint32_t sw_pad_ctl_grp_b7ds;               /* +0x748 */
    uint32_t sw_pad_ctl_grp_addds;              /* +0x74C */
    uint32_t sw_pad_ctl_grp_ddrmode_ctl;        /* +0x750 */
    uint32_t sw_pad_ctl_grp_term_ctl0;          /* +0x754 */
    uint32_t sw_pad_ctl_grp_ddrpke;             /* +0x758 */
    uint32_t sw_pad_ctl_grp_term_ctl1;          /* +0x75C */
    uint32_t sw_pad_ctl_grp_term_ctl2;          /* +0x760 */
    uint32_t sw_pad_ctl_grp_term_ctl3;          /* +0x764 */
    uint32_t sw_pad_ctl_grp_ddrpk;              /* +0x768 */
    uint32_t sw_pad_ctl_grp_term_ctl4;          /* +0x76C */
    uint32_t sw_pad_ctl_grp_ddrhys;             /* +0x770 */
    uint32_t sw_pad_ctl_grp_ddrmode;            /* +0x774 */
    uint32_t sw_pad_ctl_grp_term_ctl5;          /* +0x778 */
    uint32_t sw_pad_ctl_grp_term_ctl6;          /* +0x77C */
    uint32_t sw_pad_ctl_grp_term_ctl7;          /* +0x780 */
    uint32_t sw_pad_ctl_grp_b0ds;               /* +0x784 */
    uint32_t sw_pad_ctl_grp_b1ds;               /* +0x788 */
    uint32_t sw_pad_ctl_grp_ctlds;              /* +0x78C */
    uint32_t sw_pad_ctl_grp_ddr_type_rgmii;     /* +0x790 */
    uint32_t sw_pad_ctl_grp_b2ds;               /* +0x794 */
    uint32_t sw_pad_ctl_grp_ddr_type;           /* +0x798 */
    uint32_t sw_pad_ctl_grp_b3ds;               /* +0x79C */
    uint32_t sw_pad_ctl_grp_b4ds;               /* +0x7A0 */
    uint32_t sw_pad_ctl_grp_b5ds;               /* +0x7A4 */
    uint32_t sw_pad_ctl_grp_b6ds;               /* +0x7A8 */
    uint32_t sw_pad_ctl_grp_rgmii_term;         /* +0x7AC */
    /*** Select Input ***/
    uint32_t asrc_asrck_clock_6_select_input;   /* +0x7B0 */
    uint32_t aud4_input_da_amx_select_input;    /* +0x7B4 */
    uint32_t aud4_input_db_amx_select_input;    /* +0x7B8 */
    uint32_t aud4_input_rxclk_amx_select_input; /* +0x7BC */
    uint32_t aud4_input_rxfs_amx_select_input;  /* +0x7C0 */
    uint32_t aud4_input_txclk_amx_select_input; /* +0x7C4 */
    uint32_t aud4_input_txfs_amx_select_input;  /* +0x7C8 */
    uint32_t aud5_input_da_amx_select_input;    /* +0x7CC */
    uint32_t aud5_input_db_amx_select_input;    /* +0x7D0 */
    uint32_t aud5_input_rxclk_amx_select_input; /* +0x7D4 */
    uint32_t aud5_input_rxfs_amx_select_input;  /* +0x7D8 */
    uint32_t aud5_input_txclk_amx_select_input; /* +0x7DC */
    uint32_t aud5_input_txfs_amx_select_input;  /* +0x7E0 */
    uint32_t flexcan1_rx_select_input;          /* +0x7E4 */
    uint32_t flexcan2_rx_select_input;          /* +0x7E8 */
    uint32_t res2;
    uint32_t ccm_pmic_ready_select_input;       /* +0x7F0 */
    uint32_t ecspi1_cspi_clk_in_select_input;   /* +0x7F4 */
    uint32_t ecspi1_miso_select_input;          /* +0x7F8 */
    uint32_t ecspi1_mosi_select_input;          /* +0x7FC */
    uint32_t ecspi1_ss0_select_input;           /* +0x800 */
    uint32_t ecspi1_ss1_select_input;           /* +0x804 */
    uint32_t ecspi1_ss2_select_input;           /* +0x808 */
    uint32_t ecspi1_ss3_select_input;           /* +0x80C */
    uint32_t ecspi2_cspi_clk_in_select_input;   /* +0x814 */
    uint32_t ecspi2_miso_select_input;          /* +0x810 */
    uint32_t ecspi2_mosi_select_input;          /* +0x818 */
    uint32_t ecspi2_ss0_select_input;           /* +0x81C */
    uint32_t ecspi2_ss1_select_input;           /* +0x820 */
    uint32_t ecspi4_ss0_select_input;           /* +0x824 */
    uint32_t ecspi5_cspi_clk_in_select_input;   /* +0x828 */
    uint32_t ecspi5_miso_select_input;          /* +0x82C */
    uint32_t ecspi5_mosi_select_input;          /* +0x830 */
    uint32_t ecspi5_ss0_select_input;           /* +0x834 */
    uint32_t ecspi5_ss1_select_input;           /* +0x838 */
    uint32_t enet_ref_clk_select_input;         /* +0x83C */
    uint32_t enet_mac0_mdio_select_input;       /* +0x840 */
    uint32_t enet_mac0_rx_clk_select_input;     /* +0x844 */
    uint32_t enet_mac0_rx_data0_select_input;   /* +0x848 */
    uint32_t enet_mac0_rx_data1_select_input;   /* +0x84C */
    uint32_t enet_mac0_rx_data2_select_input;   /* +0x850 */
    uint32_t enet_mac0_rx_data3_select_input;   /* +0x854 */
    uint32_t enet_mac0_rx_en_select_input;      /* +0x858 */
    uint32_t esai_rx_fs_select_input;           /* +0x85C */
    uint32_t esai_tx_fs_select_input;           /* +0x860 */
    uint32_t esai_rx_hf_clk_select_input;       /* +0x864 */
    uint32_t esai_tx_hf_clk_select_input;       /* +0x868 */
    uint32_t esai_rx_clk_select_input;          /* +0x86C */
    uint32_t esai_tx_clk_select_input;          /* +0x870 */
    uint32_t esai_sdo0_select_input;            /* +0x874 */
    uint32_t esai_sdo1_select_input;            /* +0x878 */
    uint32_t esai_sdo2_sdi3_select_input;       /* +0x87C */
    uint32_t esai_sdo3_sdi2_select_input;       /* +0x880 */
    uint32_t esai_sdo4_sdi1_select_input;       /* +0x884 */
    uint32_t esai_sdo5_sdi0_select_input;       /* +0x888 */
    uint32_t hdmi_icecin_select_input;          /* +0x88C */
    uint32_t hdmi_ii2c_clkin_select_input;      /* +0x890 */
    uint32_t hdmi_ii2c_datain_select_input;     /* +0x894 */
    uint32_t i2c1_scl_in_select_input;          /* +0x898 */
    uint32_t i2c1_sda_in_select_input;          /* +0x89C */
    uint32_t i2c2_scl_in_select_input;          /* +0x8A0 */
    uint32_t i2c2_sda_in_select_input;          /* +0x8A4 */
    uint32_t i2c3_scl_in_select_input;          /* +0x8A8 */
    uint32_t i2c3_sda_in_select_input;          /* +0x8AC */
    uint32_t ipu2_sens1_data10_select_input;    /* +0x8B0 */
    uint32_t ipu2_sens1_data11_select_input;    /* +0x8B4 */
    uint32_t ipu2_sens1_data12_select_input;    /* +0x8B8 */
    uint32_t ipu2_sens1_data13_select_input;    /* +0x8BC */
    uint32_t ipu2_sens1_data14_select_input;    /* +0x8C0 */
    uint32_t ipu2_sens1_data15_select_input;    /* +0x8C4 */
    uint32_t ipu2_sens1_data16_select_input;    /* +0x8C8 */
    uint32_t ipu2_sens1_data17_select_input;    /* +0x8CC */
    uint32_t ipu2_sens1_data18_select_input;    /* +0x8D0 */
    uint32_t ipu2_sens1_data19_select_input;    /* +0x8D4 */
    uint32_t ipu2_sens1_data_en_select_input;   /* +0x8D8 */
    uint32_t ipu2_sens1_hsync_select_input;     /* +0x8DC */
    uint32_t ipu2_sens1_pix_clk_select_input;   /* +0x8E0 */
    uint32_t ipu2_sens1_vsync_select_input;     /* +0x8E4 */
    uint32_t key_col5_select_input;             /* +0x8E8 */
    uint32_t key_col6_select_input;             /* +0x8EC */
    uint32_t key_col7_select_input;             /* +0x8F0 */
    uint32_t key_row5_select_input;             /* +0x8F4 */
    uint32_t key_row6_select_input;             /* +0x8F8 */
    uint32_t key_row7_select_input;             /* +0x8FC */
    uint32_t mlb_mlb_clk_in_select_input;       /* +0x900 */
    uint32_t mlb_mlb_data_in_select_input;      /* +0x904 */
    uint32_t mlb_mlb_sig_in_select_input;       /* +0x908 */
    uint32_t sdma_events14_select_input;        /* +0x90C */
    uint32_t sdma_events15_select_input;        /* +0x910 */
    uint32_t spdif_spdif_in1_select_input;      /* +0x914 */
    uint32_t spdif_tx_clk2_select_input;        /* +0x91C */
    uint32_t uart1_uart_rts_b_select_input;     /* +0x918 */
    uint32_t uart1_uart_rx_data_select_input;   /* +0x920 */
    uint32_t uart2_uart_rts_b_select_input;     /* +0x924 */
    uint32_t uart2_uart_rx_data_select_input;   /* +0x928 */
    uint32_t uart3_uart_rts_b_select_input;     /* +0x92C */
    uint32_t uart3_uart_rx_data_select_input;   /* +0x930 */
    uint32_t uart4_uart_rts_b_select_input;     /* +0x934 */
    uint32_t uart4_uart_rx_data_select_input;   /* +0x938 */
    uint32_t uart5_uart_rts_b_select_input;     /* +0x93C */
    uint32_t uart5_uart_rx_data_select_input;   /* +0x940 */
    uint32_t usb_otg_oc_select_input;           /* +0x944 */
    uint32_t usb_h1_oc_select_input;            /* +0x948 */
    uint32_t usdhc1_wp_on_select_input;         /* +0x94C */
};

static struct imx6_mux {
    volatile struct imx6_iomuxc_regs* iomuxc;
} _mux;

static inline struct imx6_mux* get_mux_priv(mux_sys_t* mux) {
    return (struct imx6_mux*)mux->priv;
}

static inline void set_mux_priv(mux_sys_t* mux, struct imx6_mux* imx6_mux)
{
    assert(mux != NULL);
    assert(imx6_mux != NULL);
    mux->priv = imx6_mux;
}


static int
imx6_mux_feature_enable(mux_sys_t* mux, enum mux_feature mux_feature)
{
    struct imx6_mux* m;
    if (mux == NULL || mux->priv == NULL) {
        return -1;
    }
    m = get_mux_priv(mux);

    assert(((int)&m->iomuxc->usdhc1_wp_on_select_input & 0xfff) == 0x94C);
    switch (mux_feature) {
    case MUX_I2C1:
        DMUX("Muxing for I2C1\n");
        m->iomuxc->i2c1_scl_in_select_input  = IOMUXC_IS_DAISY(0x0);
        m->iomuxc->sw_mux_ctl_pad_eim_data21 = IOMUXC_MUXCTL_MODE(6)
                                               | IOMUXC_MUXCTL_FORCE_INPUT;
        m->iomuxc->i2c1_sda_in_select_input  = IOMUXC_IS_DAISY(0x0);
        m->iomuxc->sw_mux_ctl_pad_eim_data28 = IOMUXC_MUXCTL_MODE(1)
                                               | IOMUXC_MUXCTL_FORCE_INPUT;
        return 0;
    case MUX_I2C2:
        DMUX("Muxing for I2C2\n");
        m->iomuxc->i2c2_scl_in_select_input = IOMUXC_IS_DAISY(0x1);
        m->iomuxc->sw_mux_ctl_pad_key_col3  = IOMUXC_MUXCTL_MODE(4)
                                              | IOMUXC_MUXCTL_FORCE_INPUT;
        m->iomuxc->i2c2_sda_in_select_input = IOMUXC_IS_DAISY(0x1);
        m->iomuxc->sw_mux_ctl_pad_key_row3  = IOMUXC_MUXCTL_MODE(4)
                                              | IOMUXC_MUXCTL_FORCE_INPUT;
        return 0;
    case MUX_I2C3:
        DMUX("Muxing for I2C3\n");
        m->iomuxc->i2c3_scl_in_select_input = IOMUXC_IS_DAISY(0x2);
        m->iomuxc->sw_mux_ctl_pad_gpio05    = IOMUXC_MUXCTL_MODE(6)
                                              | IOMUXC_MUXCTL_FORCE_INPUT;
        m->iomuxc->i2c3_sda_in_select_input = IOMUXC_IS_DAISY(0x2);
        m->iomuxc->sw_mux_ctl_pad_gpio16    = IOMUXC_MUXCTL_MODE(6)
                                              | IOMUXC_MUXCTL_FORCE_INPUT;
        return 0;
    case MUX_GPIO0_CLKO1:
        DMUX("Muxing CLKO1 to MUX0\n");
        m->iomuxc->sw_mux_ctl_pad_gpio00    = IOMUXC_MUXCTL_MODE(0);
        return 0;
    default:
        return -1;
    }
}

int
imx6_mux_enable_gpio(mux_sys_t* mux_sys, int gpio_id)
{
    static struct imx6_mux* m;
    volatile uint32_t *reg;
    assert(mux_sys);
    m = (struct imx6_mux*)mux_sys->priv;
    assert(m);
    /* Surely there is a mathematical formula for finding the register to be set? */
    switch (gpio_id) {

    case GPIOID(GPIO_BANK1,  0):
        reg = &m->iomuxc->sw_mux_ctl_pad_gpio00;
        break;
    case GPIOID(GPIO_BANK1,  1):
        reg = &m->iomuxc->sw_mux_ctl_pad_gpio01;
        break;
    case GPIOID(GPIO_BANK1,  2):
        reg = &m->iomuxc->sw_mux_ctl_pad_gpio02;
        break;
    case GPIOID(GPIO_BANK1,  3):
        reg = &m->iomuxc->sw_mux_ctl_pad_gpio03;
        break;
    case GPIOID(GPIO_BANK1,  4):
        reg = &m->iomuxc->sw_mux_ctl_pad_gpio04;
        break;
    case GPIOID(GPIO_BANK1,  5):
        reg = &m->iomuxc->sw_mux_ctl_pad_gpio05;
        break;
    case GPIOID(GPIO_BANK1,  6):
        reg = &m->iomuxc->sw_mux_ctl_pad_gpio06;
        break;
    case GPIOID(GPIO_BANK1,  7):
        reg = &m->iomuxc->sw_mux_ctl_pad_gpio07;
        break;
    case GPIOID(GPIO_BANK1,  8):
        reg = &m->iomuxc->sw_mux_ctl_pad_gpio08;
        break;
    case GPIOID(GPIO_BANK1,  9):
        reg = &m->iomuxc->sw_mux_ctl_pad_gpio09;
        break;

    case GPIOID(GPIO_BANK7, 11):
        reg = &m->iomuxc->sw_mux_ctl_pad_gpio16;
        break;
    case GPIOID(GPIO_BANK7, 12):
        reg = &m->iomuxc->sw_mux_ctl_pad_gpio17;
        break;
    case GPIOID(GPIO_BANK7, 13):
        reg = &m->iomuxc->sw_mux_ctl_pad_gpio18;
        break;

    case GPIOID(GPIO_BANK4,  5):
        reg = &m->iomuxc->sw_mux_ctl_pad_gpio19;
        break;

    case GPIOID(GPIO_BANK2,  0):
        reg = &m->iomuxc->sw_mux_ctl_pad_nand_data00;
        break;
    case GPIOID(GPIO_BANK2,  1):
        reg = &m->iomuxc->sw_mux_ctl_pad_nand_data01;
        break;
    case GPIOID(GPIO_BANK2,  2):
        reg = &m->iomuxc->sw_mux_ctl_pad_nand_data02;
        break;
    case GPIOID(GPIO_BANK2,  3):
        reg = &m->iomuxc->sw_mux_ctl_pad_nand_data03;
        break;
    case GPIOID(GPIO_BANK2,  4):
        reg = &m->iomuxc->sw_mux_ctl_pad_nand_data04;
        break;
    case GPIOID(GPIO_BANK2,  5):
        reg = &m->iomuxc->sw_mux_ctl_pad_nand_data05;
        break;
    case GPIOID(GPIO_BANK2,  6):
        reg = &m->iomuxc->sw_mux_ctl_pad_nand_data06;
        break;
    case GPIOID(GPIO_BANK2,  7):
        reg = &m->iomuxc->sw_mux_ctl_pad_nand_data07;
        break;

    default:
        DMUX("Unable to mux GPIOID 0x%x\n", gpio_id);
        return -1;
    }
    *reg = IOMUXC_MUXCTL_MODE(5);
    return 0;
}

static int
imx6_mux_init_common(mux_sys_t* mux)
{
    set_mux_priv(mux, &_mux);
    mux->feature_enable = &imx6_mux_feature_enable;
    return 0;
}


int
imx6_mux_init(void* iomuxc, mux_sys_t* mux)
{
    if (iomuxc != NULL) {
        _mux.iomuxc = iomuxc;
    }
    return imx6_mux_init_common(mux);
}


int
mux_sys_init(ps_io_ops_t* io_ops, mux_sys_t* mux)
{
    MAP_IF_NULL(io_ops, IMX6_IOMUXC, _mux.iomuxc);
    return imx6_mux_init_common(mux);
}


