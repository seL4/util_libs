/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include <ethdrivers/e82574L.h>
#include "../../descriptors.h"
#include "../../raw_iface.h"
#include <ethdrivers/lwip_iface.h>
#include <netif/etharp.h>

#include "registers.h"
#include "../../dma_buffers.h"

#include <stdlib.h>
#include <assert.h>

struct e82574L_eth_data {
    struct regs *regs;
};

typedef struct e82574L_eth_data  e82574L_eth_data_t;

struct __attribute((packed)) tx_ldesc {
    uint32_t bufferAddress;
    uint32_t bufferAddressHigh;
    uint32_t length: 16;
    uint32_t CSO: 8;
    uint32_t CMD: 8;
    uint32_t STA: 4;
    uint32_t ExtCMD: 4;
    uint32_t CSS: 8;
    uint32_t VLAN: 16;
};

struct __attribute((packed)) rx_ldesc {
    uint32_t bufferAddress;
    uint32_t bufferAddressHigh;
    uint32_t length: 16;
    uint32_t packetChecksum: 16;
    uint32_t status: 8;
    uint32_t error: 8;
    uint32_t VLAN: 16;
};

struct regs {
    uint32_t *control;
    uint32_t *status;
    uint32_t *imc; // Interrupt Mask Clear
    uint32_t *ims; // Interrupt Mask Set/Read
    uint32_t *ics; // Interrupt Cause Set
    uint32_t *txdctl; // Transmit Descriptor Control
    uint32_t *tctl; // Transmit Control Register
    uint32_t *tipg; // Transmit Inter Packet Gap
    uint32_t *tdbal; // Transmit Descriptor Base Address Low
    uint32_t *tdbah; // Transmit Descriptor Base Address High
    uint32_t *tdlen; // Transmit Descriptor Length
    uint32_t *tdh; // Transmit Descriptor Head
    uint32_t *tdt; // Transmit Descriptor Tail
    uint32_t *mta; // Multicast Table Array
    uint32_t *rctl; // Recieve Control Register
    uint32_t *rdbal0; // Recieve Descriptor Base Address Low 0
    uint32_t *rdbah0; // Recieve Descriptor Base Address High 0
    uint32_t *rdlen0; // Recieve Descriptor Length 0 
    uint32_t *rdt0; // Recieve Descriptor Tail 0
    uint32_t *rdh0; // Recieve Descriptor Head 0
    uint32_t *itr; // Interrupt Throtling Register
    uint32_t *icr; // Interrupt Cause Read
};

#define NUM_DMA_BUFS (CONFIG_LIB_ETHDRIVER_RX_DESC_COUNT * 2 + \
                        CONFIG_LIB_ETHDRIVER_TX_DESC_COUNT)
#define MAX_PKT_SIZE 1536
 
// TX Descriptor Status Bits
#define TX_DD BIT(0) /* Descriptor Done */
#define TX_EOP BIT(0) /* End of Packet */

// RX Descriptor Status Bits
#define RX_DD BIT(0) /* Descriptor Done */
#define RX_E_RSV BIT(3) /* Error Reserved bit */

/* EXTEND OFF PACKET CODES (bytes) 
 * 0b00 - 2048 
 * 0b01 - 1024
 * 0b10 - 512
 * 0b11 - 256
 */

/* EXTEND ON PACKET CODES (bytes)
 * 0b00 - INVALID, DO NOT SET
 * 0b01 - 16384 
 * 0b10 - 8192
 * 0b11 - 4096
 */

// select the correct settings from the table above
#define PACKET_BUFFER_SIZE_EXTEND 1
#define PACKET_BUFFER_SIZE 8192
#define PACKET_BUFFER_SIZE_CODE 0b10
   
struct raw_iface_funcs* setup_raw_iface_funcs();
struct raw_desc_funcs* setup_raw_desc_funcs();
void e82574L_start_tx_logic();
void e82574L_start_rx_logic();
void e82574L_low_level_init(struct netif *netif); 
const int* e82574L_raw_enableIRQ(struct eth_driver* eth_driver, int *nirqs);
void e82574L_raw_handleIRQ(struct netif* netif, int irq); 
dma_addr_t e82574L_create_tx_descs(ps_dma_man_t *dma_man, int count);
dma_addr_t e82574L_create_rx_descs(ps_dma_man_t *dma_man, int count);
void e82574L_reset_tx_descs(struct eth_driver *driver);
void e82574L_reset_rx_descs(struct eth_driver *driver);
int e82574L_is_tx_desc_ready(int buf_num, struct desc *desc);
int e82574L_is_rx_desc_empty(int buf_num, struct desc *desc);
void e82574L_set_tx_desc_buf(int buf_num, dma_addr_t buf, int len, int tx_desc_wrap, int tx_last_section, struct desc *desc);
void e82574L_ready_tx_desc(int buf_num, int num, struct eth_driver *driver);
void e82574L_ready_rx_desc(int buf_num, int tx_desc_wrap, struct eth_driver *driver);
void e82574L_set_rx_desc_buf(int buf_num, dma_addr_t buf, int len, struct desc *desc);
int e82574L_get_rx_buf_len(int buf_num, struct desc *desc);
int e82574L_get_rx_desc_error(int buf_num, struct desc *desc);

void 
e82574L_eth_get_mac(struct eth_driver *eth_driver, uint8_t *hwaddr) 
{
    /* read the first 3 shorts of the eeprom */
    // e82574L_eth_data_t *data = (e82574L_eth_data_t*)eth_driver->eth_data;

    // hard coding MAC address
    hwaddr[0] = 0x68;
    hwaddr[1] = 0x05;
    hwaddr[2] = 0xca;
    hwaddr[3] = 0x01;
    hwaddr[4] = 0x50;
    hwaddr[5] = 0x55;

    printf("Got MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",hwaddr[0], hwaddr[1], 
            hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);
}

void 
reset_device (struct regs *regs) 
{
    volatile uint32_t temp_control = *regs->control; 
    temp_control &= ~CTRL_RESERVED_BITS; // clear reserved
    temp_control |= CTRL_RST; // set RST - reset the device
    *regs->control = temp_control; // write it out
    return;
}

void 
enable_interrupts(struct regs *regs) 
{
    volatile uint32_t temp_ims = *regs->ims;

    temp_ims |= IMS_RXQ0;
    temp_ims |= IMS_RXO;

    *regs->ims = temp_ims;
    return;
}

void 
disable_all_interrupts(struct regs *regs) 
{
    volatile uint32_t temp_imc = *regs->imc;
    temp_imc &= ~IMC_RESERVED_BITS; // clear reserved, they may read as 1.
    temp_imc |= ~IMC_RESERVED_BITS; // i.e. turn off all interrupts
    *regs->imc = temp_imc;
    return;
}

void 
initialise_control(struct regs *regs) 
{
    volatile uint32_t temp_ctrl = *regs->control;

    temp_ctrl &= ~CTRL_RESERVED_BITS;
    temp_ctrl &= ~CTRL_FRCDPLX;
    temp_ctrl &= ~CTRL_FRCSPD;
    temp_ctrl &= ~CTRL_ASDE;
    temp_ctrl |= CTRL_SLU; // set-link up to 1

    // RFCE is meant to do something but auto-nego?
    // TFCE is also meant to be set to something but auto-nego?

    *regs->control = temp_ctrl;
    return;
}

void 
moderate_interrupts(struct regs *regs) 
{
    volatile uint32_t temp_itr = 0;
    // set it to 651 first as minimum interval
//    temp_itr = ITR_INTERVAL_BITS(0x0);
    /* Disable interrupt intervals as seL4 already does masking */
    temp_itr = 0;
    *regs->itr = temp_itr;
}

void 
initialise(struct regs *regs)
{
    disable_all_interrupts(regs); 
    reset_device(regs); 
    disable_all_interrupts(regs); 
    initialise_control(regs);

    uint32_t status = *(regs->status);
    printf("    82574L-GBE-CONTROLLER: ONLINE\n");
    printf("    LINK UP?        [%c]\n", (status & STATUS_LU) ? 'Y' : 'N'); 
    printf("    FULL DUPLEX?    [%c]\n", (status & STATUS_FD) ? 'Y' : 'N');
    printf("    GIGABIT?        [%c]\n", (status & STATUS_SPEED_MASK) ? 'Y' : 'N');
    printf("\n");

    return;
}

void initialise_TXDCTL(struct regs *regs) 
{
    uint32_t temp_TXDCTL = 0;   
    //******** TXDCTL *********//
    // zero reserved 
    temp_TXDCTL &= ~TXDCTL_RESERVED_BITS;

    // set the bit that should be set
    temp_TXDCTL |= TXDCTL_BIT_THAT_SHOULD_BE_1;

    // set GRAN to 1
    temp_TXDCTL |= TXDCTL_GRAN;
    temp_TXDCTL |= TXDCTL_WTHRESH_BITS(1);

    // write it back
    *regs->txdctl = temp_TXDCTL;
    return;
}

void initialise_TCTL(struct regs *regs) {
    volatile uint32_t temp_TCTL = *regs->tctl;
    
    //******* TCTL ************//
    // zero reserved
    temp_TCTL &= TCTL_RESERVED_BITS;

    // set CT to 0x0F
    // not used in full-duplex
    //temp_TCTL |= TCTL_CT_BITS(0x0F);

    // set COLD to 0x03F for FD (or 0x1FF for HD)
    temp_TCTL |= TCTL_COLD_BITS(0x3F);

    // PSP 1
    temp_TCTL |= TCTL_PSP;

    // ENABLE 1
    temp_TCTL |= TCTL_EN;

    // clear SWXOFF, PBE, RTLC, UNORTX ,TXDSCMT, RRTHRESH
    temp_TCTL &= ~TCTL_SWXOFF;
    temp_TCTL &= ~TCTL_PBE;
    temp_TCTL &= ~TCTL_RTLC;
    temp_TCTL &= ~TCTL_UNORTX;
   
    temp_TCTL &= ~TCTL_TXDSCMT_BITS(0b11);

    // allows multiple transmit requests from hardware
    // at once
    // NOTE: Affects performance severely if turned off
    temp_TCTL |= TCTL_MULR; 

    temp_TCTL &= ~TCTL_RRTHRESH_BITS(0b11);

    // write it
    *regs->tctl = temp_TCTL;
    return;
}

void initialise_TIPG(struct regs *regs) {
    volatile uint32_t temp_TIPG = *regs->tipg;
    
    //********* TIPG **********//
    // clear reserve bits
    temp_TIPG &= ~TIPG_RESERVED_BITS;

    // IPGT = 8
    temp_TIPG &= ~TIPG_IPGT_BITS(0b1111111111);
    temp_TIPG |= TIPG_IPGT_BITS(8);

    // IPGR1 = 2
    temp_TIPG &= ~TIPG_IPGR1_BITS(0b1111111111);
    temp_TIPG |= TIPG_IPGR1_BITS(2);

    // IPGR2 = 10
    temp_TIPG &= ~TIPG_IPGR2_BITS(0b1111111111);
    temp_TIPG |= TIPG_IPGR2_BITS(10);

    // write it back
    *regs->tipg = temp_TIPG;
    return;
}

void initialise_transmit(struct regs *regs) 
{
    initialise_TXDCTL(regs);
    initialise_TCTL(regs);
    initialise_TIPG(regs);
    return;
}

void initialise_RCTL(struct regs *regs) {
    uint32_t temp = *(regs->rctl);

    // zero reserved bits
    temp &= ~RCTL_RESERVED_BITS;
    
    // receive descriptor minimum threshold, 1/8
    temp |= RCTL_RDMTS_BITS(0b10);

    // enable!
    temp |= RCTL_EN;

    // Unicast promiscuous enable 1!
    temp |= RCTL_UPE;

    // multicast promiscuous mode 1
    temp |= RCTL_MPE;

    // loopback mode - 00 for normal operation
    temp &= ~RCTL_LBM_BITS(3); // clear

    // descriptor type - 00 for legacy
    temp &= ~RCTL_DTYP_BITS(3);
    
    // accept broadcast - 1
    temp |= RCTL_BAM;

    // make buffers a certain size
    // NOTE: RCTL_BSEX needs to be set carefully, as it scales RCTL_BSIZE.
    if (PACKET_BUFFER_SIZE_EXTEND == 1) {
        temp |= RCTL_BSEX;
    } else {
        temp &= ~RCTL_BSEX;
    }

    // zero BSIZE
    temp &= ~RCTL_BSIZE_BITS(0b11);

    // put in the actual desired value
    temp |= RCTL_BSIZE_BITS(PACKET_BUFFER_SIZE_CODE);

    // write it back
    *(regs->rctl) = temp;
    return;
}

void initialise_receive(struct regs *regs) {
    // zero the MTA array
    int i;
    for (i = 0; i < MTA_LENGTH; i++) {
        *(regs->mta + i) = 0; // MTA ptr is of type uint32_t (32bits)
    }
 
    initialise_RCTL(regs);
    return;
}

void
map_regs(struct regs *regs, char *bar0_map)
{
    regs->control = (uint32_t*) (bar0_map + CTRL_REG_OFFSET);
    regs->status = (uint32_t*) (bar0_map + STATUS_REG_OFFSET);
    regs->imc = (uint32_t*) (bar0_map + IMC_REG_OFFSET);
    regs->ims = (uint32_t*) (bar0_map + IMS_REG_OFFSET);
    regs->ics = (uint32_t*) (bar0_map + ICS_REG_OFFSET);
    regs->icr = (uint32_t*) (bar0_map + ICR_REG_OFFSET);

    regs->txdctl = (uint32_t*) (bar0_map + TXDCTL_REG_OFFSET);
    regs->tctl = (uint32_t*) (bar0_map + TCTL_REG_OFFSET);
    regs->tipg = (uint32_t*) (bar0_map + TIPG_REG_OFFSET);

    regs->tdbal = (uint32_t*) (bar0_map + TDBAL_REG_OFFSET);   
    regs->tdbah = (uint32_t*) (bar0_map + TDBAH_REG_OFFSET);   
    regs->tdlen = (uint32_t*) (bar0_map + TDLEN_REG_OFFSET);   
    regs->tdh = (uint32_t*) (bar0_map + TDH_REG_OFFSET);   
    regs->tdt = (uint32_t*) (bar0_map + TDT_REG_OFFSET);   

    regs->mta = (uint32_t*) (bar0_map + MTA_REG_OFFSET);
    regs->rctl = (uint32_t*) (bar0_map + RCTL_REG_OFFSET);
    regs->rdbal0 = (uint32_t*) (bar0_map + RDBAL_REG_OFFSET);
    regs->rdbah0 = (uint32_t*) (bar0_map + RDBAH_REG_OFFSET);
    regs->rdlen0 = (uint32_t*) (bar0_map + RDLEN_REG_OFFSET);
    regs->rdt0 = (uint32_t*) (bar0_map + RDT_REG_OFFSET);
    regs->rdh0 = (uint32_t*) (bar0_map + RDH_REG_OFFSET);

    regs->itr = (uint32_t*) (bar0_map + ITR_REG_OFFSET);
}

struct eth_driver*
ethif_e82574L_init(int dev_id, ps_io_ops_t io_ops, char *pci_bar_start)
{
    struct eth_driver *driver = malloc(sizeof(struct eth_driver));
    assert(driver);
    struct e82574L_eth_data *eth_data = malloc(sizeof(struct e82574L_eth_data));
    assert(eth_data);
    struct desc *desc = malloc(sizeof(struct desc));
    assert(desc);

    struct regs *regs = malloc(sizeof(struct regs));
    assert(regs);
    map_regs(regs, pci_bar_start);
    eth_data->regs = regs; 
    driver->eth_data = (void*)eth_data;
    driver->io_ops = io_ops;

    initialise(regs);
    printf("Basic initialisation completed - sanity\n");
 
    initialise_transmit(regs);
    printf("Transmit initialisation complete\n");
    initialise_receive(regs); 
    printf("Receive initialisation complete\n");

    moderate_interrupts(regs);
    printf("interrupts moderation complete\n");

    // Setup driver specific functions
    driver->r_fn = setup_raw_iface_funcs();
    assert(driver->r_fn);
    driver->d_fn = setup_raw_desc_funcs();
    assert(driver->d_fn);

    desc = desc_init(&io_ops.dma_manager, CONFIG_LIB_ETHDRIVER_RX_DESC_COUNT, 
                     CONFIG_LIB_ETHDRIVER_TX_DESC_COUNT, NUM_DMA_BUFS, 
                     MAX_PKT_SIZE, driver);
    if (!desc) {
        assert(0);
    }

    driver->desc = desc;
    printf("Initialisation Complete\n\n\n");
    return driver;
}

struct raw_iface_funcs*
setup_raw_iface_funcs()
{
    struct raw_iface_funcs* r_fn = malloc(sizeof(struct raw_iface_funcs));
    if (!r_fn) {
        return NULL;
    }
    r_fn->low_level_init = e82574L_low_level_init;
    r_fn->start_tx_logic = e82574L_start_tx_logic;
    r_fn->start_rx_logic = e82574L_start_rx_logic;
    r_fn->raw_handleIRQ = e82574L_raw_handleIRQ; 
    r_fn->raw_enableIRQ = e82574L_raw_enableIRQ;
    // Missing rawscattertx, print_state
    return r_fn;
}

struct raw_desc_funcs*
setup_raw_desc_funcs()
{
    struct raw_desc_funcs *d_fn = malloc(sizeof(struct raw_desc_funcs));
    if (!d_fn) {
        return NULL;
    }
    d_fn->create_tx_descs = e82574L_create_tx_descs;
    d_fn->create_rx_descs = e82574L_create_rx_descs;
    d_fn->reset_tx_descs = e82574L_reset_tx_descs;
    d_fn->reset_rx_descs = e82574L_reset_rx_descs;
    d_fn->ready_tx_desc = e82574L_ready_tx_desc;
    d_fn->ready_rx_desc = e82574L_ready_rx_desc;
    d_fn->is_tx_desc_ready = e82574L_is_tx_desc_ready;
    d_fn->is_rx_desc_empty = e82574L_is_rx_desc_empty;
    d_fn->set_tx_desc_buf = e82574L_set_tx_desc_buf;
    d_fn->set_rx_desc_buf = e82574L_set_rx_desc_buf;
    d_fn->get_rx_buf_len = e82574L_get_rx_buf_len;
    d_fn->get_rx_desc_error = e82574L_get_rx_desc_error;
    return d_fn;
}

// These are noops for this driver 
void e82574L_start_tx_logic(){};
void e82574L_start_rx_logic(){};

void
e82574L_low_level_init(struct netif *netif) 
{
    assert(netif);
#if LWIP_NETIF_HOSTNAME
    /* Initialize interface hostname */
    netif->hostname = "82574L Device";
#endif
    netif->name[0] = 'P';
    netif->name[1] = 'C';
    
    netif->hwaddr_len = ETHARP_HWADDR_LEN;
    e82574L_eth_get_mac((struct eth_driver*)netif->state, netif->hwaddr);
    /* maximum transfer unit */
    printf("Hard coding MTU for now \n");
    netif->mtu = 1500;
}


const int*
e82574L_raw_enableIRQ(struct eth_driver* eth_driver, int *nirqs)
{
    assert(eth_driver);
    assert(nirqs);
    e82574L_eth_data_t *data = (e82574L_eth_data_t*)eth_driver->eth_data;
    enable_interrupts(data->regs);
    *nirqs = 0;
    return NULL;
}

void 
e82574L_raw_handleIRQ(struct netif* netif, int irq) 
{
    assert(netif != NULL);
    struct eth_driver *driver = (struct eth_driver*)netif->state;
    e82574L_eth_data_t *data = (e82574L_eth_data_t *)driver->eth_data;
    if (*(data->regs->icr) & (ICR_RXQ0)) {
        //printf("IRQ: Receive Queue 0 Interrupt\n");
        while(ethif_input(netif));
    }
    if (*(data->regs->icr) & (ICR_RXQO)) {
        LOG_ERROR("Receive descriptor overrun");
    }
    // Clear interrupts
    /* Synchronize before achknowledging */
    __sync_synchronize();
    *(data->regs->icr) = 0xFFFFFFFF;
    /* Synchronize what we just did */
    __sync_synchronize();
}

dma_addr_t
e82574L_create_tx_descs(ps_dma_man_t *dma_man, int count)
{
    return dma_alloc_pin(dma_man, sizeof(struct tx_ldesc) * count, 0);
}

dma_addr_t
e82574L_create_rx_descs(ps_dma_man_t *dma_man, int count)
{
    return dma_alloc_pin(dma_man, sizeof(struct rx_ldesc) * count, 0);
}

void
e82574L_reset_tx_descs(struct eth_driver *driver)
{
    int i;
    struct desc *desc = driver->desc;
    e82574L_eth_data_t *data = (e82574L_eth_data_t*)driver->eth_data;
    struct regs *regs = data->regs;
    struct tx_ldesc *d = desc->tx.ring.virt;
    for (i = 0; i < desc->tx.count; i++) {
        d[i].length = 0;
        d[i].CSO = 0;
        d[i].CMD = 0;
        d[i].STA = 1;
        d[i].ExtCMD = 0;
        d[i].CSS = 0;
        d[i].VLAN = 0;
    }

    // ensure updates to the ring are visible before writing these registers
    __sync_synchronize(); // should this do dma cache clean?

    // Tell hardware where the ring is.
    *(regs->tdbah) = 0;
    *(regs->tdbal) = (uint32_t)desc->tx.ring.phys;

    // Setup the transmit queue so its initally empty
    *(regs->tdh) = 0; 
    *(regs->tdt) = 0; 

    /* Sync what we just did */
    __sync_synchronize();

    size_t desc_bytes = desc->tx.count * sizeof(struct tx_ldesc);
    assert(desc_bytes % 128 == 0); // tdlen must be 128 byte aligned
    *(regs->tdlen) = desc_bytes;
}

void
e82574L_reset_rx_descs(struct eth_driver *driver)
{
    printf("Resetting descriptors\n");
    int i;
    struct desc *desc = driver->desc;
    struct e82574L_eth_data *data = (struct e82574L_eth_data *)driver->eth_data;
    struct regs *regs = data->regs;
    assert(regs);
    struct rx_ldesc *d = desc->rx.ring.virt;
    for (i = 0; i < desc->rx.count; i++) { 
        desc->rx.buf[i] = alloc_dma_buf(desc);        
        assert(desc->rx.buf[i].phys); 
        d[i].bufferAddress = (uint32_t)desc->rx.buf[i].phys;
        d[i].bufferAddressHigh = 0;
        d[i].length = 0;
        d[i].packetChecksum = 0;
        d[i].status = 0;
        d[i].error = 0;
        d[i].VLAN = 0;
    }

    // ensure updates to the ring are visible before writing these registers
    __sync_synchronize(); // should this do dma cache clean?

    // Tell hardware where the ring is.
    *(regs->rdbah0) = 0;
    *(regs->rdbal0) = (uint32_t)desc->rx.ring.phys;

    // Setup the transmit queue so its initally full
    *(regs->rdh0) = 0; 
    *(regs->rdt0) = desc->rx.count - 1; // Should this be -1 or nothing? 

    /* Sync what we just did */
    __sync_synchronize();

    size_t desc_bytes = desc->rx.count * sizeof(struct rx_ldesc);
    assert(desc_bytes % 128 == 0); // tdlen must be 128 byte aligned
    *(regs->rdlen0) = desc_bytes;
}

// The sematics are weird here. This chip says when the hardware is done
// with the packet, the opposite of 'ready' which means the packet is
// waiting for hardware.
int
e82574L_is_tx_desc_ready(int buf_num, struct desc *desc)
{
    volatile struct tx_ldesc *ring = desc->tx.ring.virt;
    return !(ring[buf_num].STA & TX_DD);
}

int
e82574L_is_rx_desc_empty(int buf_num, struct desc *desc)
{
    volatile struct rx_ldesc *ring = desc->rx.ring.virt;
    return !(ring[buf_num].status & RX_DD);
}

void
e82574L_set_tx_desc_buf(int buf_num, dma_addr_t buf, int len, int tx_desc_wrap, int tx_last_section, struct desc *desc)
{
    struct tx_ldesc *d = desc->tx.ring.virt;
    d[buf_num].bufferAddress = buf.phys; // Check how to assign 64 bits properly
    d[buf_num].bufferAddressHigh = 0;
    d[buf_num].length = len;
    d[buf_num].CSO = 0;
    d[buf_num].CMD = 0b1010 | (tx_last_section ? TX_EOP : 0);
    d[buf_num].STA = 0;
    d[buf_num].ExtCMD = 0;
    d[buf_num].CSS = 0;
    d[buf_num].VLAN = 0;
}

void
e82574L_ready_tx_desc(int buf_num, int num, struct eth_driver *driver)
{
    struct tx_ldesc *d = driver->desc->tx.ring.virt;
    /* Synchronize any previous updates before moving the tail register */
    __sync_synchronize();
    /* Move the tail reg. */
    e82574L_eth_data_t *eth_data = (e82574L_eth_data_t*)driver->eth_data;
    /* Tail should have previously been at the first buf we are enabling */
    assert(*(eth_data->regs->tdt) == buf_num);
    *(eth_data->regs->tdt) = (buf_num + num) % driver->desc->tx.count;
    /* Make sure these updates get seen */
    __sync_synchronize();
}

void
e82574L_ready_rx_desc(int buf_num, int tx_desc_wrap, struct eth_driver *driver)
{
    // Move the tail reg. Assumes this is called for every desc
    volatile e82574L_eth_data_t *eth_data = (e82574L_eth_data_t*)driver->eth_data;
    *(eth_data->regs->rdt0) = (*(eth_data->regs->rdt0) + 1) % driver->desc->rx.count;
    //assert(*(eth_data->regs->rdt0) - 1 == buf_num); 
    __sync_synchronize();
} 

void
e82574L_set_rx_desc_buf(int buf_num, dma_addr_t buf, int len, struct desc *desc)
{
    struct rx_ldesc *d = desc->rx.ring.virt;
    d[buf_num].bufferAddress = buf.phys; // Check how to assign 64 bits properly
    d[buf_num].bufferAddressHigh = 0;
    d[buf_num].length = len;
    d[buf_num].packetChecksum = 0;
    d[buf_num].status = 0;
    d[buf_num].error = 0;
    d[buf_num].VLAN = 0;
    __sync_synchronize();
}

int
e82574L_get_rx_buf_len(int buf_num, struct desc *desc)
{
    volatile struct rx_ldesc *ring = desc->rx.ring.virt;
    return ring[buf_num].length;
}

int
e82574L_get_rx_desc_error(int buf_num, struct desc *desc)
{
    volatile struct rx_ldesc *ring = desc->rx.ring.virt;
    return ring[buf_num].error & ~RX_E_RSV; // Ignore the reserved error bit
}
