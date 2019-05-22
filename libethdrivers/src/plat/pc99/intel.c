/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */

#include <ethdrivers/gen_config.h>
#include <ethdrivers/intel.h>
#include <assert.h>
#include <ethdrivers/helpers.h>

typedef enum e1000_family {
    e1000_82580 = 1,
    e1000_82574
} e1000_family_t;

/* An alignment of 128 bytes is required for most structures by the hardware, except for actual packets */
#define DMA_ALIGN 128
/* This driver is hard coded to use 2k buffers, don't just change this */
#define BUF_SIZE 2048

// TX Descriptor Status Bits
#define TX_DD BIT(0) /* Descriptor Done */
/* Descriptor CMD Bits */
#define TX_CMD_EOP BIT(0) /* End of Packet */
#define TX_CMD_IFCS BIT(1) /* Insert FCS (CRC) */
#define TX_CMD_RS BIT(3) /* Report status */
#define TX_CMD_IDE BIT(7) /* Interrupt Delay Enable */

// RX Descriptor Status Bits
#define RX_DD BIT(0) /* Descriptor Done */
#define RX_EOP BIT(1) /* End of Packet */

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
#define REG_82574_RXDCTL(x, y) REG(x, 0x2828 + (y) * 0x100)
#define REG_82580_IMS(x) REG(x, 0x1508)
#define REG_82574_IMS(x) REG(x, 0xD0)
#define REG_82580_ICR(x) REG(x, 0x1500)
#define REG_82574_ICR(x) REG(x, 0xC0)
#define REG_TIPG(x) REG(x, 0x410)
#define REG_82574_RDTR(x) REG(x, 0x2820)
#define REG_82574_RADV(x) REG(x, 0x282c)
#define REG_82574_RAID(x) REG(x, 0x2c08)
#define REG_82574_TIDV(x) REG(x, 0x3820)
#define REG_82574_TADV(x) REG(x, 0x382c)
#define REG_82574_PBA(x) REG(x, 0x1000)
#define REG_MDIC(x) REG(x, 0x20)
#define REG_82574_GCR(x) REG(x, 0x5B00)
#define REG_82574_FCT(x) REG(x, 0x030)
#define REG_82574_FCAL(x) REG(x, 0x028)
#define REG_82574_FCAH(x) REG(x, 0x02c)

#define IMC_82580_RESERVED_BITS ((uint32_t)(BIT(1) | BIT(3) | BIT(5) | BIT(9) | BIT(15) | BIT(16) | BIT(17) | BIT(21) | BIT(23) | BIT(27) | BIT(31)))
#define IMC_82574_RESERVED_BITS (BIT(3) | BIT(5) | BIT(8) | (0b11111 << 10) | BIT(19) | (0b1111111 << 25))

#define CTRL_82580_RESERVED_BITS (BIT(24) | BIT(25))
#define CTRL_82574_RESERVED_BITS (BIT(1) | (0b11 << 3) | BIT(7) | BIT(10) | (0b1111111 << 13) | (0b11111 << 21) | BIT(29))
#define CTRL_SLU BIT(6)
#define CTRL_RST BIT(26)

#define STATUS_LU BIT(1)

#define STATUS_82580_LAN_ID_OFFSET 2
#define STATUS_82580_LAN_ID_MASK (BIT(2) | BIT(3))

#define RCTL_82580_RESERVED_BITS (BIT(0) | BIT(10) | BIT(11) | BIT(27) | BIT(28) | BIT(29) | BIT(30) | BIT(31))
#define RCTL_82574_RESERVED_BITS (BIT(0) | BIT(14) | BIT(21) | BIT(24) | BIT(31))
#define RCTL_82574_RDMTS_OFFSET (8)
#define RCTL_82574_RDMTS_1_4 (0b10 << RCTL_82574_RDMTS_OFFSET)
#define RCTL_EN BIT(1)
#define RCTL_UPE BIT(3)
#define RCTL_MPE BIT(4)
#define RCTL_BAM BIT(15)

#define TXDCTL_82580_RESERVED_BITS (0)
#define TXDCTL_82574_RESERVED_BITS (0)
#define TXDCTL_82580_ENABLE BIT(25)
#define TXDCTL_82574_BIT_THAT_SHOULD_BE_1 BIT(22)
#define TXDCTL_82574_GRAN BIT(24)
#define TXDCTL_82574_PTHRESH_OFFSET 0
#define TXDCTL_82574_HTHRESH_OFFSET 8
#define TXDCTL_82574_WTHRESH_OFFSET 16

#define EERD_START BIT(0)
#define EERD_DONE BIT(1)
#define EERD_ADDR_OFFSET 2

#define TCTL_82580_RESERVED_BITS (BIT(0) | BIT(2))
#define TCTL_82574_RESERVED_BITS (BIT(0) | BIT(2) |BIT(31))
#define TCTL_EN BIT(1)
#define TCTL_PSP BIT(3)
#define TCTL_CT_BITS(x) ((x) << 4)
#define TCTL_82574_COLD_BITS(x) (((x) & 0b1111111111) << 12)
#define TCTL_82574_UNORTX BIT(25)
#define TCTL_82574_TXDSCMT_BITS(x) (((x) & 3) << 26)
#define TCTL_82574_RRTHRESH_BITS(x) (((x) & 3) << 29)
#define TCTL_82574_MULR BIT(28)

#define RXDCTL_82574_RESERVED_BITS (0)
#define RXDCTL_82574_PTHRESH_OFFSET (0)
#define RXDCTL_82574_HTHRESH_OFFSET (8)
#define RXDCTL_82574_WTHRESH_OFFSET (16)
#define RXDCTL_82574_GRAN BIT(24)
#define RXDCTL_82580_RESERVED_BITS (0)
#define RXDCTL_82580_ENABLE BIT(25)

#define IMS_82580_RXDW BIT(7)
#define IMS_82580_TXDW BIT(0)
#define IMS_82580_GPHY BIT(10)
#define IMS_82574_RXQ0 BIT(20)
#define IMS_82574_RXTO BIT(7)
#define IMS_82574_RXDMT0 BIT(4)
#define IMS_82574_TXDW BIT(0)
#define IMS_82574_ACK BIT(17)
#define IMS_82574_LSC BIT(2)

#define ICR_82580_RXDW BIT(7)
#define ICR_82580_TXDW BIT(0)
#define ICR_82574_RXQ0 BIT(20)
#define ICR_82580_GPHY BIT(10)
#define ICR_82574_RXTO BIT(7)
#define ICR_82574_RXDMT0 BIT(4)
#define ICR_82574_TXDW BIT(0)
#define ICR_82574_ACK BIT(17)
#define ICR_82574_LSC BIT(2)

#define EEPROM_82580_LAN(id, x) ( ((id) ? 0 : 0x40) * (id) + (x))

#define MTA_LENGTH 128

struct __attribute((packed)) legacy_tx_ldesc {
    uint64_t bufferAddress;
    uint32_t length: 16;
    uint32_t CSO: 8;
    uint32_t CMD: 8;
    uint32_t STA: 4;
    uint32_t ExtCMD : 4; /* This is reserved on the 82580 */
    uint32_t CSS: 8;
    uint32_t VLAN: 16;
};

struct __attribute((packed)) legacy_rx_ldesc {
    uint64_t bufferAddress;
    uint32_t length: 16;
    uint32_t packetChecksum: 16;
    uint32_t status: 8;
    uint32_t error: 8;
    uint32_t VLAN: 16;
};

typedef struct e1000_dev {
    e1000_family_t family;
    void *iobase;
    /* shadow the value of descriptor tails so we don't have to re-read it to increment */
    uint32_t rdt;
    uint32_t tdt;
    /* track what we think the values of rdh and tdh are in the hardware so we can find
     * complete transmit and receive descriptors */
    uint32_t tdh;
    uint32_t rdh;
    /* descriptor rings */
    volatile struct legacy_rx_ldesc *rx_ring;
    unsigned int rx_size;
    unsigned int rx_remain;
    void **rx_cookies;
    volatile struct legacy_tx_ldesc *tx_ring;
    unsigned int tx_size;
    unsigned int tx_remain;
    void **tx_cookies;
    unsigned int *tx_lengths;
    uint32_t tx_cmd_bits;
    /* whether we believe the link is up or not */
    int link_up;
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

static void reset_device(e1000_dev_t *dev) {
    uint32_t val = CTRL_RST;
    if (dev->family == e1000_82574) {
        /* 82574 docs say that bit 3 must be set, so set it and perform the reset */
        val |= BIT(3);
    }
    REG_CTRL(dev) = val;
    /* wait approximately 1us before checking for reset completion */
    for(volatile int i = 0; i < 10000000; i++);
    /* wait for reset to complete */
    while (REG_CTRL(dev) & CTRL_RST);
}

static void set_link_up(e1000_dev_t *dev) {
    uint32_t temp = REG_CTRL(dev);
    temp |= CTRL_SLU;
    REG_CTRL(dev) = temp;
}

static void check_link_status(e1000_dev_t *dev) {
    dev->link_up = !!(REG_STATUS(dev) & STATUS_LU);
}

static void configure_pba(e1000_dev_t *dev) {
    switch(dev->family) {
    case e1000_82580:
        /* Leave at defaults */
        break;
    case e1000_82574:
        /* The default should be 20/20 split for PBA, but set it to that just in case */
        REG_82574_PBA(dev) = (20 << 16) | 20;
        break;
    default:
        assert(!"Unknown device");
    }
}

static void phy_write(e1000_dev_t *dev, int phy, int reg, uint16_t data) {
    REG_MDIC(dev) = data | (reg << 16) | (phy << 21) | (BIT(26));
    while((REG_MDIC(dev) & BIT(28)) == 0);
}

static uint16_t phy_read(e1000_dev_t *dev, int phy, int reg) {
    uint32_t mdi;
    REG_MDIC(dev) = (reg << 16) | (phy << 21) | (2 << 26);
    while(( (mdi = REG_MDIC(dev)) & BIT(28)) == 0);
    return mdi & MASK(16);
}

static void reset_phy(e1000_dev_t *dev) {
    uint32_t temp;
    switch(dev->family) {
    case e1000_82580:
        /* Don't reset the phy for now */
        break;
    case e1000_82574:
        /* for unknown reasons the 82574 this was tested on cannot seem to perform
         * well if it ends up being the slave side of a connection. Therefore we
         * try and force us to negotiate to be the master */
        temp = phy_read(dev, 1, 9);
        temp |= BIT(12) | BIT(11);
        phy_write(dev, 1, 9, temp);
        /* write a reset */
        phy_write(dev, 1, 0, BIT(15) | BIT(12));
        /* wait until it completes */
        while(phy_read(dev, 1, 0) & BIT(15));
        break;
    default:
        assert(!"Unknown device");
    }
}

static void configure_flow_control(e1000_dev_t *dev) {
    switch(dev->family) {
    case e1000_82580:
        /* Nothing to setup. Default values will allow us to receive
         * pause frames, we don't send them however */
        break;
    case e1000_82574:
        /* Manual says to set these values for flow control */
        REG_82574_FCAL(dev) = 0x00C28001;
        REG_82574_FCAH(dev) = 0x0100;
        REG_82574_FCT(dev) = 0x8808;
        break;
    default:
        assert(!"Unknown device");
    }
}

static void initialize(e1000_dev_t *dev) {
    disable_all_interrupts(dev);
    reset_device(dev);
    disable_all_interrupts(dev);

    if (dev->family == e1000_82574) {
        /* this bit must be set according to the manual */
        REG_82574_GCR(dev) |= BIT(22);
    }

    reset_phy(dev);

    set_link_up(dev);

    configure_flow_control(dev);
    configure_pba(dev);
}

static void initialise_TXDCTL(e1000_dev_t *dev) {
    uint32_t temp;
    switch(dev->family) {
    case e1000_82580:
        /* Enable transmit queue */
        temp = REG_82580_TXDCTL(dev, 0);
        temp &= ~TXDCTL_82580_RESERVED_BITS;
        temp |= TXDCTL_82580_ENABLE;
        REG_82580_TXDCTL(dev, 0) = temp;
        break;
    case e1000_82574:
        temp = REG_82574_TXDCTL(dev, 0);
        temp &= ~TXDCTL_82574_RESERVED_BITS;
        /* set the bit that we have to set */
        temp |= TXDCTL_82574_BIT_THAT_SHOULD_BE_1;
        /* set gran to 1 */
        temp |= TXDCTL_82574_GRAN;
        /* wthresh and hthresh to 1 to avoid tx stalls */
        temp |= 1 << TXDCTL_82574_WTHRESH_OFFSET;
        temp |= 1 << TXDCTL_82574_HTHRESH_OFFSET;
        /* prefetch when less than 31 */
        temp |= 31 << TXDCTL_82574_PTHRESH_OFFSET;
        REG_82574_TXDCTL(dev, 0) = temp;
        break;
    default:
        assert(!"Unknown device");
        break;
    }
}

static void initialise_TCTL(e1000_dev_t *dev)
{
    uint32_t temp = 0;
    switch(dev->family) {
    case e1000_82580:
        break;
    case e1000_82574:
        /* set cold to 0x3f for FD (or 0x1FF for HD) */
        temp |= TCTL_82574_COLD_BITS(0x3f);
        /* allow multiple transmit requests from hardware at once */
        temp |= TCTL_82574_MULR;
        break;
    default:
        assert(!"Unknown device");
    }
    temp |= TCTL_EN;
    temp |= TCTL_PSP;
    temp |= TCTL_CT_BITS(0xf);
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

static void initialise_transmit_timers(e1000_dev_t *dev) {
    switch(dev->family) {
    case e1000_82580:
        break;
    case e1000_82574:
        /* Set the transmit notifcations really high. We will cleanup transmits
         * lazily for the most part and there is no real rush to release buffers quickly */
        REG_82574_TIDV(dev) = 20000;
        REG_82574_TADV(dev) = 60000;
        break;
    default:
        assert(!"Unknown device");
    }
}

static void initialize_transmit(e1000_dev_t *dev) {
    initialise_TXDCTL(dev);
    initialise_TIPG(dev);
    initialise_transmit_timers(dev);
    initialise_TCTL(dev);
}

static void initialize_RCTL(e1000_dev_t *dev) {
    uint32_t temp = 0;
    switch(dev->family) {
    case e1000_82580:
        break;
    case e1000_82574:
        /* set free receive descriptor threshold to one quarter */
        temp |= RCTL_82574_RDMTS_1_4;
        break;
    }
    /* Enable receive */
    temp |= RCTL_EN;
    /* Enable promiscuous mode and accept broadcast packets */
    temp |= RCTL_UPE;
    temp |= RCTL_MPE;
    temp |= RCTL_BAM;
    /* defaults for everything else will give us 2K pages, which is what we want */
    REG_RCTL(dev) = temp;
}

static void initialize_RXDCTL(e1000_dev_t *dev) {
    uint32_t temp;
    switch(dev->family) {
    case e1000_82580:
        temp = REG_82580_RXDCTL(dev, 0);
        temp &= ~RXDCTL_82580_RESERVED_BITS;
        temp |= RXDCTL_82580_ENABLE;
        REG_82580_RXDCTL(dev, 0) = temp;
        break;
    case e1000_82574:
        temp = REG_82574_RXDCTL(dev, 0);
        temp &= ~RXDCTL_82574_RESERVED_BITS;
        /* count in descriptors */
        temp |= RXDCTL_82574_GRAN;
        /* prefetch once below 4 */
        temp |= 4 << RXDCTL_82574_PTHRESH_OFFSET;
        /* keep host at 32 free */
        temp |= 32 << RXDCTL_82574_HTHRESH_OFFSET;
        /* write back 4 at a time */
        temp |= 4 << RXDCTL_82574_WTHRESH_OFFSET;
        REG_82574_TXDCTL(dev, 0) = temp;
        break;
    default:
        assert(!"Unknown device");
    }
}

static void initialize_receive_timers(e1000_dev_t *dev) {
    switch(dev->family) {
    case e1000_82580:
        break;
    case e1000_82574:
        /* set a base delay of 20 microseconds */
        REG_82574_RDTR(dev) = 20;
        /* force descriptor write back after 20 microseconds */
        REG_82574_RADV(dev) = 20;
        REG_82574_RAID(dev) = 0;
        break;
    default:
        assert(!"Unknown device");
    }
}

static void initialize_receive(e1000_dev_t *dev) {
    /* zero the MTA */
    int i;
    for (i = 0; i < MTA_LENGTH; i++) {
        REG_MTA(dev, i) = 0;
    }
    initialize_receive_timers(dev);
    initialize_RXDCTL(dev);
    initialize_RCTL(dev);
}

static void enable_interrupts(e1000_dev_t *dev) {
    switch(dev->family) {
    case e1000_82580:
        REG_82580_IMS(dev) = IMS_82580_RXDW | IMS_82580_TXDW | IMS_82580_GPHY;
        /* enable link status change interrupts in the phy */
        phy_write(dev, 0, 24, BIT(2));
        break;
    case e1000_82574:
        REG_82574_IMS(dev) = IMS_82574_RXQ0 | IMS_82574_RXTO | IMS_82574_RXDMT0 | IMS_82574_ACK | IMS_82574_TXDW | IMS_82574_LSC;
        break;
    default:
        assert(!"Unknown device");
        break;
    }
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

void low_level_init(struct eth_driver *driver, uint8_t *mac, int *mtu) {
    e1000_dev_t *dev = (e1000_dev_t*)driver->eth_data;
    eth_get_mac(dev, mac);
    /* hardcode MTU for now */
    *mtu = 1500;
}

static void set_tx_ring(e1000_dev_t *dev, uintptr_t phys) {
    uint32_t phys_low = (uint32_t)phys;
    uint32_t phys_high = (uint32_t)(sizeof(phys) > 4 ? phys >> 32 : 0);
    switch(dev->family) {
    case e1000_82580:
        REG_82580_TDBAL(dev, 0) = phys_low;
        REG_82580_TDBAH(dev, 0) = phys_high;
        break;
    case e1000_82574:
        REG_82574_TDBAL(dev, 0) = phys_low;
        REG_82574_TDBAH(dev, 0) = phys_high;
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
    /* tdlen must be multiple of 128 */
    assert(val % 128 == 0);
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

static void set_rx_ring(e1000_dev_t *dev, uint64_t phys) {
    uint32_t phys_low = (uint32_t)phys;
    uint32_t phys_high = (uint32_t)(sizeof(phys) > 4 ? phys >> 32 : 0);
    switch(dev->family) {
    case e1000_82580:
        REG_82580_RDBAL(dev, 0) = phys_low;
        REG_82580_RDBAH(dev, 0) = phys_high;
        break;
    case e1000_82574:
        REG_82574_RDBAL(dev, 0) = phys_low;
        REG_82574_RDBAH(dev, 0) = phys_high;
        break;
    default:
        assert(!"Unknown device");
    }
}

static void set_rdlen(e1000_dev_t *dev, uint32_t val) {
    /* rdlen must be multiple of 128 */
    assert(val % 128 == 0);
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

static void free_desc_ring(e1000_dev_t *dev, ps_dma_man_t *dma_man) {
    if (dev->rx_ring) {
        dma_unpin_free(dma_man, (void*)dev->rx_ring, sizeof(struct legacy_rx_ldesc) * dev->rx_size);
        dev->rx_ring = NULL;
    }
    if (dev->tx_ring) {
        dma_unpin_free(dma_man, (void*)dev->tx_ring, sizeof(struct legacy_tx_ldesc) * dev->tx_size);
        dev->tx_ring = NULL;
    }
    if (dev->rx_cookies) {
        free(dev->rx_cookies);
        dev->rx_cookies = NULL;
    }
    if (dev->tx_cookies) {
        free(dev->tx_cookies);
        dev->tx_cookies = NULL;
    }
    if (dev->tx_lengths) {
        free(dev->tx_lengths);
        dev->tx_lengths = NULL;
    }
}

static int initialize_desc_ring(e1000_dev_t *dev, ps_dma_man_t *dma_man) {
    dma_addr_t rx_ring = dma_alloc_pin(dma_man, sizeof(struct legacy_rx_ldesc) * dev->rx_size, 1, DMA_ALIGN);
    if (!rx_ring.phys) {
        LOG_ERROR("Failed to allocate rx_ring");
        return -1;
    }
    dev->rx_ring = rx_ring.virt;
    dma_addr_t tx_ring = dma_alloc_pin(dma_man, sizeof(struct legacy_tx_ldesc) * dev->tx_size, 1, DMA_ALIGN);
    if (!tx_ring.phys) {
        LOG_ERROR("Failed to allocate tx_ring");
        free_desc_ring(dev, dma_man);
        return -1;
    }
    dev->rx_cookies = malloc(sizeof(void*) * dev->rx_size);
    dev->tx_cookies = malloc(sizeof(void*) * dev->tx_size);
    dev->tx_lengths = malloc(sizeof(unsigned int) * dev->tx_size);
    if (!dev->rx_cookies || !dev->tx_cookies || !dev->tx_lengths) {
        if (dev->rx_cookies) {
            free(dev->rx_cookies);
        }
        if (dev->tx_cookies) {
            free(dev->tx_cookies);
        }
        if (dev->tx_lengths) {
            free(dev->tx_lengths);
        }
        LOG_ERROR("Failed to malloc");
        free_desc_ring(dev, dma_man);
        return -1;
    }
    dev->tx_ring = tx_ring.virt;
    /* Remaining needs to be 2 less than size as we cannot actually enqueue size many descriptors,
     * since then the head and tail pointers would be equal, indicating empty. */
    dev->rx_remain = dev->rx_size - 2;
    dev->tx_remain = dev->tx_size - 2;

    /* Tell the hardware where the rings are and now big they are */
    set_tx_ring(dev, tx_ring.phys);
    set_tdlen(dev, dev->tx_size * sizeof(struct legacy_tx_ldesc));
    set_rx_ring(dev, rx_ring.phys);
    set_rdlen(dev, dev->rx_size * sizeof(struct legacy_rx_ldesc));

    /* Set transmit ring initially empty */
    dev->tdh = dev->tdt = 0;
    set_tdh(dev, dev->tdh);
    set_tdt(dev, dev->tdt);

    /* Set receive ring initially empty */
    dev->rdh = dev->rdt = read_rdh(dev);
    set_rdt(dev, dev->rdt);

    return 0;
}

static void complete_rx(struct eth_driver *driver) {
    e1000_dev_t *dev = (e1000_dev_t*)driver->eth_data;
    if (dev->rdh == dev->rdt) {
        /* We haven't enqueued anything */
        return;
    }
    unsigned int i, j;
    unsigned int count = 1;
    unsigned int rdt = dev->rdt;
    for (i = dev->rdh; i != rdt; i = (i + 1) % dev->rx_size, count++) {
        unsigned int status = dev->rx_ring[i].status;
        /* Ensure no memory references get ordered before we checked the descriptor was written back */
        asm volatile("lfence" ::: "memory");
        if (!(status & RX_DD)) {
            /* not complete yet */
            break;
        }
        if (status & RX_EOP) {
            void *cookies[count];
            unsigned int len[count];
            for (j = 0; j < count; j++) {
                cookies[j] = dev->rx_cookies[(dev->rdh + j) % dev->rx_size];
                len[j] = dev->rx_ring[(dev->rdh + j) % dev->rx_size].length;
            }
            /* update rdh */
            dev->rdh = (dev->rdh + count) % dev->rx_size;
            dev->rx_remain += count;
            /* Give the buffers back */
            driver->i_cb.rx_complete(driver->cb_cookie, count, cookies, len);
            count = 0;
        }
    }
}

static void complete_tx(struct eth_driver *driver) {
    e1000_dev_t *dev = (e1000_dev_t*)driver->eth_data;
    while (dev->tdh != dev->tdt) {
        unsigned int i;
        for (i = 0; i < dev->tx_lengths[dev->tdh]; i++) {
            if (!(dev->tx_ring[(i + dev->tdh) % dev->tx_size].STA & TX_DD)) {
                /* not all parts complete */
                return;
            }
        }
        /* do not let memory loads happen before our checking of the descriptor write back */
        asm volatile("lfence" ::: "memory");
        /* increase where we believe tdh to be */
        void *cookie = dev->tx_cookies[dev->tdh];
        dev->tx_remain += dev->tx_lengths[dev->tdh];
        dev->tdh = (dev->tdh + dev->tx_lengths[dev->tdh]) % dev->tx_size;
        /* give the buffer back */
        driver->i_cb.tx_complete(driver->cb_cookie, cookie);
    }
}

static int raw_tx(struct eth_driver *driver, unsigned int num, uintptr_t *phys, unsigned int *len, void *cookie) {
    e1000_dev_t *dev = (e1000_dev_t*)driver->eth_data;
    if (!dev->link_up) {
        return ETHIF_TX_FAILED;
    }
    /* Ensure we have room */
    if (dev->tx_remain < num) {
        /* try and complete some */
        complete_tx(driver);
        if (dev->tx_remain < num) {
            return ETHIF_TX_FAILED;
        }
    }
    unsigned int i;
    for (i = 0; i < num; i++) {
        dev->tx_ring[(dev->tdt + i) % dev->tx_size] = (struct legacy_tx_ldesc) {
            .bufferAddress = phys[i],
            .length = len[i],
            .CSO = 0,
            .CMD = dev->tx_cmd_bits | (i + 1 == num ? TX_CMD_EOP: 0),
            .STA = 0,
            .ExtCMD = 0,
            .CSS = 0,
            .VLAN = 0
        };
    }
    dev->tx_cookies[dev->tdt] = cookie;
    dev->tx_lengths[dev->tdt] = num;
    /* ensure update to descriptors visible before updating tdt */
    asm volatile("mfence" ::: "memory");
    dev->tdt = (dev->tdt + num) % dev->tx_size;
    dev->tx_remain -= num;
    set_tdt(dev, dev->tdt);
    return ETHIF_TX_ENQUEUED;
}

static int fill_rx_bufs(struct eth_driver *driver) {
    e1000_dev_t *dev = (e1000_dev_t*)driver->eth_data;
    int rdt = dev->rdt;
    /* We want to install buffers in bursts for performance reasons.
     * constantly enqueueing single buffers is expensive */
    if (dev->rx_remain < 32) return 0;
    while (dev->rx_remain > 0) {
        /* request a buffer */
        void *cookie;
        uintptr_t phys = driver->i_cb.allocate_rx_buf(driver->cb_cookie, BUF_SIZE, &cookie);
        if (!phys) {
            break;
        }
        dev->rx_cookies[dev->rdt] = cookie;
        /* zery the descriptor */
        dev->rx_ring[dev->rdt] = (struct legacy_rx_ldesc) {
            .bufferAddress = phys,
            .length = BUF_SIZE,
            .packetChecksum = 0,
            .status = 0,
            .error = 0,
            .VLAN = 0
        };
        dev->rdt = (dev->rdt + 1) % dev->rx_size;
        dev->rx_remain--;
    }
    if (dev->rdt != rdt) {
        /* ensure update to descriptor visible before updating rdt */
        asm volatile("sfence" ::: "memory");
        set_rdt(dev, dev->rdt);
    }
    return dev->rx_remain != 0;
}

static void raw_poll(struct eth_driver *driver) {
    complete_rx(driver);
    complete_tx(driver);
    fill_rx_bufs(driver);
    check_link_status(driver->eth_data);
}

static void handle_irq(struct eth_driver *driver, int irq) {
    e1000_dev_t *dev = (e1000_dev_t*)driver->eth_data;
    uint32_t icr;
    switch(dev->family) {
    case e1000_82580:
        icr = REG_82580_ICR(dev);
        if (icr & ICR_82580_RXDW) {
            complete_rx(driver);
            fill_rx_bufs(driver);
        }
        if (icr & ICR_82580_TXDW) {
            complete_tx(driver);
        }
        if (icr & ICR_82580_GPHY) {
            uint32_t phy = phy_read(dev, 0, 25);
            if (phy & BIT(3)) {
                check_link_status(dev);
            }
        }
        break;
    case e1000_82574:
        icr = REG_82574_ICR(dev);
        /* ack */
        REG_82574_ICR(dev) = icr;
        if(icr & (ICR_82574_RXQ0 | ICR_82574_RXTO | ICR_82574_ACK | ICR_82574_RXDMT0)) {
            complete_rx(driver);
            fill_rx_bufs(driver);
        }
        if (icr & ICR_82574_TXDW) {
            complete_tx(driver);
        }
        if (icr & ICR_82574_LSC) {
            check_link_status(dev);
            if (!dev->link_up) {
                /* should probably remove everything from the TX ring here */
            }
        }
        break;
    default:
        assert(!"Unknown device");
    }
}

static struct raw_iface_funcs iface_fns = {
    .raw_handleIRQ = handle_irq,
    .print_state = print_state,
    .low_level_init = low_level_init,
    .raw_tx = raw_tx,
    .raw_poll = raw_poll
};

static int
common_init(struct eth_driver *driver, ps_io_ops_t io_ops, void *config, e1000_dev_t *dev) {
    int err;
    ethif_intel_config_t *eth_config = (ethif_intel_config_t*) config;
    dev->iobase = eth_config->bar0;
    dev->tx_size = CONFIG_LIB_ETHDRIVER_RX_DESC_COUNT;
    dev->rx_size = CONFIG_LIB_ETHDRIVER_TX_DESC_COUNT;

    /* technically we support alignemtn of 1, but get better performance with some alignment */
    driver->dma_alignment = 16;
    driver->eth_data = dev;
    driver->i_fn = iface_fns;

    initialize(dev);
    err = initialize_desc_ring(dev, &io_ops.dma_manager);
    if (err) {
        /* Reset device */
        disable_all_interrupts(dev);
        reset_device(dev);
        /* Free memory */
        free(dev);
        return -1;
    }
    /* the transmit and receive initialization functions assume
     * that we have setup descriptor rings for the transmit receive queues */
    initialize_transmit(dev);
    initialize_receive(dev);
    /* fill up the receive ring as much as possible */
    fill_rx_bufs(driver);
    /* turn interrupts on */
    enable_interrupts(dev);
    /* check the current status of the link */
    check_link_status(dev);
    return 0;
}

int
ethif_e82580_init(struct eth_driver *driver, ps_io_ops_t io_ops, void *config) {
    e1000_dev_t *dev = malloc(sizeof(*dev));
    if (!dev) {
        LOG_ERROR("Failed to malloc");
        return -1;
    }
    dev->family = e1000_82580;
    dev->tx_cmd_bits = TX_CMD_IFCS | TX_CMD_RS;
    return common_init(driver, io_ops, config, dev);
}

int
ethif_e82574_init(struct eth_driver *driver, ps_io_ops_t io_ops, void *config) {
    e1000_dev_t *dev = malloc(sizeof(*dev));
    if (!dev) {
        LOG_ERROR("Failed to malloc");
        return -1;
    }
    dev->family = e1000_82574;
    dev->tx_cmd_bits = TX_CMD_IFCS | TX_CMD_RS | TX_CMD_IDE;
    return common_init(driver, io_ops, config, dev);
}
