/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include <ethdrivers/intel.h>
#include "../../descriptors.h"
#include "../../raw_iface.h"
#include <ethdrivers/lwip_iface.h>
#include <netif/etharp.h>
#include "../../dma_buffers.h"
#include <assert.h>

typedef enum e1000_family {
    e1000_82580 = 1,
    e1000_82574
} e1000_family_t;

// TX Descriptor Status Bits
#define TX_DD BIT(0) /* Descriptor Done */
#define TX_EOP BIT(0) /* End of Packet */

// RX Descriptor Status Bits
#define RX_DD BIT(0) /* Descriptor Done */
#define RX_E_RSV BIT(3) /* Error Reserved bit */


#define REG(x,y) (*(volatile uint32_t*)(((uintptr_t)(x)->iobase) + (y)))

#define REG_CTRL(x) REG(x, 0x0)
#define REG_82580_IMC(x) REG(x, 0x150c)
#define REG_82574_IMC(x) REG(x, 0xD8)
#define REG_STATUS(x) REG(x, 0x8)
#define REG_82580_TXDCTL(x, y) REG(x, 0xE028 + 0x40 * (y))
#define REG_82574_TXDCTL(x, y) REG(x, 0x3828 + 0x100 * (y))
#define REG_MTA(x, y) REG(x, 0x5200 + 4 * (y))
#define REG_RCTL(x) REG(x, 0x100)
#define REG_EERD(x) REG(x, 0x14)
#define REG_TCTL(x) REG(x, 0x0400)
#define REG_82580_TDBAL(x, y) REG(x, 0xE000 + (y) * 0x40)
#define REG_82574_TDBAL(x, y) REG(x, 0x3800 + (y) * 0x100)
#define REG_82580_TDBAH(x, y) REG(x, 0xE004 + (y) * 0x40)
#define REG_82574_TDBAH(x, y) REG(x, 0x3804 + (y) * 0x100)
#define REG_82580_TDLEN(x, y) REG(x, 0xE008 + (y) * 0x40)
#define REG_82574_TDLEN(x, y) REG(x, 0x3808 + (y) * 0x100)
#define REG_82580_TDH(x, y) REG(x, 0xE010 + (y) * 0x40)
#define REG_82574_TDH(x, y) REG(x, 0x3810 + (y) * 0x100)
#define REG_82580_TDT(x, y) REG(x, 0xE018 + (y) * 0x40)
#define REG_82574_TDT(x, y) REG(x, 0x3818 + (y) * 0x100)
#define REG_82580_RDBAL(x, y) REG(x, 0xC000 + (y) * 0x40)
#define REG_82574_RDBAL(x, y) REG(x, 0x2800 + (y) * 0x100)
#define REG_82580_RDBAH(x, y) REG(x, 0xC004 + (y) * 0x40)
#define REG_82574_RDBAH(x, y) REG(x, 0x2804 + (y) * 0x100)
#define REG_82580_RDLEN(x, y) REG(x, 0xC008 + (y) * 0x40)
#define REG_82574_RDLEN(x, y) REG(x, 0x2808 + (y) * 0x100)
#define REG_82580_RDH(x, y) REG(x, 0xc010 + (y) * 0x40)
#define REG_82574_RDH(x, y) REG(x, 0x2810 + (y) * 0x100)
#define REG_82580_RDT(x, y) REG(x, 0xc018 + (y) * 0x40)
#define REG_82574_RDT(x, y) REG(x, 0x2818 + (y) * 0x100)
#define REG_82580_RXDCTL(x, y) REG(x, 0xc028 + (y) * 0x40)
#define REG_82580_IMS(x) REG(x, 0x1508)
#define REG_82574_IMS(x) REG(x, 0xD0)
#define REG_82580_ICR(x) REG(x, 0x1500)
#define REG_82574_ICR(x) REG(x, 0xC0)
#define REG_TIPG(x) REG(x, 0x410)

#define IMC_82580_RESERVED_BITS (BIT(1) | BIT(3) | BIT(5) | BIT(9) | BIT(15) | BIT(16) | BIT(17) | BIT(21) | BIT(23) | BIT(27) | BIT(31))
#define IMC_82574_RESERVED_BITS (BIT(3) | BIT(5) | BIT(8) | (0b11111 << 10) | BIT(19) | (0b1111111 << 25))

#define CTRL_82580_RESERVED_BITS (BIT(1) | BIT(3) | BIT(4) | BIT(5) | BIT(10) | BIT(13) | BIT(14) | BIT(15) | BIT(24) | BIT(25))
#define CTRL_82574_RESERVED_BITS (BIT(1) | (0b11 << 3) | BIT(7) | BIT(10) | (0b1111111 << 13) | (0b11111 << 21) | BIT(29))
#define CTRL_SLU BIT(6)
#define CTRL_RST BIT(26)

#define STATUS_82580_LAN_ID_OFFSET 2
#define STATUS_82580_LAN_ID_MASK (BIT(2) | BIT(3))

#define RCTL_82580_RESERVED_BITS (BIT(0) | BIT(10) | BIT(11) | BIT(27) | BIT(28) | BIT(29) | BIT(30) | BIT(31))
#define RCTL_82574_RESERVED_BITS (BIT(0) | BIT(14) | BIT(21) | BIT(24) | BIT(31))
#define RCTL_EN BIT(1)
#define RCTL_UPE BIT(3)
#define RCTL_MPE BIT(4)
#define RCTL_BAM BIT(15)

#define TXDCTL_82580_RESERVED_BITS (0)
#define TXDCTL_82574_RESERVED_BITS (0)
#define TXDCTL_82580_ENABLE BIT(25)
#define TXDCTL_82574_BIT_THAT_SHOULD_BE_1 BIT(22)
#define TXDCTL_82574_GRAN BIT(24)

#define EERD_START BIT(0)
#define EERD_DONE BIT(1)
#define EERD_ADDR_OFFSET 2

#define TCTL_82580_RESERVED_BITS (BIT(0) | BIT(2))
#define TCTL_82574_RESERVED_BITS (BIT(0) | BIT(2) |BIT(31))
#define TCTL_EN BIT(1)
#define TCTL_PSP BIT(3)
#define TCTL_82574_COLD_BITS(x) (((x) & 0b1111111111) << 12)
#define TCTL_82574_UNORTX BIT(25)
#define TCTL_82574_TXDSCMT_BITS(x) (((x) & 3) << 26)
#define TCTL_82574_RRTHRESH_BITS(x) (((x) & 3) << 29)
#define TCTL_82574_MULR BIT(28)

#define RXDCTL_82580_RESERVED_BITS (0)
#define RXDCTL_82580_ENABLE BIT(25)

#define IMS_82580_RXDW BIT(7)
#define IMS_82574_RXQ0 BIT(20)

#define ICR_82580_RXDW BIT(7)
#define ICR_82574_RXQ0 BIT(20)

#define EEPROM_82580_LAN(id, x) ( ((id) ? 0 : 0x40) * (id) + (x))

#define MTA_LENGTH 128

struct __attribute((packed)) legacy_tx_ldesc {
    uint32_t bufferAddress;
    uint32_t bufferAddressHigh;
    uint32_t length: 16;
    uint32_t CSO: 8;
    uint32_t CMD: 8;
    uint32_t STA: 4;
    uint32_t ExtCMD : 4; /* This is reserved on the 82580 */
    uint32_t CSS: 8;
    uint32_t VLAN: 16;
};

struct __attribute((packed)) legacy_rx_ldesc {
    uint32_t bufferAddress;
    uint32_t bufferAddressHigh;
    uint32_t length: 16;
    uint32_t packetChecksum: 16;
    uint32_t status: 8;
    uint32_t error: 8;
    uint32_t VLAN: 16;
};

typedef struct e1000_dev {
    e1000_family_t family;
    void *iobase;
}e1000_dev_t;

static void disable_all_interrupts(e1000_dev_t *dev) {
    switch(dev->family) {
    case e1000_82580:
        REG_82580_IMC(dev) = ~IMC_82580_RESERVED_BITS;
        break;
    case e1000_82574:
        REG_82574_IMC(dev) = ~IMC_82574_RESERVED_BITS;
        break;
    }
}

static uint32_t ctrl_reserved[] = {
    [e1000_82580] = CTRL_82580_RESERVED_BITS,
    [e1000_82574] = CTRL_82574_RESERVED_BITS
};

static void reset_device(e1000_dev_t *dev) {
    assert(dev->family < ARRAY_SIZE(ctrl_reserved));
    REG_CTRL(dev) = (REG_CTRL(dev) & (ctrl_reserved[dev->family])) | CTRL_RST;
}

static void initialize_control(e1000_dev_t *dev) {
    uint32_t temp = REG_CTRL(dev);
    assert(dev->family < ARRAY_SIZE(ctrl_reserved));
    temp &= ~ctrl_reserved[dev->family];
    temp |= CTRL_SLU;
    REG_CTRL(dev) = temp;
}

static void initialize(e1000_dev_t *dev) {
    disable_all_interrupts(dev);
    reset_device(dev);
    disable_all_interrupts(dev);
    initialize_control(dev);
}

static void initialise_TXDCTL(e1000_dev_t *dev) {
    uint32_t temp;
    switch(dev->family) {
    case e1000_82580:
        /* Nothing to do here, we will enable the transmit queue itself later,
         * but that has to be done after we setup the descriptors */
        break;
    case e1000_82574:
        temp = REG_82574_TXDCTL(dev, 0);
        temp &= ~TXDCTL_82574_RESERVED_BITS;
        /* set the bit that we have to set */
        temp |= TXDCTL_82574_BIT_THAT_SHOULD_BE_1;
        /* set gran to 1 */
        temp |= TXDCTL_82574_GRAN;
        REG_82574_TXDCTL(dev, 0) = temp;
        break;
    default:
        assert(!"Unknown device");
        break;
    }
}

static void initialise_TCTL(e1000_dev_t *dev)
{
    uint32_t temp = REG_TCTL(dev);
    switch(dev->family) {
    case e1000_82580:
        temp &= TCTL_82580_RESERVED_BITS;
        /* Enable transmit, this is safe here as we will enable per
         * transmit queue later on */
        temp |= TCTL_EN;
        break;
    case e1000_82574:
        temp &= TCTL_82574_RESERVED_BITS;
        /* Do not enable transmit as we have no per queue transmit
         * enable. We will enable this later */
        /* set cold to 0x3f for FD (or 0x1FF for HD) */
        temp |= TCTL_82574_COLD_BITS(0x3f);
        /* Clear UNORTX, TXDSCMT, RRTHRESH */
        temp &= ~TCTL_82574_UNORTX;
        temp &= ~TCTL_82574_TXDSCMT_BITS(0b11);
        temp &= ~TCTL_82574_RRTHRESH_BITS(0b11);
        /* allow multiple transmit requests from hardware at once */
        temp |= TCTL_82574_MULR;
        break;
    default:
        assert(!"Unknown device");
    }
    temp |= TCTL_PSP;
    REG_TCTL(dev) = temp;
}

static void initialise_TIPG(e1000_dev_t *dev) {
    switch(dev->family) {
    case e1000_82580:
        /* use defaults */
        break;
    case e1000_82574:
        /* Write in recommended value from manual */
        REG_TIPG(dev) = 0x00602006;
        break;
    default:
        assert(!"Unknown device");
    }
}

static void initialize_transmit(e1000_dev_t *dev) {
    initialise_TXDCTL(dev);
    initialise_TCTL(dev);
    initialise_TIPG(dev);
}

static void initialize_RCTL(e1000_dev_t *dev) {
    uint32_t temp = REG_RCTL(dev);
    switch(dev->family) {
    case e1000_82580:
        temp &= ~RCTL_82580_RESERVED_BITS;
        /* Enable receive. This is safe as we have to enable receive
         * per queue later on */
        temp |= RCTL_EN;
        break;
    case e1000_82574:
        temp &= ~RCTL_82574_RESERVED_BITS;
        /* Do not enable receive as we have no per queue receive enable.
         * we will enable this later */
        break;
    }
    /* Don't think we want promiscuous mode */
//    temp |= RCTL_UPE;
//    temp |= RCTL_MPE;
    /* Or to accept broadcast packets */
//    temp |= RCTL_BAM;
    /* defaults for everything else will give us 2K pages, which is what we want */
    REG_RCTL(dev) = temp;
}

static void initialize_receive(e1000_dev_t *dev) {
    /* zero the MTA */
    int i;
    for (i = 0; i < MTA_LENGTH; i++) {
        REG_MTA(dev, i) = 0;
    }
    initialize_RCTL(dev);
}

void handle_irq(struct netif *netif, int irq) {
    e1000_dev_t *dev = ((struct eth_driver*)netif->state)->eth_data;
    uint32_t icr;
    switch(dev->family) {
    case e1000_82580:
        icr = REG_82580_ICR(dev);
        if (icr & ICR_82580_RXDW) {
            while(ethif_input(netif));
        }
        break;
    case e1000_82574:
        icr = REG_82574_ICR(dev);
        if(icr & ICR_82574_RXQ0) {
            while(ethif_input(netif));
        }
        __sync_synchronize();
        REG_82574_ICR(dev) = 0xffffff;
        break;
    default:
        assert(!"Unknown device");
    }
}

const int* enable_irq(struct eth_driver *eth_driver, int *nirqs) {
    e1000_dev_t *dev = (e1000_dev_t*)eth_driver->eth_data;
    uint32_t temp;
    switch(dev->family) {
    case e1000_82580:
        /* Ack any existing interrupts by reading the ICR */
        temp = REG_82580_ICR(dev);
        (void)temp;
        /* Now enable interrupts */
        REG_82580_IMS(dev) = IMS_82580_RXDW;
        break;
    case e1000_82574:
        /* Ack any existing interrupts by writing ICR */
        REG_82574_ICR(dev) = 0xffffff;
        /* now enable interrupts */
        REG_82574_IMS(dev) |= IMS_82574_RXQ0;
        break;
    default:
        assert(!"Unknown device");
        break;
    }
    *nirqs = 0;
    return NULL;
}

void print_state(struct eth_driver *eth_driver) {
}

static uint16_t read_eeprom(e1000_dev_t *dev, uint16_t reg) {
    REG_EERD(dev) = EERD_START | (reg << EERD_ADDR_OFFSET);
    uint32_t val;
    while ( ((val = REG_EERD(dev)) & EERD_DONE) == 0);
    return val >> 16;
}

void eth_get_mac(e1000_dev_t *dev, uint8_t *hwaddr) {
    /* read the first 3 shorts of the eeprom */
    uint16_t mac[3];
    uint16_t base = 0x00;
    switch(dev->family) {
    case e1000_82580: {
        /* read our LAN ID so we know what port we are */
        int id = (REG_STATUS(dev) & STATUS_82580_LAN_ID_MASK) >> STATUS_82580_LAN_ID_OFFSET;
        base = EEPROM_82580_LAN(id, 0x00);
        break;
    }
    case e1000_82574:
        /* Single port, so default base is fine */
        break;
    default:
        assert(!"Unknown device");
        return;
    }
    mac[0] = read_eeprom(dev, base + 0x00);
    mac[1] = read_eeprom(dev, base + 0x01);
    mac[2] = read_eeprom(dev, base + 0x02);
    hwaddr[0] = mac[0] & MASK(8);
    hwaddr[1] = mac[0] >> 8;
    hwaddr[2] = mac[1] & MASK(8);
    hwaddr[3] = mac[1] >> 8;
    hwaddr[4] = mac[2] & MASK(8);
    hwaddr[5] = mac[2] >> 8;
}

void low_level_init(struct netif *netif) {
    e1000_dev_t *dev = (e1000_dev_t*)((struct eth_driver*)netif->state)->eth_data;
#if LWIP_NETIF_HOSTNAME
    switch(dev->family) {
    case e1000_82580:
        netif->hostname = "82580 Device";
        break;
    case e1000_82574:
        netif->hostname =" 82574 Device";
        break;
    default:
        assert(!"Unknown device");
        netif->hostname = "Unknown intel";
    }
#endif
    netif->name[0] = 'P';
    netif->name[1] = 'C';
    netif->hwaddr_len = ETHARP_HWADDR_LEN;
    eth_get_mac(dev, netif->hwaddr);
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
    return dma_alloc_pin(dma_man, sizeof(struct legacy_tx_ldesc) * count, 1, 128);
}

static dma_addr_t create_rx_descs(ps_dma_man_t *dma_man, int count) {
    return dma_alloc_pin(dma_man, sizeof(struct legacy_rx_ldesc) * count, 1, 128);
}

static void disable_transmit(e1000_dev_t *dev) {
    switch(dev->family) {
    case e1000_82580:
        REG_82580_TXDCTL(dev, 0) &= ~(TXDCTL_82580_RESERVED_BITS | TXDCTL_82580_ENABLE);
        break;
    case e1000_82574:
        REG_TCTL(dev) &= ~(TCTL_82574_RESERVED_BITS | TCTL_EN);
        break;
    default:
        assert(!"Unkown device");
    }
}

static void enable_transmit(e1000_dev_t *dev) {
    switch(dev->family) {
    case e1000_82580:
        REG_82580_TXDCTL(dev, 0) = (REG_82580_TXDCTL(dev, 0) & (~TXDCTL_82580_RESERVED_BITS)) | (TXDCTL_82580_ENABLE);
        break;
    case e1000_82574:
        REG_TCTL(dev) = (REG_TCTL(dev) & (~TCTL_82574_RESERVED_BITS)) | (TCTL_EN);
        break;
    default:
        assert(!"Unkown device");
    }
}

static void set_tx_ring(e1000_dev_t *dev, uint32_t phys) {
    switch(dev->family) {
    case e1000_82580:
        REG_82580_TDBAL(dev, 0) = phys;
        REG_82580_TDBAH(dev, 0) = 0;
        break;
    case e1000_82574:
        REG_82574_TDBAL(dev, 0) = phys;
        REG_82574_TDBAH(dev, 0) = 0;
        break;
    default:
        assert(!"Unknown device");
    }
}

static void set_tdh(e1000_dev_t *dev, uint32_t val) {
    switch(dev->family) {
    case e1000_82580:
        REG_82580_TDH(dev, 0) = val;
        break;
    case e1000_82574:
        REG_82574_TDH(dev, 0) = val;
        break;
    default:
        assert(!"Unknown device");
    }
}

static void set_tdt(e1000_dev_t *dev, uint32_t val) {
    switch(dev->family) {
    case e1000_82580:
        REG_82580_TDT(dev, 0) = val;
        break;
    case e1000_82574:
        REG_82574_TDT(dev, 0) = val;
        break;
    default:
        assert(!"Unknown device");
    }
}

static void set_tdlen(e1000_dev_t *dev, uint32_t val) {
    switch(dev->family) {
    case e1000_82580:
        REG_82580_TDLEN(dev, 0) = val;
        break;
    case e1000_82574:
        REG_82574_TDLEN(dev, 0) = val;
        break;
    default:
        assert(!"Unknown device");
    }
}

static void reset_tx_descs(struct eth_driver *driver) {
    e1000_dev_t *dev = (e1000_dev_t*)driver->eth_data;
    int i;
    struct desc *desc = driver->desc;
    struct legacy_tx_ldesc *d = desc->tx.ring.virt;
    /* First ensure transmit is not enabled */
    disable_transmit(dev);
    /* Zero the descriptors */
    for (i = 0; i < desc->tx.count; i++) {
        d[i].length = 0;
        d[i].CSO = 0;
        d[i].CMD = 0;
        d[i].STA = 1;
        d[i].ExtCMD = 0;
        d[i].CSS = 0;
        d[i].VLAN = 0;
    }

    /* Ensure updates are visible */
    __sync_synchronize();

    /* Tell the hardware where the ring is */
    set_tx_ring(dev, (uint32_t)desc->tx.ring.phys);

    /* Setup transmit queues to be initially empty */
    set_tdh(dev, 0);
    set_tdt(dev, 0);

    /* Set length of the ring */
    size_t desc_bytes = desc->tx.count * sizeof(struct legacy_tx_ldesc);
    assert(desc_bytes % 128 == 0); // tdlen must be 128 byte aligned
    set_tdlen(dev, desc_bytes);

    /* Sync everything we just did */
    __sync_synchronize();

    /* Enable transmit */
    enable_transmit(dev);
}

static void disable_receive(e1000_dev_t *dev) {
    switch(dev->family) {
    case e1000_82580:
        REG_82580_RXDCTL(dev, 0) &= ~(RXDCTL_82580_RESERVED_BITS | RXDCTL_82580_ENABLE);
        break;
    case e1000_82574:
        REG_RCTL(dev) &= ~(RCTL_82580_RESERVED_BITS | RCTL_EN);
        break;
    default:
        assert(!"Unknown device");
    }
}

static void enable_receive(e1000_dev_t *dev) {
    switch(dev->family) {
    case e1000_82580:
        REG_82580_RXDCTL(dev, 0) = (REG_82580_RXDCTL(dev, 0) & (~RXDCTL_82580_RESERVED_BITS)) | (RXDCTL_82580_ENABLE);
        break;
    case e1000_82574:
        REG_RCTL(dev) = (REG_RCTL(dev) & (~RCTL_82580_RESERVED_BITS)) | (RCTL_EN);
        break;
    default:
        assert(!"Unknown device");
    }
}

static void set_rx_ring(e1000_dev_t *dev, uint32_t phys) {
    switch(dev->family) {
    case e1000_82580:
        REG_82580_RDBAL(dev, 0) = phys;
        REG_82580_RDBAH(dev, 0) = 0;
        break;
    case e1000_82574:
        REG_82574_RDBAL(dev, 0) = phys;
        REG_82574_RDBAH(dev, 0) = 0;
        break;
    default:
        assert(!"Unknown device");
    }
}

static void set_rdlen(e1000_dev_t *dev, uint32_t val) {
    switch(dev->family) {
    case e1000_82580:
        REG_82580_RDLEN(dev, 0) = val;
        break;
    case e1000_82574:
        REG_82574_RDLEN(dev, 0) = val;
        break;
    default:
        assert(!"Unknown device");
    }
}

static void set_rdt(e1000_dev_t *dev, uint32_t val) {
    switch(dev->family) {
    case e1000_82580:
        REG_82580_RDT(dev, 0) = val;
        break;
    case e1000_82574:
        REG_82574_RDT(dev, 0) = val;
        break;
    default:
        assert(!"Unknown device");
    }
}

static uint32_t read_rdh(e1000_dev_t *dev) {
    switch(dev->family) {
    case e1000_82580:
        return REG_82580_RDH(dev, 0);
    case e1000_82574:
        return REG_82574_RDH(dev, 0);
    default:
        assert(!"Unknown device");
        return 0;
    }
}

static uint32_t read_rdt(e1000_dev_t *dev) {
    switch(dev->family) {
    case e1000_82580:
        return REG_82580_RDT(dev, 0);
    case e1000_82574:
        return REG_82574_RDT(dev, 0);
    default:
        assert(!"Unknown device");
        return 0;
    }
}

static void reset_rx_descs(struct eth_driver *driver) {
    e1000_dev_t *dev = (e1000_dev_t*)driver->eth_data;
    int i;
    struct desc *desc = driver->desc;
    struct legacy_rx_ldesc *d = desc->rx.ring.virt;
    /* First ensure receive is not enabled */
    disable_receive(dev);
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
    set_rx_ring(dev, (uint32_t)desc->rx.ring.phys);

    /* Set length of the ring */
    size_t desc_bytes = desc->rx.count * sizeof(struct legacy_rx_ldesc);
    assert(desc_bytes % 128 == 0); // tdlen must be 128 byte aligned
    set_rdlen(dev, desc_bytes);

    __sync_synchronize();

    /* Enable receive queue */
    enable_receive(dev);

    __sync_synchronize();

    /* Set the tail to initialize be full */
    set_rdt(dev, (read_rdh(dev) + desc->rx.count - 1) % desc->rx.count);

    __sync_synchronize();
}

static void ready_tx_desc(int buf_num, int num, struct eth_driver *driver) {
    e1000_dev_t *dev = (e1000_dev_t*)driver->eth_data;
    /* Synchronize any previous updates before moving the tail register */
    __sync_synchronize();
    /* Move the tail reg. */
    set_tdt(dev,(buf_num + num) % driver->desc->tx.count);
    /* Make sure these updates get seen */
    __sync_synchronize();
}

static void ready_rx_desc(int buf_num, int tx_desc_wrap, struct eth_driver *driver) {
    e1000_dev_t *dev = (e1000_dev_t*)driver->eth_data;
    set_rdt(dev, (read_rdt(dev) + 1) % driver->desc->rx.count);
    __sync_synchronize();
}

static int is_tx_desc_ready(int buf_num, struct desc *desc) {
    volatile struct legacy_tx_ldesc *ring = desc->tx.ring.virt;
    return !(ring[buf_num].STA & TX_DD);
}

static int is_rx_desc_empty(int buf_num, struct desc *desc) {
    volatile struct legacy_rx_ldesc *ring = desc->rx.ring.virt;
    return !(ring[buf_num].status & RX_DD);
}

static void set_tx_desc_buf(int buf_num, dma_addr_t buf, int len, int tx_desc_wrap, int tx_last_section, struct desc *desc) {
    struct legacy_tx_ldesc *d = desc->tx.ring.virt;
    d[buf_num].bufferAddress = buf.phys;
    d[buf_num].bufferAddressHigh = 0;
    d[buf_num].length = len;
    d[buf_num].CSO = 0;
    d[buf_num].CMD = 0b1010 | (tx_last_section ? TX_EOP : 0);
    d[buf_num].STA = 0;
    d[buf_num].ExtCMD = 0;
    d[buf_num].CSS = 0;
    d[buf_num].VLAN = 0;
}

static void set_rx_desc_buf(int buf_num, dma_addr_t buf, int len, struct desc *desc) {
    struct legacy_rx_ldesc *d = desc->rx.ring.virt;
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
    volatile struct legacy_rx_ldesc *ring = desc->rx.ring.virt;
    return ring[buf_num].length;
}

static int get_rx_desc_error(int buf_num, struct desc *desc) {
    volatile struct legacy_rx_ldesc *ring = desc->rx.ring.virt;
    return ring[buf_num].error & ~RX_E_RSV; // Ignore the reserved error bit
}

static struct raw_desc_funcs legacy_desc_fns = {
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

static struct eth_driver *
common_init(ps_io_ops_t io_ops, void *bar0, e1000_family_t family) {
    struct eth_driver *driver = malloc(sizeof(*driver));
    if (!driver) {
        return NULL;
    }
    e1000_dev_t *dev = malloc(sizeof(*dev));
    if (!dev) {
        free(driver);
        return NULL;
    }
    dev->family = family;
    dev->iobase = bar0;

    driver->eth_data = dev;
    driver->r_fn = &iface_fns;
    driver->d_fn = &legacy_desc_fns;

    initialize(dev);
    initialize_transmit(dev);
    initialize_receive(dev);
    driver->desc = desc_init(&io_ops.dma_manager, CONFIG_LIB_ETHDRIVER_RX_DESC_COUNT,
                         CONFIG_LIB_ETHDRIVER_TX_DESC_COUNT, 2048, 128, driver);
    if (!driver->desc) {
        /* Reset device */
        disable_all_interrupts(dev);
        reset_device(dev);
        /* Free memory */
        free(dev);
        free(driver);
        return NULL;
    }
    return driver;
}

struct eth_driver*
ethif_e82580_init(ps_io_ops_t io_ops, void *bar0) {
    return common_init(io_ops, bar0, e1000_82580);
}

struct eth_driver*
ethif_e82574_init(ps_io_ops_t io_ops, void *bar0) {
    return common_init(io_ops, bar0, e1000_82574);
}
