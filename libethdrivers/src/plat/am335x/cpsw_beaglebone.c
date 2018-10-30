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

/**
 * \file   cpsw.c
 *
 * \brief  This file contains functions which configure CPSW instance
 */

/*
* Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
*/
/*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#include <ethdrivers/plat/hw/soc_AM335x.h>
#include <ethdrivers/plat/hw/hw_control_AM335x.h>
#include <ethdrivers/plat/hw/hw_types.h>
#include <ethdrivers/plat/beaglebone.h>
#include <ethdrivers/plat/hw/hw_cm_per.h>

/******************************************************************************
**                       INTERNAL MACRO DEFINITIONS
******************************************************************************/
#define CPSW_MII_SEL_MODE                     (0x00u)
#define CPSW_MDIO_SEL_MODE                    (0x00u)
#define LEN_MAC_ADDR                          (0x06u)
#define OFFSET_MAC_ADDR                       (0x30u)

/******************************************************************************
**                          FUNCTION DEFINITIONS
******************************************************************************/
/**
 * \brief   This function selects the CPSW pins for use in RGMII mode.
 *
 * \param   None
 *
 * \return  None.
 *
 */
void CPSWPinMuxSetup(void)
{
    HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MII1_RXERR) =
        CONTROL_CONF_MII1_RXERR_CONF_MII1_RXERR_RXACTIVE | CPSW_MII_SEL_MODE;
    HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MII1_TXEN) =  CPSW_MII_SEL_MODE;
    HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MII1_RXDV) =
        CONTROL_CONF_MII1_RXDV_CONF_MII1_RXDV_RXACTIVE | CPSW_MII_SEL_MODE;
    HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MII1_TXD3) =  CPSW_MII_SEL_MODE;
    HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MII1_TXD2) =  CPSW_MII_SEL_MODE;
    HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MII1_TXD1) =  CPSW_MII_SEL_MODE;
    HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MII1_TXD0) =  CPSW_MII_SEL_MODE;
    HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MII1_TXCLK) =
        CONTROL_CONF_MII1_TXCLK_CONF_MII1_TXCLK_RXACTIVE | CPSW_MII_SEL_MODE;
    HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MII1_RXCLK) =
        CONTROL_CONF_MII1_RXCLK_CONF_MII1_RXCLK_RXACTIVE | CPSW_MII_SEL_MODE;
    HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MII1_RXD3) =
        CONTROL_CONF_MII1_RXD3_CONF_MII1_RXD3_RXACTIVE | CPSW_MII_SEL_MODE;
    HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MII1_RXD2) =
        CONTROL_CONF_MII1_RXD2_CONF_MII1_RXD2_RXACTIVE | CPSW_MII_SEL_MODE;
    HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MII1_RXD1) =
        CONTROL_CONF_MII1_RXD1_CONF_MII1_RXD1_RXACTIVE | CPSW_MII_SEL_MODE;
    HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MII1_RXD0) =
        CONTROL_CONF_MII1_RXD0_CONF_MII1_RXD0_RXACTIVE | CPSW_MII_SEL_MODE;
    HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MDIO_DATA) =
        CONTROL_CONF_MDIO_DATA_CONF_MDIO_DATA_RXACTIVE
        | CONTROL_CONF_MDIO_DATA_CONF_MDIO_DATA_PUTYPESEL
        | CPSW_MDIO_SEL_MODE;
    HWREG(SOC_CONTROL_REGS + CONTROL_CONF_MDIO_CLK) =
        CONTROL_CONF_MDIO_CLK_CONF_MDIO_CLK_PUTYPESEL | CPSW_MDIO_SEL_MODE;
}

/**
 * \brief   Enables CPSW clocks
 *
 * \param   None
 *
 * \return  None.
 */
void CPSWClkEnable(void)
{
    HWREG(SOC_PRCM_REGS + CM_PER_CPGMAC0_CLKCTRL) =
        CM_PER_CPGMAC0_CLKCTRL_MODULEMODE_ENABLE;

    while (0 != (HWREG(SOC_PRCM_REGS + CM_PER_CPGMAC0_CLKCTRL)
                 & CM_PER_CPGMAC0_CLKCTRL_IDLEST));

    HWREG(SOC_PRCM_REGS + CM_PER_CPSW_CLKSTCTRL) =
        CM_PER_CPSW_CLKSTCTRL_CLKTRCTRL_SW_WKUP;

    while (0 == (HWREG(SOC_PRCM_REGS + CM_PER_CPSW_CLKSTCTRL)
                 & CM_PER_CPSW_CLKSTCTRL_CLKACTIVITY_CPSW_125MHZ_GCLK));
}

/**
 * \brief   This function sets the RGMII mode for both ports
 *
 * \param   None
 *
 * \return  None.
 */
void EVMPortMIIModeSelect(void)
{
    /* Select MII, Internal Delay mode */
    HWREG(SOC_CONTROL_REGS + CONTROL_GMII_SEL) = 0x00;
}

/**
 * \brief   This function returns the MAC address for the EVM
 *
 * \param   addrIdx    the MAC address index.
 * \param   macAddr    the Pointer where the MAC address shall be stored
 *     'addrIdx' can be either 0 or 1
 *
 * \return  None.
 */
void EVMMACAddrGet(unsigned int addrIdx, unsigned char *macAddr)
{
    macAddr[0] =  (HWREG(SOC_CONTROL_REGS + CONTROL_MAC_ID_LO(addrIdx))
                   >> 8) & 0xFF;
    macAddr[1] =  (HWREG(SOC_CONTROL_REGS + CONTROL_MAC_ID_LO(addrIdx)))
                  & 0xFF;
    macAddr[2] =  (HWREG(SOC_CONTROL_REGS + CONTROL_MAC_ID_HI(addrIdx))
                   >> 24) & 0xFF;
    macAddr[3] =  (HWREG(SOC_CONTROL_REGS + CONTROL_MAC_ID_HI(addrIdx))
                   >> 16) & 0xFF;
    macAddr[4] =  (HWREG(SOC_CONTROL_REGS + CONTROL_MAC_ID_HI(addrIdx))
                   >> 8) & 0xFF;
    macAddr[5] =  (HWREG(SOC_CONTROL_REGS + CONTROL_MAC_ID_HI(addrIdx)))
                  & 0xFF;
}

/****************************** End Of File *********************************/
