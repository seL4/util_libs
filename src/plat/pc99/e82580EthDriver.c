/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include <ethdrivers/e82580.h>
#include "../../descriptors.h"
#include "../../raw_iface.h"
#include <ethdrivers/lwip_iface.h>
#include <netif/etharp.h>
#include "../../dma_buffers.h"

#define NUM_DMA_BUFS (CONFIG_LIB_ETHDRIVER_RX_DESC_COUNT * 2 + \
                        CONFIG_LIB_ETHDRIVER_TX_DESC_COUNT)

// TX Descriptor Status Bits
#define TX_DD BIT(0) /* Descriptor Done */
#define TX_EOP BIT(0) /* End of Packet */

// RX Descriptor Status Bits
#define RX_DD BIT(0) /* Descriptor Done */
#define RX_E_RSV BIT(3) /* Error Reserved bit */


#define REG(x,y) (*(volatile uint32_t*)(((uintptr_t)(x)) + (y)))

#define REG_CTRL(x) REG(x, 0x0)
#define REG_IMC(x) REG(x, 0x150c)
#define REG_STATUS(x) REG(x, 0x8)
#define REG_TXDCTL(x, y) REG(x, 0xE028 + 0x40 * (y))
#define REG_MTA(x, y) REG(x, 0x5200 + 4 * (y))
#define REG_RCTL(x) REG(x, 0x100)
#define REG_EERD(x) REG(x, 0x14)
#define REG_TCTL(x) REG(x, 0x0400)
#define REG_TDBAL(x, y) REG(x, 0xE000 + (y) * 0x40)
#define REG_TDBAH(x, y) REG(x, 0xE004 + (y) * 0x40)
#define REG_TDLEN(x, y) REG(x, 0xE008 + (y) * 0x40)
#define REG_TDH(x, y) REG(x, 0xE010 + (y) * 0x40)
#define REG_TDT(x, y) REG(x, 0xE018 + (y) * 0x40)
#define REG_RDBAL(x, y) REG(x, 0xC000 + (y) * 0x40)
#define REG_RDBAH(x, y) REG(x, 0xC004 + (y) * 0x40)
#define REG_RDLEN(x, y) REG(x, 0xC008 + (y) * 0x40)
#define REG_RDH(x, y) REG(x, 0xc010 + (y) * 0x40)
#define REG_RDT(x, y) REG(x, 0xc018 + (y) * 0x40)
#define REG_RXDCTL(x, y) REG(x, 0xc028 + (y) * 0x40)
#define REG_IMS(x) REG(x, 0x1508)
#define REG_ICR(x) REG(x, 0x1500)
#define REG_EICR(x) REG(x, 0x1580)
#define REG_EICS(x) REG(x, 0x1520)
#define REG_EIMS(x) REG(x, 0x1524)
#define REG_EIMC(x) REG(x, 0x1528)

#define IMC_RESERVED_BITS (BIT(1) | BIT(3) | BIT(5) | BIT(9) | BIT(15) | BIT(16) | BIT(17) | BIT(21) | BIT(23) | BIT(27) | BIT(31))
#define CTRL_RESERVED_BITS (BIT(1) | BIT(3) | BIT(4) | BIT(5) | BIT(10) | BIT(13) | BIT(14) | BIT(15) | BIT(24) | BIT(25))

#define CTRL_SLU BIT(6)
#define CTRL_FRCSPD BIT(11)
#define CTRL_FRCDPLX BIT(12)
#define CTRL_RST BIT(26)

#define STATUS_FD BIT(0)
#define STATUS_LU BIT(1)
#define STATUS_SPEED_OFFSET 6
#define STATUS_SPEED_MASK (BIT(6) | BIT(7))
#define STATUS_LAN_ID_OFFSET 2
#define STATUS_LAN_ID_MASK (BIT(2) | BIT(3))

#define RCTL_RESERVED_BITS (BIT(0) | BIT(10) | BIT(11) | BIT(27) | BIT(28) | BIT(29) | BIT(30) | BIT(31))
#define RCTL_EN BIT(1)
#define RCTL_UPE BIT(3)
#define RCTL_MPE BIT(4)
#define RCTL_LBM(x) ((x) << 6)
#define RCTL_LBM_NORMAL RCTL_LBM(0x00)
#define RCTL_BSIZE(x) ((x) << 16)
#define RCTL_BSIZE_2K RCTL_BSIZE(0x00)
#define RCTL_BAM BIT(15)

#define TXDCTL_RESERVED_BITS (0)
#define TXDCTL_ENABLE BIT(25)

#define EERD_START BIT(0)
#define EERD_DONE BIT(1)
#define EERD_ADDR_OFFSET 2

#define TCTL_RESERVED_BITS (BIT(0) | BIT(2))
#define TCTL_EN BIT(1)
#define TCTL_PSP BIT(3)

#define RXDCTL_RESERVED_BITS (0)
#define RXDCTL_ENABLE BIT(25)

#define IMS_RXDW BIT(7)

#define ICR_RXDW BIT(7)

#define EEPROM_LAN(id, x) ( ((id) ? 0 : 0x40) * (id) + (x))

#define MTA_LENGTH 128

struct __attribute((packed)) tx_ldesc {
    uint32_t bufferAddress;
    uint32_t bufferAddressHigh;
    uint32_t length: 16;
    uint32_t CSO: 8;
    uint32_t CMD: 8;
    uint32_t STA: 4;
    uint32_t reserved : 4;
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

static void disable_all_interrupts(void *base) {
    REG_IMC(base) = ~IMC_RESERVED_BITS;
}

static void reset_device(void *base) {
    REG_CTRL(base) = (REG_CTRL(base) & (~CTRL_RESERVED_BITS)) | CTRL_RST;
}

static void initialize_control(void *base) {
    uint32_t temp = REG_CTRL(base);
    temp &= ~CTRL_RESERVED_BITS;
    temp &= ~CTRL_FRCDPLX;
    temp &= ~CTRL_FRCSPD;
    temp |= CTRL_SLU;
    REG_CTRL(base) = temp;
}

static void initialize(void *base) {
    disable_all_interrupts(base);
    reset_device(base);
    disable_all_interrupts(base);
    initialize_control(base);
}

static void initialise_TXDCTL(void *base) {
    /* Enable a single transmit queue */
    uint32_t temp = REG_TXDCTL(base, 0);
    temp &= ~TXDCTL_RESERVED_BITS;
    /* Make sure not currently enabled */
    temp &= ~TXDCTL_ENABLE;
    REG_TXDCTL(base, 0) = temp;
}

static void initialise_TCTL(void *base)
{
    uint32_t temp = REG_TCTL(base);
    temp &= TCTL_RESERVED_BITS;
    temp |= TCTL_EN;
    temp |= TCTL_PSP;
    REG_TCTL(base) = temp;
}

static void initialise_TIPG(void *base) {
    /* Do nothing? */
}

static void initialize_transmit(void *base) {
    initialise_TXDCTL(base);
    initialise_TCTL(base);
    initialise_TIPG(base);
}

static void initialize_RCTL(void *base) {
    uint32_t temp = REG_RCTL(base);
    /* zero reserved bits */
    temp &= ~RCTL_RESERVED_BITS;
    temp |= RCTL_EN;
    temp |= RCTL_UPE;
    temp |= RCTL_MPE;
    temp |= RCTL_LBM_NORMAL;
    temp |= RCTL_BSIZE_2K;
    temp |= RCTL_BAM;
    REG_RCTL(base) = temp;
}

static void initialize_receive(void *base) {
    /* zero the MTA */
    int i;
    for (i = 0; i < MTA_LENGTH; i++) {
        REG_MTA(base, i) = 0;
    }
    initialize_RCTL(base);
}

void handle_irq(struct netif *netif, int irq) {
    void *base = ((struct eth_driver*)netif->state)->eth_data;
    uint32_t icr = REG_ICR(base);
    /* delay before we acknowledge in the kernel */
    if (icr & ICR_RXDW) {
        while(ethif_input(netif));
    }
}

const int* enable_irq(struct eth_driver *eth_driver, int *nirqs) {
    /* ACK any existing interrupts */
    REG_ICR(eth_driver->eth_data) = 0xffffffff;
    uint32_t temp = REG_IMS(eth_driver->eth_data);
    temp |= IMS_RXDW;
    REG_IMS(eth_driver->eth_data) = temp;
    temp = REG_ICR(eth_driver->eth_data);
    REG_ICR(eth_driver->eth_data) = 0xffffffff;
    *nirqs = 0;
    return NULL;
}

void print_state(struct eth_driver *eth_driver) {
}

static uint16_t read_eeprom(void *base, uint16_t reg) {
    REG_EERD(base) = EERD_START | (reg << EERD_ADDR_OFFSET);
    uint32_t val;
    while ( ((val = REG_EERD(base)) & EERD_DONE) == 0);
    return val >> 16;
}

void eth_get_mac(struct eth_driver *eth_driver, uint8_t *hwaddr) {
    /* read our LAN ID so we know what port we are */
    int id = (REG_STATUS(eth_driver->eth_data) & STATUS_LAN_ID_MASK) >> STATUS_LAN_ID_OFFSET;
    /* read the first 3 shorts of the eeprom */
    uint16_t mac[3];
    mac[0] = read_eeprom(eth_driver->eth_data, EEPROM_LAN(id, 0x00));
    mac[1] = read_eeprom(eth_driver->eth_data, EEPROM_LAN(id, 0x01));
    mac[2] = read_eeprom(eth_driver->eth_data, EEPROM_LAN(id, 0x02));
    hwaddr[0] = mac[0] & MASK(8);
    hwaddr[1] = mac[0] >> 8;
    hwaddr[2] = mac[1] & MASK(8);
    hwaddr[3] = mac[1] >> 8;
    hwaddr[4] = mac[2] & MASK(8);
    hwaddr[5] = mac[2] >> 8;
}

void low_level_init(struct netif *netif) {
#if LWIP_NETIF_HOSTNAME
    netif->hostname = "82580 Device";
#endif
    netif->name[0] = 'P';
    netif->name[1] = 'C';
    netif->hwaddr_len = ETHARP_HWADDR_LEN;
    eth_get_mac((struct eth_driver*)netif->state, netif->hwaddr);
    /* hardcode MTU for now */
    netif->mtu = 1500;
}

static void start_tx_logic(struct netif *netif) {
}

static void start_rx_logic(struct netif *netif) {
}

static struct raw_iface_funcs iface_fns = {
    .raw_handleIRQ = handle_irq,
    .raw_enableIRQ = enable_irq,
    .print_state = print_state,
    .low_level_init = low_level_init,
    .start_tx_logic = start_tx_logic,
    .start_rx_logic = start_rx_logic
};

static dma_addr_t create_tx_descs(ps_dma_man_t *dma_man, int count) {
    return dma_alloc_pin(dma_man, sizeof(struct tx_ldesc) * count, 1, 128);
}

static dma_addr_t create_rx_descs(ps_dma_man_t *dma_man, int count) {
    return dma_alloc_pin(dma_man, sizeof(struct rx_ldesc) * count, 1, 128);
}

static void reset_tx_descs(struct eth_driver *driver) {
    void *base = driver->eth_data;
    int i;
    struct desc *desc = driver->desc;
    struct tx_ldesc *d = desc->tx.ring.virt;
    /* First ensure transmit is not enabled */
    REG_TXDCTL(base, 0) &= (~TXDCTL_RESERVED_BITS) | (~TXDCTL_ENABLE);
    /* Zero the descriptors */
    for (i = 0; i < desc->tx.count; i++) {
        d[i].length = 0;
        d[i].CSO = 0;
        d[i].CMD = 0;
        d[i].STA = 1;
        d[i].CSS = 0;
        d[i].VLAN = 0;
    }

    /* Ensure updates are visible */
    __sync_synchronize();

    /* Tell the hardware where the ring is */
    REG_TDBAL(base, 0) = (uint32_t)desc->tx.ring.phys;
    REG_TDBAH(base, 0) = 0;

    /* Setup transmit queues to be initially empty */
    REG_TDH(base, 0) = 0;
    REG_TDT(base, 0) = 0;

    /* Set length of the ring */
    size_t desc_bytes = desc->tx.count * sizeof(struct tx_ldesc);
    assert(desc_bytes % 128 == 0); // tdlen must be 128 byte aligned
    REG_TDLEN(base, 0) = desc_bytes;

    /* Sync everything we just did */
    __sync_synchronize();

    /* Enable transmit */
    REG_TXDCTL(base, 0) = (REG_TXDCTL(base, 0) & (~TXDCTL_RESERVED_BITS)) | TXDCTL_ENABLE;
}

static void reset_rx_descs(struct eth_driver *driver) {
    void *base = driver->eth_data;
    int i;
    struct desc *desc = driver->desc;
    struct rx_ldesc *d = desc->rx.ring.virt;
    /* First ensure receive is not enabled */
    REG_RXDCTL(base, 0) &= (~RXDCTL_RESERVED_BITS) | (~RXDCTL_ENABLE);
    /* Zero the descriptors */
    for (i = 0; i < desc->rx.count; i++) {
        desc->rx.buf[i] = alloc_dma_buf(desc);
        d[i].bufferAddress = (uint32_t)desc->rx.buf[i].phys;
        d[i].bufferAddressHigh = 0;
        d[i].length = 0;
        d[i].packetChecksum = 0;
        d[i].status = 0;
        d[i].error = 0;
        d[i].VLAN = 0;
    }

    /* Ensure updates are visible */
    __sync_synchronize();

    /* Tell the hardware where the ring is */
    REG_RDBAL(base, 0) = (uint32_t)desc->rx.ring.phys;
    REG_RDBAH(base, 0) = 0;

    /* Set length of the ring */
    size_t desc_bytes = desc->rx.count * sizeof(struct rx_ldesc);
    assert(desc_bytes % 128 == 0); // tdlen must be 128 byte aligned
    REG_RDLEN(base, 0) = desc_bytes;

    __sync_synchronize();

    /* Enable receive queue */
    REG_RXDCTL(base, 0) = (REG_RXDCTL(base, 0) & (~RXDCTL_RESERVED_BITS)) | RXDCTL_ENABLE;

    __sync_synchronize();

    /* Set the tail to initialize be full */
    REG_RDT(base, 0) = (REG_RDH(base, 0) + desc->rx.count - 1) % desc->rx.count;

    __sync_synchronize();
}

static void ready_tx_desc(int buf_num, int num, struct eth_driver *driver) {
    /* Synchronize any previous updates before moving the tail register */
    __sync_synchronize();
    /* Tail should have previously been at the first buf we are enabling */
    assert(REG_TDT(driver->eth_data, 0) == buf_num);
    /* Move the tail reg. */
    REG_TDT(driver->eth_data, 0) = (buf_num + num) % driver->desc->tx.count;
    /* Make sure these updates get seen */
    __sync_synchronize();
}

static void ready_rx_desc(int buf_num, int tx_desc_wrap, struct eth_driver *driver) {
    REG_RDT(driver->eth_data, 0) = (REG_RDT(driver->eth_data, 0) + 1) % driver->desc->rx.count;
    __sync_synchronize();
}

static int is_tx_desc_ready(int buf_num, struct desc *desc) {
    volatile struct tx_ldesc *ring = desc->tx.ring.virt;
    return !(ring[buf_num].STA & TX_DD);
}

static int is_rx_desc_empty(int buf_num, struct desc *desc) {
    volatile struct rx_ldesc *ring = desc->rx.ring.virt;
    return !(ring[buf_num].status & RX_DD);
}

static void set_tx_desc_buf(int buf_num, dma_addr_t buf, int len, int tx_desc_wrap, int tx_last_section, struct desc *desc) {
    struct tx_ldesc *d = desc->tx.ring.virt;
    d[buf_num].bufferAddress = buf.phys;
    d[buf_num].bufferAddressHigh = 0;
    d[buf_num].length = len;
    d[buf_num].CSO = 0;
    d[buf_num].CMD = 0b1010 | (tx_last_section ? TX_EOP : 0);
    d[buf_num].STA = 0;
    d[buf_num].CSS = 0;
    d[buf_num].VLAN = 0;
}

static void set_rx_desc_buf(int buf_num, dma_addr_t buf, int len, struct desc *desc) {
    struct rx_ldesc *d = desc->rx.ring.virt;
    d[buf_num].bufferAddress = buf.phys;
    d[buf_num].bufferAddressHigh = 0;
    d[buf_num].length = len;
    d[buf_num].packetChecksum = 0;
    d[buf_num].status = 0;
    d[buf_num].error = 0;
    d[buf_num].VLAN = 0;
    __sync_synchronize();
}

static int get_rx_buf_len(int buf_num, struct desc *desc) {
    volatile struct rx_ldesc *ring = desc->rx.ring.virt;
    return ring[buf_num].length;
}

static int get_rx_desc_error(int buf_num, struct desc *desc) {
    volatile struct rx_ldesc *ring = desc->rx.ring.virt;
    return ring[buf_num].error & ~RX_E_RSV; // Ignore the reserved error bit
}

static struct raw_desc_funcs desc_fns = {
    .create_tx_descs = create_tx_descs,
    .create_rx_descs = create_rx_descs,
    .reset_tx_descs = reset_tx_descs,
    .reset_rx_descs = reset_rx_descs,
    .ready_tx_desc = ready_tx_desc,
    .ready_rx_desc = ready_rx_desc,
    .is_tx_desc_ready = is_tx_desc_ready,
    .is_rx_desc_empty = is_rx_desc_empty,
    .set_tx_desc_buf = set_tx_desc_buf,
    .set_rx_desc_buf = set_rx_desc_buf,
    .get_rx_buf_len = get_rx_buf_len,
    .get_rx_desc_error = get_rx_desc_error
};

struct eth_driver*
ethif_e82580_init(int dev_id, ps_io_ops_t io_ops, void *bar0) {
    struct eth_driver *driver = malloc(sizeof(*driver));
    assert(driver);
    initialize(bar0);
    initialize_transmit(bar0);
    initialize_receive(bar0);
    driver->eth_data = bar0;
    driver->r_fn = &iface_fns;
    driver->d_fn = &desc_fns;
    driver->desc = desc_init(&io_ops.dma_manager, CONFIG_LIB_ETHDRIVER_RX_DESC_COUNT,
                         CONFIG_LIB_ETHDRIVER_TX_DESC_COUNT, NUM_DMA_BUFS, 2048, 128, driver);
    assert(driver->desc);
    return driver;
}

