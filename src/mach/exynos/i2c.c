/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <platsupport/i2c.h>
#include <platsupport/mux.h>
#include "../../services.h"

//#define I2C_DEBUG
#ifdef I2C_DEBUG
#define dprintf(...) printf("I2C: " __VA_ARGS__)
#else
#define dprintf(...) do{}while(0)
#endif

/*********************
 *** SoC specifics ***
 *********************/

#if defined(PLAT_EXYNOS4)
/* IRQS */
#define EXYNOS_I2C0_IRQ  90
#define EXYNOS_I2C1_IRQ  91
#define EXYNOS_I2C2_IRQ  92
#define EXYNOS_I2C3_IRQ  93
#define EXYNOS_I2C4_IRQ  94
#define EXYNOS_I2C5_IRQ  95
#define EXYNOS_I2C6_IRQ  96
#define EXYNOS_I2C7_IRQ  97
#define EXYNOS_I2C8_IRQ   0
#define EXYNOS_I2C9_IRQ   0
#define EXYNOS_I2C10_IRQ  0
#define EXYNOS_I2C11_IRQ  0
/* Physical addresses */
#define EXYNOS_I2C0_PADDR  0x13860000
#define EXYNOS_I2C1_PADDR  0x13870000
#define EXYNOS_I2C2_PADDR  0x13880000
#define EXYNOS_I2C3_PADDR  0x13890000
#define EXYNOS_I2C4_PADDR  0x138A0000
#define EXYNOS_I2C5_PADDR  0x138B0000
#define EXYNOS_I2C6_PADDR  0x138C0000
#define EXYNOS_I2C7_PADDR  0x138D0000
#define EXYNOS_I2C8_PADDR  0x138E0000
#define EXYNOS_I2C9_PADDR  0xDEADBEEF
#define EXYNOS_I2C10_PADDR 0xDEADBEEF
#define EXYNOS_I2C11_PADDR 0xDEADBEEF

#elif defined(PLAT_EXYNOS5)
/* IRQS */
#define EXYNOS_I2C0_IRQ  88
#define EXYNOS_I2C1_IRQ  89
#define EXYNOS_I2C2_IRQ  90
#define EXYNOS_I2C3_IRQ  91
#define EXYNOS_I2C4_IRQ  92
#define EXYNOS_I2C5_IRQ  93
#define EXYNOS_I2C6_IRQ  94
#define EXYNOS_I2C7_IRQ  95
#define EXYNOS_I2C8_IRQ  96
#define EXYNOS_I2C9_IRQ   0
#define EXYNOS_I2C10_IRQ  0
#define EXYNOS_I2C11_IRQ  0
/* Physical addresses */
#define EXYNOS_I2C0_PADDR  0x12C60000
#define EXYNOS_I2C1_PADDR  0x12C70000
#define EXYNOS_I2C2_PADDR  0x12C80000
#define EXYNOS_I2C3_PADDR  0x12C90000
#define EXYNOS_I2C4_PADDR  0x12CA0000
#define EXYNOS_I2C5_PADDR  0x12CB0000
#define EXYNOS_I2C6_PADDR  0x12CC0000
#define EXYNOS_I2C7_PADDR  0x12CD0000
#define EXYNOS_I2C8_PADDR  0x12CE0000
#define EXYNOS_I2C9_PADDR  0x13130000
#define EXYNOS_I2C10_PADDR 0x13140000
#define EXYNOS_I2C11_PADDR 0x121D0000

#else  /* EXYNOS? */
#error Unknown Exynos based platform
#endif /* EXYNOSX */

/* Sizes */
#define EXYNOS_I2CX_SIZE  0x1000
#define EXYNOS_I2C0_SIZE  EXYNOS_I2CX_SIZE
#define EXYNOS_I2C1_SIZE  EXYNOS_I2CX_SIZE
#define EXYNOS_I2C2_SIZE  EXYNOS_I2CX_SIZE
#define EXYNOS_I2C3_SIZE  EXYNOS_I2CX_SIZE
#define EXYNOS_I2C4_SIZE  EXYNOS_I2CX_SIZE
#define EXYNOS_I2C5_SIZE  EXYNOS_I2CX_SIZE
#define EXYNOS_I2C6_SIZE  EXYNOS_I2CX_SIZE
#define EXYNOS_I2C7_SIZE  EXYNOS_I2CX_SIZE
#define EXYNOS_I2C8_SIZE  EXYNOS_I2CX_SIZE
#define EXYNOS_I2C9_SIZE  EXYNOS_I2CX_SIZE
#define EXYNOS_I2C10_SIZE EXYNOS_I2CX_SIZE
#define EXYNOS_I2C11_SIZE EXYNOS_I2CX_SIZE

/************************
 *** Register bitmaps ***
 ************************/
/* control */
#define I2CCON_ACK_EN        BIT(7)
#define I2CCON_CLK_SRC       BIT(6)
#define I2CCON_IRQ_EN        BIT(5)
#define I2CCON_IRQ_PEND      BIT(4)
#define I2CCON_PRESCALE(x)   (((x) & 0xf) * BIT(0))
#define I2CCON_PRESCALE_MASK I2CCON_PRESCALE(0xf)
/* status */
#define I2CSTAT_MODE(x)      (((x) & 0x3) * BIT(6))
#define I2CSTAT_MODE_SRX     I2CSTAT_MODE(0x0)
#define I2CSTAT_MODE_STX     I2CSTAT_MODE(0x1)
#define I2CSTAT_MODE_MRX     I2CSTAT_MODE(0x2)
#define I2CSTAT_MODE_MTX     I2CSTAT_MODE(0x3)
#define I2CSTAT_MODE_MASK    I2CSTAT_MODE(0x3)
#define I2CSTAT_BUSY         BIT(5)
#define I2CSTAT_ENABLE       BIT(4)
#define I2CSTAT_ARB_FAIL     BIT(3)
#define I2CSTAT_ADDR_SLAVE   BIT(2)
#define I2CSTAT_ADDR_ZERO    BIT(1)
#define I2CSTAT_ACK          BIT(0)
/* address */
#define I2CADDR(x)           (((x) & 0xff) * BIT(0))
#define I2CADDR_MASK         I2CADDR(0xff)
/* data */
#define I2CDATA(x)           (((x) & 0xff) * BIT(0))
#define I2CDATA_MASK         I2CDATA(0xff)
#define I2CDATA_READ(addr)   I2CDATA(((addr) & 0xfe) | 1)
#define I2CDATA_WRITE(addr)  I2CDATA(((addr) & 0xfe) | 0)
/* Line control */
#define I2CLC_FILT_EN        BIT(2)
#define I2CLC_SDA_DELAY(x)   (((x) & 0x3) * BIT(0))
#define I2CLC_SDA_DELAY0CLK  I2CLC_SDA_DELAY(0x0)
#define I2CLC_SDA_DELAY5CLK  I2CLC_SDA_DELAY(0x1)
#define I2CLC_SDA_DELAY10CLK I2CLC_SDA_DELAY(0x2)
#define I2CLC_SDA_DELAY15CLK I2CLC_SDA_DELAY(0x3)
#define I2CLC_SDA_DELAY_MASK I2CLC_SDA_DELAY(0x3)

struct exynos_i2c_regs {
    uint32_t control;        /* 0x0000 control register 0x0X              */
    uint32_t status;         /* 0x0004 control/status register 0x00       */
    uint32_t address;        /* 0x0008 address register 0xXX              */
    uint32_t data;           /* 0x000C tx/rx data shift register 0xXX     */
    uint32_t line_control;   /* 0x0010 multi-master line control register */
};


struct i2c_bus {
    volatile struct exynos_i2c_regs* regs;
    const char* tx_buf;
    int tx_len;
    int tx_count;
    char* rx_buf;
    int rx_len;
    int rx_count;
    enum mux_feature mux;
};

static i2c_bus_t _i2c[NI2C] = {
    { .regs = NULL, .mux = MUX_I2C0  },
    { .regs = NULL, .mux = MUX_I2C1  },
    { .regs = NULL, .mux = MUX_I2C2  },
    { .regs = NULL, .mux = MUX_I2C3  },
    { .regs = NULL, .mux = MUX_I2C4  },
    { .regs = NULL, .mux = MUX_I2C5  },
    { .regs = NULL, .mux = MUX_I2C6  },
    { .regs = NULL, .mux = MUX_I2C7  },
#if defined(PLAT_EXYNOS4)
#elif defined(PLAT_EXYNOS5)
    { .regs = NULL, .mux = MUX_I2C8  },
    { .regs = NULL, .mux = MUX_I2C9  },
    { .regs = NULL, .mux = MUX_I2C10 },
    { .regs = NULL, .mux = MUX_I2C11 }
#else  /* EXYNOS? */
#error Unknown Exynos based platform
#endif /* EXYNOSX */
};


static inline int irq_pending(i2c_bus_t* dev){
    return !!(dev->regs->control & I2CCON_IRQ_PEND);
}

static inline void clear_pending(i2c_bus_t* dev){
    uint32_t v = dev->regs->control;
    v &= ~(I2CCON_IRQ_PEND);
    dev->regs->control = v;
}

static inline int addressed_as_slave(i2c_bus_t* dev){
    return !!(dev->regs->status & I2CSTAT_ADDR_SLAVE);
}

static inline int busy(i2c_bus_t* dev){
    return !!(dev->regs->status & I2CSTAT_BUSY);
}

static inline int acked(i2c_bus_t* dev){
    return !(dev->regs->status & I2CSTAT_ACK);
}

static inline int enabled(i2c_bus_t* dev){
    return !!(dev->regs->status & I2CSTAT_ENABLE);
}

void debug(i2c_bus_t* dev){
    printf("%sbusy, %saddressed_as_slave, irq %s pending\n",
              busy(dev)? "" : "not ", 
              addressed_as_slave(dev)? "" : "not ",
              irq_pending(dev)? "is" : "not"); 
}

int
i2c_read(i2c_bus_t* dev, void* vdata, int len){
    char* data = (char*)vdata;
    int count = 0;
    uint32_t c;

    dprintf("Reading %d bytes as slave 0x%x\n", len, dev->regs->address);
    /** Configure Master Rx mode **/
    clear_pending(dev);
    dev->regs->status = I2CSTAT_ENABLE | I2CSTAT_MODE_SRX;
    dev->regs->control |= I2CCON_ACK_EN;
    /* Wait for addressed as slave */
    dprintf("RX waiting for master\n");
    while(!irq_pending(dev));
    if(addressed_as_slave(dev)){

        clear_pending(dev);
        /* Dummy read */
        c = dev->regs->data;

        /* Read bytes */
        while(count < len && busy(dev)){
            /* After ACK period, IRQ is pending */
            while(!irq_pending(dev) && busy(dev));
            if(irq_pending(dev)){
                c = dev->regs->data;
                *data++ = c;
                clear_pending(dev);
                count++;
            }
        }
        dev->regs->control &= ~I2CCON_ACK_EN;
        while(busy(dev)){
            while(!irq_pending(dev) && busy(dev));
            clear_pending(dev);
            c = dev->regs->data;
        }
        dprintf("read %d bytes\n", count);
        dev->regs->status = ~(I2CSTAT_ENABLE | I2CSTAT_MODE_SRX);
        return count;
    }else{
        dprintf("Not addressed as slave\n");
        return -1;
    }
}

static void
master_txstart(i2c_bus_t* dev, int slave){
    dev->regs->control |= I2CCON_ACK_EN;
    /** Configure Master Tx mode **/
    dev->regs->status = I2CSTAT_MODE_MTX | I2CSTAT_ENABLE; 
    /* Write slave address */
    dev->regs->data = I2CDATA_WRITE(slave);
    /* Write 0xF0 (M/T Start) to I2CSTAT */
    clear_pending(dev);
    dev->regs->status |= I2CSTAT_BUSY;
}

static void
master_rxstart(i2c_bus_t* dev, int slave){
    dev->regs->control |= I2CCON_ACK_EN;
    /** Configure Master Rx mode **/
    dev->regs->status = I2CSTAT_ENABLE | I2CSTAT_MODE_MRX;
    /* Write slave address */
    dev->regs->data = I2CDATA_READ(slave);
    /* Write 0xB0 (M/R Start) to I2CSTAT */
    dev->regs->status |= I2CSTAT_BUSY;
}

int
i2c_write(i2c_bus_t* dev, const void* vdata, int len){
    const char* data = (const char*)vdata;
    int count = 0;

    dprintf("Writing %d bytes as slave 0x%x\n", len, dev->regs->address);
    /** Configure Master Rx mode **/
    clear_pending(dev);
    dev->regs->control |= I2CCON_ACK_EN;
    dev->regs->status = I2CSTAT_ENABLE | I2CSTAT_MODE_STX;
    /* Wait for addressed as slave */
    dprintf("TX waiting for master\n");
    while(!irq_pending(dev));

    if(addressed_as_slave(dev)){
        while(count < len && busy(dev)){
            /* After ACK period, IRQ is pending */
            dev->regs->data = *data++;
            clear_pending(dev);
            while(!irq_pending(dev) && busy(dev));
            count++;
        }
        dprintf("wrote %d bytes\n", count);
        dev->regs->status = ~(I2CSTAT_ENABLE | I2CSTAT_MODE_SRX);
        clear_pending(dev);
        return count;
    }else{
        dprintf("Not addressed as slave\n");
        return -1;
    }
}

/* Exynos4 manual Figure 14-6 p14-9 */
int
i2c_mread(i2c_bus_t* dev, int slave, void* vdata, int len)
{
    dprintf("Reading %d bytes from slave@0x%02x\n", len, slave);
    master_rxstart(dev, slave);

    /* Setup the RX descriptor */
    dev->rx_buf = (char*)vdata;
    dev->rx_len = len;
    dev->rx_count = -1;

    /* Wait for completion */
    while(busy(dev)){
        i2c_handle_irq(dev);
    }
    return dev->rx_count;
}

/* Exynos4 manual Figure 14-6 p14-8 */
int
i2c_mwrite(i2c_bus_t* dev, int slave, const void* vdata, int len)
{
    dprintf("Writing %d bytes to slave@0x%02x\n", len, slave);
    master_txstart(dev, slave);

    dev->tx_count = -1;
    dev->tx_len = len;
    dev->tx_buf = (const char*)vdata;

    while(busy(dev)){
        i2c_handle_irq(dev);
    }
    return dev->tx_count;
}


void
i2c_handle_irq(i2c_bus_t* dev){
    uint32_t v;
    if(enabled(dev) && irq_pending(dev)){
        switch(dev->regs->status & I2CSTAT_MODE_MASK){
        case I2CSTAT_MODE_MRX:
            if(dev->rx_count < 0){
                if(acked(dev)){
                    /* slave responded to the address */
                    dev->rx_count = 0;
                }else{
                    /* No response: Abort */
                    dev->regs->status &= ~(I2CSTAT_BUSY);
                }
            }else if(dev->regs->control & I2CCON_ACK_EN){
                /* Read from slave */
                v = dev->regs->data;
                *dev->rx_buf++ = v;
                dev->rx_count++;
                /* Don't ACK the last byte */
                if(dev->rx_count == dev->rx_len){
                    dev->regs->control &= ~(I2CCON_ACK_EN);
                }
            }else{
                /* Finally, send stop */
                dev->regs->status &= ~(I2CSTAT_BUSY);
            }
            break;
        case I2CSTAT_MODE_MTX:
            if(acked(dev)){
                /* Start pumping out data */
                v = *dev->tx_buf++;
                dev->regs->data = v;
                dev->tx_count++;
                if(dev->tx_count == dev->tx_len){
                    /* Write 0xD0 (M/T Stop) to I2CSTAT */
                    dev->regs->status &= ~(I2CSTAT_BUSY);
                }
            }else{
                /* No response: Abort */
                dev->regs->status &= ~(I2CSTAT_BUSY);
            }
            break;
        case I2CSTAT_MODE_STX:
        case I2CSTAT_MODE_SRX:
        default:
            assert(!"Unknown I2C mode");
        }
        clear_pending(dev);
    }
}

int
i2c_init(enum i2c_id id, ps_io_ops_t* io_ops, i2c_bus_t** i2c)
{
    i2c_bus_t* dev = _i2c + id;
    mux_sys_t* mux = &io_ops->mux_sys;
    /* Map memory */
    dprintf("Mapping i2c %d\n", id);
    switch(id){
    case I2C0:  MAP_IF_NULL(io_ops, EXYNOS_I2C0,  dev->regs); break;
    case I2C1:  MAP_IF_NULL(io_ops, EXYNOS_I2C1,  dev->regs); break;
    case I2C2:  MAP_IF_NULL(io_ops, EXYNOS_I2C2,  dev->regs); break;
    case I2C3:  MAP_IF_NULL(io_ops, EXYNOS_I2C3,  dev->regs); break;
    case I2C4:  MAP_IF_NULL(io_ops, EXYNOS_I2C4,  dev->regs); break;
    case I2C5:  MAP_IF_NULL(io_ops, EXYNOS_I2C5,  dev->regs); break;
    case I2C6:  MAP_IF_NULL(io_ops, EXYNOS_I2C6,  dev->regs); break;
    case I2C7:  MAP_IF_NULL(io_ops, EXYNOS_I2C7,  dev->regs); break;
#if defined(PLAT_EXYNOS4)
#elif defined(PLAT_EXYNOS5)
    case I2C8:  MAP_IF_NULL(io_ops, EXYNOS_I2C8,  dev->regs); break;
    case I2C9:  MAP_IF_NULL(io_ops, EXYNOS_I2C9,  dev->regs); break;
    case I2C10: MAP_IF_NULL(io_ops, EXYNOS_I2C10, dev->regs); break;
    case I2C11: MAP_IF_NULL(io_ops, EXYNOS_I2C11, dev->regs); break;
#else  /* EXYNOS? */
#error Unknown Exynos based platform
#endif /* EXYNOSX */
    default :
        return -1;
    }
    /* Check that our memory was mapped */
    if(dev->regs == NULL){
        return -2;
    }
    /* Configure MUX */
    if(mux_feature_enable(mux, dev->mux)){
        dprintf("Warning: failed to configure MUX\n");
    }

    /* TODO setup clocks */
    //gate = ps_io_map(&io_ops->io_mapper, 0x1003C000, 0x1000, 0, PS_MEM_NORMAL);
    //printf("gates: 0x%08x\n", gate[0x950/4]);


    /* I2C setup */
    dev->regs->control = I2CCON_ACK_EN | 0*I2CCON_CLK_SRC
                       | I2CCON_PRESCALE(7) | I2CCON_IRQ_EN;
    dev->regs->line_control = I2CLC_SDA_DELAY15CLK | I2CLC_FILT_EN;
    dev->regs->address = 0x54;

    *i2c = dev;
    return 0;
}

int
i2c_set_address(i2c_bus_t* i2c_bus, int addr){
    i2c_bus->regs->address = addr & 0xfe;
    return 0;
}
