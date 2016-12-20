/**
 * @file - cpswif.c
 * lwIP Ethernet interface for CPSW port
 *
 */

/**
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/**
 * Copyright (c) 2010 Texas Instruments Incorporated
 *
 * This file is dervied from the "ethernetif.c" skeleton Ethernet network
 * interface driver for lwIP.
 */
#include "../lwiplib.h"
#include <lwip/opt.h>
#include <lwip/def.h>
#include <lwip/mem.h>
#include <lwip/pbuf.h>
#include <lwip/sys.h>
#include <lwip/stats.h>
#include <lwip/snmp.h>
#include <netif/etharp.h>
#include <netif/ppp_oe.h>
#include <lwip/err.h>
#include "cpswif.h"
#include <ethdrivers/helpers.h>
#include <platsupport/io.h>
#include <ethdrivers/raw.h>

/* DriverLib Header Files required for this interface driver. */
#include <sel4platsupport/plat/cpsw.h>
#include <sel4platsupport/plat/mdio.h>
#include <sel4platsupport/plat/phy.h>

#define PORT_1                                   0x0
#define PORT_2                                   0x1
#define PORT_0_MASK                              0x1
#define PORT_1_MASK                              0x2
#define PORT_2_MASK                              0x4
#define HOST_PORT_MASK                           PORT_0_MASK
#define SLAVE_PORT_MASK(slv_port_num)            (1 << slv_port_num)
#define PORT_MASK                                (0x7)
#define INDV_PORT_MASK(slv_port_num)             (1 << slv_port_num)

#define ENTRY_TYPE                               0x30
#define ENTRY_TYPE_IDX                           7
#define ENTRY_FREE                               0

/* MDIO input and output frequencies in Hz */
#define MDIO_FREQ_INPUT                          125000000
#define MDIO_FREQ_OUTPUT                         1000000

#define MAX_TRANSFER_UNIT                        1500
#define PBUF_LEN_MAX                             1520

#define MIN_PKT_LEN                              60

/* Define those to better describe the network interface. */
#define IFNAME0                                  'e'
#define IFNAME1                                  'n'

#define MASK_LOWER_4BITS_BYTE                    (0x0F)
#define MASK UPPER_4BITS_BYTE                    (0xF0)

#define MASK_BROADCAST_ADDR                      (0xFF)
#define MASK_MULTICAST_ADDR                      (0x01)

#define ALE_ENTRY_VLAN                           0x20
#define ALE_ENTRY_VLANUCAST                      0x30
#define ALE_ENTRY_UCAST                          0x10
#define ALE_ENTRY_MCAST                          0xD0
#define ALE_ENTRY_OUI                            (0x80)
#define ALE_ENTRY_ADDR                           (0x10)
#define ALE_ENTRY_VLAN_ADDR                      (0x30)
#define ALE_VLAN_ENTRY_MEMBER_LIST               0
#define ALE_VLAN_ENTRY_FRC_UNTAG_EGR             3
#define ALE_VLAN_ENTRY_MCAST_UNREG               (1)
#define ALE_VLAN_ENTRY_MCAST_REG                 (2)
#define ALE_VLAN_ENTRY_ID                        (3)
#define ALE_VLAN_ID_MASK                         (0x0FFF)
#define ALE_VLAN_ENTRY_ID_BIT0_BIT7              6
#define ALE_VLAN_ENTRY_TYPE_ID_BIT8_BIT11        7
#define ALE_VLAN_ENTRY_TYPE_ID_BIT8_BIT11_ALIGN  (0x08)
#define ALE_VLANUCAST_ENTRY_ID_BIT0_BIT7         6
#define ALE_VLANUCAST_ENTRY_TYPE_ID_BIT8_BIT11   7
#define ALE_UCAST_ENTRY_TYPE                     7
#define ALE_UCAST_TYPE_MASK                      (0xC0)
#define ALE_UCAST_TYPE_SHIFT                     (6)
#define ALE_UCAST_TYPE_PERSISTANT                (0x00)
#define ALE_UCAST_TYPE_UNTOUCHED                 (0x40)
#define ALE_UCAST_TYPE_OUI                       (0x80)
#define ALE_UCAST_TYPE_TOUCHED                   (0xC0)
#define ALE_UCAST_ENTRY_DLR_PORT_BLK_SEC         8
#define ALE_UCAST_ENTRY_DLR_BLK_SEC_MASK         (0x03)
#define ALE_UCAST_ENTRY_PORT_SHIFT               2
#define ALE_MCAST_ENTRY_TYPE_FWD_STATE           7
#define ALE_MCAST_ENTRY_TYPE_FWD_STATE_SHIFT     (6)
#define ALE_MCAST_ENTRY_PORTMASK_SUP             8
#define ALE_MCAST_ENTRY_SUPER_MASK               (0x02)
#define ALE_MCAST_ENTRY_SUPER_SHIFT              (1)
#define ALE_MCAST_ENTRY_PORT_MASK                (0x1C)
#define ALE_MCAST_ENTRY_PORTMASK_SHIFT           2

#define SELECT_10_HALF                          (1 << 0)
#define SELECT_10_FULL                          (1 << 1)
#define SELECT_100_HALF                         (1 << 2)
#define SELECT_100_FULL                         (1 << 3)
#define SELECT_1000_HALF                        (1 << 4)
#define SELECT_1000_FULL                        (1 << 5)

#define SELECT_SPEED_10                         (0)
#define SELECT_SPEED_100                        (1)
#define SELECT_SPEED_1000                       (2)

#define SELECT_FORCED                           (0)
#define SELECT_AUTONEG                          (1)
#define SELECT_BOTH                             (2)

#define SELECT_HALF_DUPLEX                      (0)
#define SELECT_FULL_DUPLEX                      (1)

static void delay(uint32_t ms)
{
    volatile int i;
    for (; ms > 0; ms--) {
        for (i = 0; i < 1000000; i++) {
        }
    }
}

/**
* Function to setup the instance parameters inside the interface
* @param  driver   ethernet driver data structure
* @return None.
*/
static void
cpswif_inst_config(struct eth_driver *driver)
{
    struct beaglebone_eth_data *eth_data = (struct beaglebone_eth_data*)driver->eth_data;
    struct cpswinst *cpswinst = ((struct beaglebone_eth_data *) driver->eth_data)->cpswinst;

    /**
     * Code is added for only instance 0. If more instances
     * are there, assign base addresses and phy info here
     */
    cpswinst->ss_base = eth_data->iomm_address.eth_mmio_cpsw_reg;
    cpswinst->mdio_base = eth_data->iomm_address.eth_mmio_cpsw_reg + 0x1000;
    cpswinst->wrpr_base = eth_data->iomm_address.eth_mmio_cpsw_reg + 0x1200;
    cpswinst->cpdma_base = eth_data->iomm_address.eth_mmio_cpsw_reg + 0x800;
    cpswinst->ale_base = eth_data->iomm_address.eth_mmio_cpsw_reg + 0xD00;
    cpswinst->cppi_ram_base = eth_data->iomm_address.eth_mmio_cpsw_reg + 0x2000;
    cpswinst->host_port_base = eth_data->iomm_address.eth_mmio_cpsw_reg + 0x100;
    cpswinst->port[PORT_1].port_base = eth_data->iomm_address.eth_mmio_cpsw_reg + 0x200;
    cpswinst->port[PORT_1].sliver_base = eth_data->iomm_address.eth_mmio_cpsw_reg + 0xD80;
    cpswinst->port[PORT_2].port_base =  eth_data->iomm_address.eth_mmio_cpsw_reg + 0x300;
    cpswinst->port[PORT_2].sliver_base = eth_data->iomm_address.eth_mmio_cpsw_reg + 0xDC0;
}

/**
 * Gives the index of the ALE entry which is free
 * @param  cpswinst  The CPSW instance structure pointer
 *
 * @return index of the ALE entry which is free
 *         ERR_VAL if entry not found
 */
static err_t
cpswif_ale_entry_match_free(struct cpswinst *cpswinst)
{
    u32_t ale_entry[ALE_ENTRY_NUM_WORDS];
    s32_t idx;

    /* Check which ALE entry is free starting from 0th entry */
    for (idx = 0; idx < MAX_ALE_ENTRIES; idx++) {
        CPSWALETableEntryGet(cpswinst->ale_base, idx, ale_entry);

        /* Break if the table entry is free */
        if (((*(((u8_t *)ale_entry) + ENTRY_TYPE_IDX))
                & ENTRY_TYPE) == ENTRY_FREE) {
            return idx;
        }
    }

    return ERR_VAL;
}

/**
 * Sets a unicast entry in the ALE table.
 * @param cpswinst   The CPSW instance structure pointer
 * @param port_num   The slave port number
 * @param eth_addr   Ethernet address
 *
 * @return None
 */
static void
cpswif_ale_unicastentry_set(struct cpswinst *cpswinst, u32_t port_num,
                            u8_t *eth_addr)
{
    volatile u32_t cnt;
    volatile s32_t idx;
    u32_t ale_entry[ALE_ENTRY_NUM_WORDS] = {0, 0, 0};

    for (cnt = 0; cnt < ETHARP_HWADDR_LEN; cnt++) {
        *(((u8_t *)ale_entry) + cnt) = eth_addr[ETHARP_HWADDR_LEN - cnt - 1];
    }

    *(((u8_t *)ale_entry) + ALE_UCAST_ENTRY_TYPE) = ALE_ENTRY_UCAST;
    *(((u8_t *)ale_entry) + ALE_UCAST_ENTRY_DLR_PORT_BLK_SEC) =
        (port_num << ALE_UCAST_ENTRY_PORT_SHIFT);

    idx = cpswif_ale_entry_match_free(cpswinst);

    if (idx < MAX_ALE_ENTRIES ) {
        CPSWALETableEntrySet(cpswinst->ale_base, idx, ale_entry);
    }
}

/**
 * Sets a multicast entry in the ALE table
 * @param cpswinst   The CPSW instance structure pointer
 * @param portmask   The port mask for the port number
 * @param eth_addr   Ethernet Address
 *
 * @return index of the ALE entry added
 *         ERR_VAL if table entry is not free
 */
static void
cpswif_ale_multicastentry_set(struct cpswinst *cpswinst, u32_t portmask,
                              u8_t *eth_addr)
{
    volatile u32_t cnt;
    volatile s32_t idx;
    u32_t ale_entry[ALE_ENTRY_NUM_WORDS] = {0, 0, 0};

    idx = cpswif_ale_entry_match_free(cpswinst);
    if (idx < MAX_ALE_ENTRIES ) {
        for (cnt = 0; cnt < ETHARP_HWADDR_LEN; cnt++) {
            *(((u8_t *)ale_entry) + cnt) = eth_addr[ETHARP_HWADDR_LEN - cnt - 1];
        }

        *(((u8_t *)ale_entry) + ALE_MCAST_ENTRY_TYPE_FWD_STATE) = ALE_ENTRY_MCAST;
        *(((u8_t *)ale_entry) + ALE_MCAST_ENTRY_PORTMASK_SUP) |=
            (portmask << ALE_MCAST_ENTRY_PORTMASK_SHIFT);

        CPSWALETableEntrySet(cpswinst->ale_base, idx, ale_entry);
    }
}

/**
 * AutoNegotiates with phy for link, set it in silver and check for link status.
 * @param  cpswinst   The CPSW instance structure pointer
 * @param  port_num    The slave port number
 * @param  adv         Configuration for advertisement
 *                     SELECT_10_HALF - 10Base Half Duplex
 *                     SELECT_10_FULL - 10Base Full Duplex
 *                     SELECT_100_HALF - 100Base Half Duplex
 *                     SELECT_100_FULL - 100Base Full Duplex
 *                     SELECT_1000_HALF - 1000Base Half Duplex
 *                     SELECT_1000_FULL - 1000Base Full Duplex
 * @return ERR_OK      If link set up is successful
 *                     others if not successful
 */
static err_t
cpswif_phy_autoneg(struct cpswinst *cpswinst, u32_t port_num, u32_t adv)
{
    err_t linkstat = ERR_CONN;
    u16_t adv_val = 0, partnr_ablty = 0, gbps_partnr_ablty = 0, gig_adv_val = 0;
    u32_t aut_neg_cnt = 200, auto_stat, transfer_mode = 0;

    /* Check if ethernet PHY is present or not */
    if (0 == (MDIOPhyAliveStatusGet(cpswinst->mdio_base)
              & (1 << cpswinst->port[port_num - 1].phy_addr))) {
        LWIP_PRINTF("\n\rNo PHY found at addr %d for Port %d of Instance %d.",
                    cpswinst->port[port_num - 1].phy_addr,
                    port_num, 0);
        return linkstat;
    }

    LWIP_PRINTF("\n\rPHY found at address %d for  Port %d of Instance %d.",
                cpswinst->port[port_num - 1].phy_addr,
                port_num, 0);

    if (SELECT_1000_HALF == adv) {
        LWIP_PRINTF("\n\rCPSW doesnot support Half Duplex for Gigabyte...");
        return linkstat;
    }

    /* We advertise for 10/100 Mbps both half and full duplex */
    if (adv & SELECT_10_HALF) {
        adv_val |= PHY_10BT;
    }
    if (adv & SELECT_10_FULL) {
        adv_val |= PHY_10BT_FD;
    }
    if (adv & SELECT_100_HALF) {
        adv_val |= PHY_100BTX;
    }
    if (adv & SELECT_100_FULL) {
        adv_val |= PHY_100BTX_FD;
    }

    gig_adv_val = 0;
    partnr_ablty = TRUE;
    gbps_partnr_ablty = FALSE;

    /**
     * Not all the PHYs can operate at 1000 Mbps. So advertise only
     * if the PHY is capable
     */
    if (cpswinst->port[port_num - 1].phy_gbps) {
        LWIP_PRINTF("\n\rPhy supports Gigabyte...");
        if (adv & SELECT_1000_FULL) {
            gig_adv_val = PHY_1000BT_FD;
            partnr_ablty = TRUE;
            gbps_partnr_ablty = TRUE;
        }
        if (adv & SELECT_1000_HALF) {
            LWIP_PRINTF("\n\rCPSW doesnot support Half Duplex for Gigabyte...");
        }
    } else {
        LWIP_PRINTF("\n\rPhy doesnot support Gigabyte...");
    }

    LWIP_PRINTF("\n\rPerforming Auto-Negotiation...");

    /**
     * Now start Autonegotiation. PHY will talk to its partner
     * and give us what the partner can handle
     */
    if (PhyAutoNegotiate(cpswinst->mdio_base,
                         cpswinst->port[port_num - 1].phy_addr,
                         &adv_val, &gig_adv_val) == TRUE) {
        while (aut_neg_cnt) {
            delay(50);
            auto_stat = PhyAutoNegStatusGet(cpswinst->mdio_base,
                                            cpswinst->port[port_num - 1].phy_addr);
            if (TRUE == auto_stat) {
                break;
            }
            aut_neg_cnt--;
        }

        if (0 != aut_neg_cnt) {
            linkstat = ERR_OK;
            LWIP_PRINTF("\n\rAuto-Negotiation Successful.");
        } else {
            LWIP_PRINTF("\n\rAuto-Negotiation Not Successful.");
            return ERR_CONN;
        }

        /* Get what the partner supports */
        PhyPartnerAbilityGet(cpswinst->mdio_base,
                             cpswinst->port[port_num - 1].phy_addr,
                             &partnr_ablty, &gbps_partnr_ablty);
        if (gbps_partnr_ablty & PHY_LINK_PARTNER_1000BT_FD) {
            LWIP_PRINTF("\n\rTransfer Mode : 1000 Mbps.");
            transfer_mode = CPSW_SLIVER_GIG_FULL_DUPLEX;
        } else {
            if ((adv_val & partnr_ablty) & PHY_100BTX_FD) {
                LWIP_PRINTF("\n\rTransfer Mode : 100 Mbps Full Duplex.");
                transfer_mode = CPSW_SLIVER_NON_GIG_FULL_DUPLEX;
            } else if ((adv_val & partnr_ablty) & PHY_100BTX) {
                LWIP_PRINTF("\n\rTransfer Mode : 100 Mbps Half Duplex.");
                transfer_mode = CPSW_SLIVER_NON_GIG_HALF_DUPLEX;
            } else if ((adv_val & partnr_ablty) & PHY_10BT_FD) {
                LWIP_PRINTF("\n\rTransfer Mode : 10 Mbps Full Duplex.");
                transfer_mode = CPSW_SLIVER_INBAND | CPSW_SLIVER_NON_GIG_FULL_DUPLEX;
            } else if ((adv_val & partnr_ablty) & PHY_10BT) {
                LWIP_PRINTF("\n\rTransfer Mode : 10 Mbps Half Duplex.");
                transfer_mode = CPSW_SLIVER_INBAND | CPSW_SLIVER_NON_GIG_HALF_DUPLEX;
            } else {
                LWIP_PRINTF("\n\rNo Valid Transfer Mode is detected.");
            }
        }
    } else {
        LWIP_PRINTF("\n\rAuto-Negotiation Not Successful.");
        linkstat = ERR_CONN;
    }

    /**
     * Set the Sliver with the negotiation results if autonegotiation
     * is successful
     */
    if (linkstat == ERR_OK) {
        CPSWSlTransferModeSet(cpswinst->port[port_num - 1].sliver_base,
                              transfer_mode);
    }

    /* Check if PHY link is there or not */
    if (FALSE == ((PhyLinkStatusGet(cpswinst->mdio_base,
                                    cpswinst->port[port_num - 1].phy_addr, 1000)))) {
        LWIP_PRINTF("\n\rPHY link connectivity failed for Port %d of Inst %d.",
                    port_num, 0);
        return linkstat;
    }

    LWIP_PRINTF("\n\rPHY link verified for Port %d of Instance %d.",
                port_num, 0);

    CPSWSlRGMIIEnable(
        cpswinst->port[port_num - 1].sliver_base);

    return linkstat;
}

/**
 * Manually configure phy, set it in silver and check for link status.
 * @param  cpswinst   The CPSW instance structure pointer
 * @param  port_num    The slave port number
 * @param  speed       Configuration for speed
 *                     SELECT_SPEED_1000 - 1000 Mbps
 *                     SELECT_SPEED_100 - 100 Mbps
 *                     SELECT_SPEED_10 - 10 Mbps
 * @param  duplex      Configuration for duplex
 *                     SELECT_HALF_DUPLEX - Half Duplex
 *                     SELECT_FULL_DUPLEX - Full Duplex
 * @return ERR_OK      If link set up is successful
 *                     others if not successful
 */
static err_t
cpswif_phy_forced(struct cpswinst *cpswinst, u32_t port_num, u32_t speed,
                  u32_t duplex)
{
    err_t linkstat = ERR_CONN;
    u16_t speed_val = 0, duplex_val = 0;
    u32_t frc_stat_cnt = 200, frc_stat = FALSE, transfer_mode = 0;

    /* Check if ethernet PHY is present or not */
    if (0 == (MDIOPhyAliveStatusGet(cpswinst->mdio_base)
              & (1 << cpswinst->port[port_num - 1].phy_addr))) {
        LWIP_PRINTF("\n\rNo PHY found at addr %d for Port %d of Instance %d.",
                    cpswinst->port[port_num - 1].phy_addr,
                    port_num, 0);
        return linkstat;
    }

    LWIP_PRINTF("\n\rPHY found at address %d for  Port %d of Instance %d.",
                cpswinst->port[port_num - 1].phy_addr,
                port_num, 0);

    /* configure control for speed and duples */
    if (SELECT_SPEED_1000 == speed) {
        speed_val = PHY_SPEED_1000MBPS;
    } else if (SELECT_SPEED_100 == speed) {
        speed_val = PHY_SPEED_100MBPS;
    }

    if (TRUE == duplex) {
        duplex_val = PHY_FULL_DUPLEX;
    }

    if (SELECT_SPEED_1000 == speed) {
        LWIP_PRINTF("\n\rManual Configuration not allowed for Gigabyte...");
        return linkstat;
    }

    if (FALSE == PhyReset(cpswinst->mdio_base,
                          cpswinst->port[port_num - 1].phy_addr)) {
        LWIP_PRINTF("\n\rPHY Reset Failed...");
        return linkstat;
    }

    if (TRUE == (PhyLinkStatusGet(cpswinst->mdio_base,
                                  cpswinst->port[port_num - 1].phy_addr, 1000))) {
        while (frc_stat_cnt) {
            delay(50);
            /* Check if PHY link is there or not */
            frc_stat = (PhyLinkStatusGet(cpswinst->mdio_base,
                                         cpswinst->port[port_num - 1].phy_addr, 1000));

            if (TRUE == frc_stat) {
                LWIP_PRINTF("\n\rPHY Link is Down.");
                break;
            }
            frc_stat_cnt--;
        }
    }

    LWIP_PRINTF("\n\rPerforming Manual Configuration...");

    frc_stat_cnt = 200;
    frc_stat = FALSE;

    if (PhyConfigure(cpswinst->mdio_base, cpswinst->port[port_num - 1].phy_addr,
                     speed_val, duplex_val)) {
        while (frc_stat_cnt) {
            delay(50);
            frc_stat = PhyLinkStatusGet(cpswinst->mdio_base,
                                        cpswinst->port[port_num - 1].phy_addr, 1000);

            if (1 == frc_stat) {
                break;
            }
            frc_stat_cnt--;
        }

        if (0 != frc_stat_cnt) {
            linkstat = ERR_OK;
            LWIP_PRINTF("\n\rPhy Configuration Successful.");
            LWIP_PRINTF("\n\rPHY link verified for Port %d of Instance %d.",
                        port_num, 0);
        } else {
            LWIP_PRINTF("\n\rPhy Configuration Successful.");
            LWIP_PRINTF("\n\rPHY link connectivity failed for Port %d of Inst %d.",
                        port_num, 0);
            return ERR_CONN;
        }

        if (SELECT_SPEED_1000 == speed) {
            LWIP_PRINTF("\n\rTransfer Mode : 1000 Mbps.");
            transfer_mode = CPSW_SLIVER_GIG_FULL_DUPLEX;
        } else {
            if (SELECT_SPEED_10 == speed) {
                LWIP_PRINTF("\n\rTransfer Mode : 10 Mbps ");
                transfer_mode = CPSW_SLIVER_INBAND;
            } else {
                LWIP_PRINTF("\n\rTransfer Mode : 100 Mbps ");
            }
            if (TRUE == duplex) {
                LWIP_PRINTF("Full Duplex.");
                transfer_mode |= CPSW_SLIVER_NON_GIG_FULL_DUPLEX;
            } else {
                LWIP_PRINTF("Half Duplex.");
                transfer_mode |= CPSW_SLIVER_NON_GIG_HALF_DUPLEX;
            }
        }
    } else {
        LWIP_PRINTF("\n\rPhy Configuration Not Successful.");
        LWIP_PRINTF("\n\rPHY link connectivity failed for Port %d of Inst %d.",
                    port_num, 0);
        linkstat = ERR_CONN;
    }

    /**
     * Set the Sliver with the forced phy configuration
     */
    CPSWSlTransferModeSet(cpswinst->port[port_num - 1].sliver_base,
                          transfer_mode);

    CPSWSlRGMIIEnable(cpswinst->port[port_num - 1].sliver_base);

    return linkstat;
}

/**
* Function to setup the link. AutoNegotiates with the phy for link
* setup and set the CPSW with the result of autonegotiation.
* @param  driver   ethernet driver data structure
* @param  cpswportif  The cpsw port interface structure pointer
* @return ERR_OK      If link set up is successful
*                     others if not successful
*/
static err_t
cpswif_autoneg_config(struct eth_driver *driver, u32_t inst_num, u32_t port_num)
{
    struct cpswinst *cpswinst = ((struct beaglebone_eth_data*)driver->eth_data)->cpswinst;
    err_t linkstat = ERR_CONN;
    u16_t adv_val, partnr_ablty, gbps_partnr_ablty, gig_adv_val;
    u32_t aut_neg_cnt = 200, auto_stat, transfer_mode = 0;

    /* We advertise for 10/100 Mbps both half and full duplex */
    adv_val = (PHY_100BTX | PHY_100BTX_FD | PHY_10BT | PHY_10BT_FD);

    /**
     * Not all the PHYs can operate at 1000 Mbps. So advertise only
     * if the PHY is capable
     */
    if (TRUE == cpswinst->port[port_num - 1].phy_gbps) {
        gig_adv_val = PHY_1000BT_FD;
        partnr_ablty = TRUE;
        gbps_partnr_ablty = TRUE;
    } else {
        gig_adv_val = 0;
        partnr_ablty = TRUE;
        gbps_partnr_ablty = FALSE;
    }

    LWIP_PRINTF("\n\rPerforming Auto-Negotiation...");

    /**
     * Now start Autonegotiation. PHY will talk to its partner
     * and give us what the partner can handle
     */
    if (PhyAutoNegotiate(cpswinst->mdio_base,
                         cpswinst->port[port_num - 1].phy_addr,
                         &adv_val, &gig_adv_val) == TRUE) {
        while (aut_neg_cnt) {
            delay(50);
            auto_stat = PhyAutoNegStatusGet(cpswinst->mdio_base,
                                            cpswinst->port[port_num - 1].phy_addr);
            if (TRUE == auto_stat) {
                break;
            }
            aut_neg_cnt--;
        }

        if (0 != aut_neg_cnt) {
            linkstat = ERR_OK;
            LWIP_PRINTF("\n\rAuto-Negotiation Successful.");
        } else {
            LWIP_PRINTF("\n\rAuto-Negotiation Not Successful.");
            return ERR_CONN;
        }

        /* Get what the partner supports */
        PhyPartnerAbilityGet(cpswinst->mdio_base,
                             cpswinst->port[port_num - 1].phy_addr,
                             &partnr_ablty, &gbps_partnr_ablty);
        if (gbps_partnr_ablty & PHY_LINK_PARTNER_1000BT_FD) {
            LWIP_PRINTF("\n\rTransfer Mode : 1000 Mbps.");
            transfer_mode = CPSW_SLIVER_GIG_FULL_DUPLEX;
        } else {
            if ((adv_val & partnr_ablty) & PHY_100BTX_FD) {
                LWIP_PRINTF("\n\rTransfer Mode : 100 Mbps Full Duplex.");
                transfer_mode = CPSW_SLIVER_NON_GIG_FULL_DUPLEX;
            } else if ((adv_val & partnr_ablty) & PHY_100BTX) {
                LWIP_PRINTF("\n\rTransfer Mode : 100 Mbps Half Duplex.");
                transfer_mode = CPSW_SLIVER_NON_GIG_HALF_DUPLEX;
            } else if ((adv_val & partnr_ablty) & PHY_10BT_FD) {
                LWIP_PRINTF("\n\rTransfer Mode : 10 Mbps Full Duplex.");
                transfer_mode = CPSW_SLIVER_INBAND | CPSW_SLIVER_NON_GIG_FULL_DUPLEX;
            } else if ((adv_val & partnr_ablty) & PHY_10BT) {
                LWIP_PRINTF("\n\rTransfer Mode : 10 Mbps Half Duplex.");
                transfer_mode = CPSW_SLIVER_INBAND | CPSW_SLIVER_NON_GIG_HALF_DUPLEX;
            } else {
                LWIP_PRINTF("\n\rNo Valid Transfer Mode is detected.");
            }
        }
    } else {
        LWIP_PRINTF("\n\rAuto-Negotiation Not Successful.");
        linkstat = ERR_CONN;
    }

    /**
     * Set the Sliver with the negotiation results if autonegotiation
     * is successful
     */
    if (linkstat == ERR_OK) {
        CPSWSlTransferModeSet(cpswinst->port[port_num - 1].sliver_base,
                              transfer_mode);
    }

    return linkstat;
}

/**
 * Configures PHY link for a port
 * @param  driver   ethernet driver data structure
 * @param cpswif  The CPSW interface structure pointer
 * @param slv_port_num  The slave port number
 *
 * @return ERR_OK    if link configurations are successful
 *                   an error status if failed
 */
static err_t
cpswif_phylink_config(struct eth_driver *driver, struct cpswportif * cpswif, u32_t slv_port_num)
{
    struct cpswinst *cpswinst = ((struct beaglebone_eth_data*)driver->eth_data)->cpswinst;
    err_t err;

    /* Check if ethernet PHY is present or not */
    if (0 == (MDIOPhyAliveStatusGet(cpswinst->mdio_base)
              & (1 << cpswinst->port[slv_port_num - 1].phy_addr))) {
        LWIP_PRINTF("\n\rNo PHY found at address %d for  Port %d of Instance %d.",
                    cpswinst->port[slv_port_num - 1].phy_addr, slv_port_num,
                    cpswif->inst_num);
        return ERR_CONN;
    }

    LWIP_PRINTF("\n\rPHY found at address %d for  Port %d of Instance %d.",
                cpswinst->port[slv_port_num - 1].phy_addr, slv_port_num,
                cpswif->inst_num);

    /**
     * PHY is alive. So autonegotiate and get the speed and duplex
     * parameters, set it in the sliver
     */
    err = (err_t)(cpswif_autoneg_config(driver, cpswif->inst_num, slv_port_num));

    /* Check if PHY link is there or not */
    if (FALSE == ((PhyLinkStatusGet(cpswinst->mdio_base,
                                    cpswinst->port[slv_port_num - 1].phy_addr, 1000)))) {
        LWIP_PRINTF("\n\rPHY link connectivity failed for Port %d of Instance %d.",
                    slv_port_num, cpswif->inst_num);
        return ERR_CONN;
    }

    LWIP_PRINTF("\n\rPHY link verified for Port %d of Instance %d.",
                slv_port_num, cpswif->inst_num);

    CPSWSlRGMIIEnable(cpswinst->port[slv_port_num - 1].sliver_base);

    return err;
}

/**
 * Initializes the CPSW port
 * @param driver   ethernet driver data structure
 *
 * @return ERR_OK    if port initialization is successful
 *                   an error status if failed
 */
static err_t
cpswif_port_init(struct eth_driver *driver)
{
    struct beaglebone_eth_data *eth_data = (struct beaglebone_eth_data*)driver->eth_data;

    struct cpswportif *cpswif = (struct cpswportif*) eth_data->cpswPortIf;

    /* We only use one interface, comment out the following line if you need another */
    //err = err & (cpswif_phylink_config(cpswif, 2));

    return cpswif_phylink_config(driver, cpswif, 1);
}

/**
 * This function intializes the CPDMA.
 * The CPPI RAM will be initialized for transmit and receive
 * buffer descriptor rings.
 *
 * @param  driver   ethernet driver data structure
 * @return None
 */
static void
cpswif_cpdma_init(struct eth_driver *driver)
{
    struct beaglebone_eth_data *eth_data = (struct beaglebone_eth_data*)driver->eth_data;
    struct cpswinst *cpswinst = eth_data->cpswinst;
    CPSWCPDMARxHdrDescPtrWrite(cpswinst->cpdma_base, ((struct descriptor *) eth_data->rx_ring_phys), 0);
}

/**
 * In this function, the hardware should be initialized.
 * Called from cpswif_init().
 *
 * @param driver   ethernet driver data structure
 * @return None
 */
static void
cpswif_inst_init(struct eth_driver *driver)
{

    struct beaglebone_eth_data *eth_data = (struct beaglebone_eth_data*)driver->eth_data;
    struct cpswportif *cpswif = (struct cpswportif*) eth_data->cpswPortIf;
    u32_t inst_num = cpswif->inst_num;

    struct cpswinst *cpswinst = eth_data->cpswinst;

    /* Reset the different modules */
    CPSWSSReset(cpswinst->ss_base);
    CPSWWrReset(cpswinst->wrpr_base);
    CPSWSlReset(cpswinst->port[PORT_1].sliver_base);
    CPSWSlReset(cpswinst->port[PORT_2].sliver_base);

    __atomic_thread_fence(__ATOMIC_ACQ_REL);

    CPSWCPDMAReset(cpswinst->cpdma_base);

    /* Initialize MDIO */
    MDIOInit(cpswinst->mdio_base, MDIO_FREQ_INPUT, MDIO_FREQ_OUTPUT);
    delay(1);

    __atomic_thread_fence(__ATOMIC_ACQ_REL);

    CPSWALEInit(cpswinst->ale_base);

    /* Set the port 0, 1 and 2 states to FORWARD */
    CPSWALEPortStateSet(cpswinst->ale_base, 0, CPSW_ALE_PORT_STATE_FWD);
    CPSWALEPortStateSet(cpswinst->ale_base, 1, CPSW_ALE_PORT_STATE_FWD);
    CPSWALEPortStateSet(cpswinst->ale_base, 2, CPSW_ALE_PORT_STATE_FWD);

    __atomic_thread_fence(__ATOMIC_ACQ_REL);

    /* For normal CPSW switch mode, set multicast entry. */
    u8_t bcast_addr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    cpswif_ale_multicastentry_set(cpswinst,
                                  PORT_0_MASK | PORT_1_MASK | PORT_2_MASK,
                                  bcast_addr);
    cpswif_ale_unicastentry_set(cpswinst, 0,
                                (u8_t *)(&(cpswif->eth_addr)));

    /* Set the ethernet address for both the ports */
    CPSWPortSrcAddrSet(cpswinst->port[0].port_base,
                       (u8_t *)(&(cpswif->eth_addr)));
    CPSWPortSrcAddrSet(cpswinst->port[1].port_base,
                       (u8_t *)(&(cpswif->eth_addr)));

    /* Enable the statistics. Lets see in case we come across any issues */
    CPSWStatisticsEnable(cpswinst->ss_base);

    /* Initialize the buffer descriptors for CPDMA */
    cpswif_cpdma_init(driver);

    __atomic_thread_fence(__ATOMIC_ACQ_REL);

    /* Acknowledge receive and transmit interrupts for proper interrupt pulsing*/
    CPSWCPDMAEndOfIntVectorWrite(cpswinst->cpdma_base, CPSW_EOI_TX_PULSE);
    CPSWCPDMAEndOfIntVectorWrite(cpswinst->cpdma_base, CPSW_EOI_RX_PULSE);

    /* Enable the transmission and reception */
    CPSWCPDMATxEnable(cpswinst->cpdma_base);
    CPSWCPDMARxEnable(cpswinst->cpdma_base);

    /* Enable the interrupts for channel 0 and for control core 0 */
    CPSWCPDMATxIntEnable(cpswinst->cpdma_base, 0);
    CPSWWrCoreIntEnable(cpswinst->wrpr_base, 0, 0, CPSW_CORE_INT_TX_PULSE);

    CPSWCPDMARxIntEnable(cpswinst->cpdma_base, 0);
    CPSWWrCoreIntEnable(cpswinst->wrpr_base, 0, 0, CPSW_CORE_INT_RX_PULSE);

    __atomic_thread_fence(__ATOMIC_ACQ_REL);
}

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the functions cpswif_inst_init() and
 * cpswif_port_init() to do low level initializations
 *
 * @param driver   ethernet driver data structure
 * @return ERR_OK   If the interface is initialized
 *                  any other err_t on error
 */
err_t
cpswif_init(struct eth_driver *driver)
{
    static u32_t inst_init_flag = 0;

    struct beaglebone_eth_data *eth_data = (struct beaglebone_eth_data*)driver->eth_data;

    /* We only use one instance */
    u32_t inst_num = 0;

    /**
     * Initialize an instance only once. Port initialization will be
     * done separately.
     */
    if (((inst_init_flag >> inst_num) & 0x01) == 0) {
        cpswif_inst_config(driver);
        cpswif_inst_init(driver);
        inst_init_flag |= (1 << inst_num);
    }

    if (cpswif_port_init(driver) != ERR_OK) {
        return ERR_CONN;
    }

    return ERR_OK;
}

/**
 * Gets the netif status
 *
 * @param   netif   The netif whoes status to be checked
 * @return  The netif status
 */
u32_t
cpswif_netif_status(struct netif *netif)
{
    return ((u32_t)(netif_is_up(netif)));
}

/**
 * Returns the link status
 *
 * @param   driver        Eth driver control structure
 * @param   inst_num      The instance number of the module
 * @param   slv_port_num  The slave port number for the module
 *
 * @return  the link status
 */
u32_t
cpswif_link_status(struct eth_driver *driver, u32_t inst_num, u32_t slv_port_num)
{
    struct cpswinst *cpswinst = ((struct beaglebone_eth_data*)driver->eth_data)->cpswinst;

    return (PhyLinkStatusGet(cpswinst->mdio_base,
                             cpswinst->port[slv_port_num - 1].phy_addr, 3));
}

/**
 * Checks the value is in the range of min and max
 *
 * @param   vlaue      Value
 * @param   min        Minimum Value
 * @param   max        Maximum Value
 *
 * @return  the status
 */
static u32_t
check_valid(u32_t value, u32_t min, u32_t max)
{
    if ((min <= value) && (value <= max)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*
 * Executes following CPSW Configutarions
 * Switch Configuration (CPSW_SWITCH_CONFIG has to be defined)
 *  1 - Add a multicast entry
 *  2 - Add a unicast entry
 *  3 - Add a OUI entry
 *  4 - Search address in entry list
 *  5 - Delete a multicast entry
 *  6 - Delete a unicast entry
 *  7 - Adds a vlan entry
 *  8 - Search vlan in entry list
 *  9 - Delete vlan
 * 10 - Configure Port Vlan (ID, CFI, PRI)
 * 11 - Age Out the Untouched entries of ALE Table
 * 12 - Print Dump of Switch
 * 13 - Print Dump of Switch Config
 * 14 - ALE VLAN Aware Config
 * 15 - Configure Rate Limit for TX or RX
 * 16 - Enable Engress Check
 * 17 - Set port unknown VLAN info
 * 18 - Enable MAC Auth
 * 19 - Configure Port State
 * Phy Configuration
 *  1 - Configure PHY of a port
 *
 * @param  driver   ethernet driver data structure
 * @param   cpsw_switch_config  parameters required for configuration
 *
 * @return  None
*/
void
cpsw_switch_configuration(struct eth_driver *driver, struct cpsw_config *cpsw_config)
{
    struct cpswinst *cpswinst = ((struct beaglebone_eth_data*) driver->eth_data)->cpswinst;
    struct cpsw_phy_param *cpsw_phy_param = cpsw_config->phy_param;

    switch (cpsw_config->cmd) {
    case CONFIG_SWITCH_SET_PORT_CONFIG: {
        if (!check_valid(cpsw_phy_param->slv_port_num, MIN_SLV_PORT,
                         MAX_SLV_PORT)) {
            cpsw_config->ret = ERR_SLV_PORT;
            break;
        }

        if (!check_valid(cpsw_phy_param->autoneg, MIN_AUTONEG, MAX_AUTONEG)) {
            cpsw_config->ret = ERR_AUTONEG;
            break;
        }

        if (TRUE == cpsw_phy_param->autoneg) {
            if (!check_valid(cpsw_phy_param->config, MIN_PHY_CONFIG,
                             MAX_PHY_CONFIG)) {
                cpsw_config->ret = ERR_PHY_CONFIG;
                break;
            }
        } else {
            if (!check_valid(cpsw_phy_param->speed, MIN_SPEED, MAX_SPEED)) {
                cpsw_config->ret = ERR_SPEED;
                break;
            }

            if (!check_valid(cpsw_phy_param->duplex, MIN_DUPLEX, MAX_DUPLEX)) {
                cpsw_config->ret = ERR_DUPLEX;
                break;
            }
        }

        if (cpsw_phy_param->autoneg)
            cpswif_phy_autoneg(cpswinst, cpsw_phy_param->slv_port_num,
                               cpsw_phy_param->config);
        else
            cpswif_phy_forced(cpswinst, cpsw_phy_param->slv_port_num,
                              cpsw_phy_param->speed,
                              cpsw_phy_param->duplex);

        cpsw_config->ret = ERR_PASS;
        break;
    }

    default:
        cpsw_config->ret = ERR_INVAL;
        break;
    }
}
