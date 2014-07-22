/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#ifndef _ETHDRIVER_PC99_REGISTERS_H_
#define _ETHDRIVER_PC99_REGISTERS_H_
/* keeps track of where registers are and what their offsets are */

#include <stdint.h>
#include <utils/util.h>

struct CTRL_bits {
    uint32_t FD : 1;
    uint32_t reserved_1 : 1;
    uint32_t GIO_MASTER_DISABLE : 1;
    uint32_t reserved_2 : 1;
    uint32_t resreved_3 : 1;
    uint32_t ASDE : 1;
    uint32_t SLU : 1;
    uint32_t reserved_4 : 1;
    uint32_t SPEED : 2;
    uint32_t reserved_5 : 1;
    uint32_t FRCSPD : 1;
    uint32_t FRCDPLX : 1;
    uint32_t reserved_6 : 7;
    uint32_t ADVD3WUC : 1;
    uint32_t reserved_7 : 5;
    uint32_t RST : 1;
    uint32_t RFCE : 1;
    uint32_t TFCE : 1;
    uint32_t VME : 1;
    uint32_t PHY_RST : 1;
};

compile_time_assert(CTRL_bits_size, sizeof(struct CTRL_bits) == 4);

struct STATUS_bits{
    uint32_t FD : 1;
    uint32_t LU : 1;
    uint32_t reserved_1 : 2;
    uint32_t TXOFF : 1;
    uint32_t reserved_2 : 1;
    uint32_t SPEED : 2;
    uint32_t ASDV : 2;
    uint32_t PHYRA : 1;
    uint32_t reserved_3 : 8;
    uint32_t GIO_MASTER_STATUS : 1;
    uint32_t reserved_4 : 11;
    uint32_t reserved_5 : 1;
};

compile_time_assert(STATUS_bits_size, sizeof(struct STATUS_bits) == 4);

struct EEC_bits {
    uint32_t EE_SK : 1;
    uint32_t EE_CS : 1;
    uint32_t EE_DI : 1;
    uint32_t EE_DO : 1;
    uint32_t FWE : 2;
    uint32_t EE_REQ : 1;
    uint32_t EE_GNT : 1;
    uint32_t EE_PRES : 1;
    uint32_t AUTO_RD : 1;
    uint32_t reserved_1 : 1;
    uint32_t NVSIZE : 4;
    uint32_t NVADDS : 2;
    uint32_t reserved_2 : 1;
    uint32_t reserved_3 : 1;
    uint32_t reserved_4 : 1;
    uint32_t AUDPEN : 1;
    uint32_t reserved_5 : 1;
    uint32_t SEC1VAL : 1;
    uint32_t NVMTYPE : 1;
    uint32_t reserved_6 : 1;
    uint32_t reserved_7 : 1;
    uint32_t reserved_8 : 6;
};

compile_time_assert(EEC_bits_size, sizeof(struct EEC_bits) == 4);

struct EERD_bits {
    uint32_t START : 1;
    uint32_t DONE : 1;
    uint32_t ADDR : 14;
    uint32_t DATA : 16;
};

compile_time_assert(EERD_bits_size, sizeof(struct EERD_bits) == 4);

typedef struct csr_map {
    /* offset 0x00000 */
    union {
        uint32_t CTRL;
        struct CTRL_bits CTRL_bits;
    };
    /* offset 0x00004 */
    uint32_t reserved_1;
    /* offset 0x00008 */
    union {
        uint32_t STATUS;
        struct STATUS_bits STATUS_bits;
    };
    /* offset 0x0000C */
    uint32_t reserved_2;
    /* offset 0x00010 */
    union {
        uint32_t EEC;
        struct EEC_bits EEC_bits;
    };
    /* offset 0x00014 */
    union {
        uint32_t EERD;
        struct EERD_bits EERD_bits;
    };
} csr_map_t;

#define CTRL_REG_OFFSET 0x0
#define CTRL_RESERVED_BITS (BIT(1) | (0b11 << 3) | BIT(7) | BIT(10) | (0b1111111 << 13) | (0b11111 << 21) | BIT(29)) // 17
#define CTRL_FD BIT(0)
#define CTRL_GIO_MASTER_DISABLE BIT(2)
#define CTRL_ASDE BIT(5)
#define CTRL_SLU BIT(6)
#define CTRL_SPEED(x) (((x) & 3) << 8)
#define CTRL_FRCSPD BIT(11)
#define CTRL_FRCDPLX BIT(12)
#define CTRL_ADVD3WUC BIT(20)
#define CTRL_RST BIT(26)
#define CTRL_RFCE BIT(27)
#define CTRL_TFCE BIT(28)
#define CTRL_VME BIT(30)
#define CTRL_PHY_RST BIT(31)

#define STATUS_REG_OFFSET 0x8
#define STATUS_FD BIT(0)
#define STATUS_LU BIT(1)
#define STATUS_TXOFF BIT(4)
#define STATUS_SPEED_MASK (0b11 << 6)
#define STATUS_SPEED_SHIFT (6)
#define STATUS_ASDV_MASK (0b11 << 8)
#define STATUS_ASDV_SHIFT (8)
#define STATUS_PHYRA BIT(10)
#define STATUS_GIO_MASTER_STATUS BIT(19)

#define LEDCTL_REG_OFFSET 0x00E00
#define LEDCTL_RESERVED_BITS (BIT(4) | BIT(12) | BIT(20) | (0b1111111 << 24))
#define LEDCTL_LED0_MODE_BITS(x) (((x) & 15) << 0)
#define LEDCTL_GLOBAL_BLINK_MODE BIT(5)
#define LEDCTL_LED0_IVRT BIT(6)
#define LEDCTL_LED0_BLINK BIT(7)
#define LEDCTL_LED1_MODE_BITS(x) (((x) & 15) << 8)
#define LEDCTL_LED1_BLINK_MODE BIT(13)
#define LEDCTL_LED1_IVRT BIT(14)
#define LEDCTL_LED1_BLINK BIT(15)
#define LEDCTL_LED2_MODE_BITS(x) (((x) & 15) << 16)
#define LEDCTL_LED2_BLINK_MODE BIT(21)
#define LEDCTL_LED2_IVRT BIT(22)
#define LEDCTL_LED2_BLINK BIT(23)

#define IMC_REG_OFFSET 0xD8
#define IMC_RESERVED_BITS (BIT(3) | BIT(5) | BIT(8) | (0b11111 << 10) | BIT(19) | (0b1111111 << 25))
#define IMC_TXDW BIT(0)
#define IMC_TXQE BIT(1)
#define IMC_LSC BIT(2)
#define IMC_RXDMT0 BIT(4)
#define IMC_RXO BIT(6)
#define IMC_RXT0 BIT(7)
#define IMC_MDAC BIT(9)
#define IMC_TXD_LOW BIT(15)
#define IMC_SRPD BIT(16)
#define IMC_ACK BIT(17)
#define IMC_MNG BIT(18)
#define IMC_RXQ0 BIT(20)
#define IMC_RXQ1 BIT(21)
#define IMC_TXQ0 BIT(22)
#define IMC_TXQ1 BIT(23)
#define IMC_OTHER BIT(24)

#define TCTL_REG_OFFSET 0x400
#define TCTL_RESERVED_BITS (BIT(0) | BIT(2) | BIT(31)) 
#define TCTL_EN BIT(1)
#define TCTL_PSP BIT(3)
#define TCTL_CT_BITS(x) (((x) & 0b11111111) << 4)
#define TCTL_COLD_BITS(x) (((x) & 0b1111111111) << 12)
#define TCTL_SWXOFF BIT(22)
#define TCTL_PBE BIT(23)
#define TCTL_RTLC BIT(24)
#define TCTL_UNORTX BIT(25)
#define TCTL_TXDSCMT_BITS(x) (((x) & 3) << 26)
#define TCTL_MULR BIT(28)
#define TCTL_RRTHRESH_BITS(x) (((x) & 3) << 29)

#define TIPG_REG_OFFSET 0x410
#define TIPG_RESERVED_BITS (BIT(30) | BIT(31))
#define TIPG_IPGT_BITS(x) (((x) & 0b1111111111) << 0)
#define TIPG_IPGR1_BITS(x) (((x) & 0b1111111111) << 10)
#define TIPG_IPGR2_BITS(x) (((x) & 0b1111111111) << 20)

#define TXDCTL_REG_OFFSET 0x3828
#define TXDCTL_RESERVED_BITS ((0b11 << 6) | (0b11 << 14) | BIT(23))
#define TXDCTL_BIT_THAT_SHOULD_BE_1 BIT(22)
#define TXDCTL_PTHRESH_BITS(x) (((x) & 0b111111) << 0)
#define TXDCTL_HTHRESH_BITS(x) (((x) & 0b111111) << 8)
#define TXDCTL_WTHRESH_BITS(x) (((x) & 0b111111) << 16)
#define TXDCTL_GRAN BIT(24)
#define TXDCTL_LWTHRESH_BITS(x) (((x) & 0b1111111) << 25)

#define TDBAL_REG_OFFSET 0x3800
#define TDBAH_REG_OFFSET 0x3804
#define TDLEN_REG_OFFSET 0x3808 // TODO: Dafaq? Why is this 32bit reg, so close to 0x3810
#define TDH_REG_OFFSET 0x3810
#define TDT_REG_OFFSET 0x3818

#define GPTC_REG_OFFSET 0x4080 // good packets transmitted count

#define MTA_LENGTH 128
#define MTA_REG_OFFSET 0x5200

#define RDBAL_REG_OFFSET 0x2800
#define RDBAH_REG_OFFSET 0x2804
#define RDLEN_REG_OFFSET 0x2808
#define RDH_REG_OFFSET 0x2810
#define RDT_REG_OFFSET 0x2818

#define RCTL_REG_OFFSET 0x100
#define RCTL_RESERVED_BITS (BIT(0) | BIT(14) | BIT(21) | BIT(24) | BIT(31))
#define RCTL_EN BIT(1)
#define RCTL_SBP BIT(2)
#define RCTL_UPE BIT(3)
#define RCTL_MPE BIT(4)
#define RCTL_LPE BIT(5)
#define RCTL_LBM_BITS(x) (((x) & 0b11) << 6)
#define RCTL_RDMTS_BITS(x) (((x) & 0b11) << 8)
#define RCTL_DTYP_BITS(x) (((x) & 0b11) << 10)
#define RCTL_MO_BITS(x) (((x) & 0b11) << 12)
#define RCTL_BAM BIT(15)
#define RCTL_BSIZE_BITS(x) (((x) & 0b11) << 16)
#define RCTL_VFE BIT(18)
#define RCTL_CFIEN BIT(19)
#define RCTL_CFI BIT(20)
#define RCTL_DPF BIT(22)
#define RCTL_PMCF BIT(23)
#define RCTL_BSEX BIT(25)
#define RCTL_SECRC BIT(26)
#define RCTL_FLXBUF_BITS(x) (((x) & 0b1111) << 27)

#define IMS_REG_OFFSET 0xD0
#define IMS_RESERVED_BITS (BIT(3) | BIT(5) | BIT(8) | (0b11111 << 10) | BIT(19) | (0b1111111 << 25))
#define IMS_TXDW BIT(0)
#define IMS_TXQE BIT(1)
#define IMS_LSC BIT(2)
#define IMS_RXDMT0 BIT(4)
#define IMS_RXO BIT(6)
#define IMS_RXT0 BIT(7)
#define IMS_MDAC BIT(9)
#define IMS_TXD_LOW BIT(15)
#define IMS_SRPD BIT(16)
#define IMS_ACK BIT(17)
#define IMS_MNG BIT(18)
#define IMS_RXQ0 BIT(20)
#define IMS_RXQ1 BIT(21)
#define IMS_TXQ0 BIT(22)
#define IMS_TXQ1 BIT(23)
#define IMS_OTHER BIT(24)

#define ICS_REG_OFFSET 0xC8
#define ICS_RESERVED_BITS (BIT(3) | BIT(5) | BIT(8) | (0b11111 << 10) | BIT(19) | (0b1111111 << 25))
#define ICS_TXDW BIT(0)
#define ICS_TXQE BIT(1)
#define ICS_LSC BIT(2)
#define ICS_RXDMT0 BIT(4)
#define ICS_RXO BIT(6)
#define ICS_RXT0 BIT(7)
#define ICS_MDAC BIT(9)
#define ICS_TXD_LOW BIT(15)
#define ICS_SRPD BIT(16)
#define ICS_ACK BIT(17)
#define ICS_MNG BIT(18)
#define ICS_RXQ0 BIT(20)
#define ICS_RXQ1 BIT(21)
#define ICS_TXQ0 BIT(22)
#define ICS_TXQ1 BIT(23)
#define ICS_OTHER BIT(24)

#define ICR_REG_OFFSET 0xC0
#define ICR_RESERVED_BITS (BIT(3) | BIT(5) | BIT(8) | (0b11111 << 10) | BIT(19) | (0b111111 << 25))
#define ICR_TXDW BIT(0)
#define ICR_TXQE BIT(1)
#define ICR_LSC BIT(2)
#define ICR_RXDMT0 BIT(4)
#define ICR_RXO BIT(6)
#define ICR_RXT0 BIT(7)
#define ICR_MDAC BIT(9)
#define ICR_TXD_LOW BIT(15)
#define ICR_SRPD BIT(16)
#define ICR_ACK BIT(17)
#define ICR_MNG BIT(18)
#define ICR_RXQ0 BIT(20)
#define ICR_RXQ1 BIT(21)
#define ICR_TXQ0 BIT(22)
#define ICR_TXQ1 BIT(23)
#define ICR_OTHER BIT(24)
#define ICR_INT_ASSERTED BIT(31)

#define ITR_REG_OFFSET 0xC4
#define ITR_RESERVED_BITS (0b1111111111111111 << 16)
#define ITR_INTERVAL_BITS(x) (((x) & 0b1111111111111111))

#define MPC_REG_OFFSET 0x4010


#endif
