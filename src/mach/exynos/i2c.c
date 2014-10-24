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

#define I2C_DEBUG
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


struct i2c_bus_priv {
    volatile struct exynos_i2c_regs* regs;
    const char* tx_buf;
    int tx_len;
    int tx_count;
    char* rx_buf;
    int rx_len;
    int rx_count;
    enum mux_feature mux;
};

static struct i2c_bus_priv _i2c[NI2C] = {
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

static inline struct i2c_bus_priv*
i2c_bus_get_priv(i2c_bus_t* i2c_bus) {
    return (struct i2c_bus_priv*)i2c_bus->priv;
}

static inline int
irq_pending(struct i2c_bus_priv* dev)
{
    return !!(dev->regs->control & I2CCON_IRQ_PEND);
}

static inline void
clear_pending(struct i2c_bus_priv* dev)
{
    uint32_t v = dev->regs->control;
    v &= ~(I2CCON_IRQ_PEND);
    dev->regs->control = v;
}

static inline int
addressed_as_slave(struct i2c_bus_priv* dev)
{
    return !!(dev->regs->status & I2CSTAT_ADDR_SLAVE);
}

static inline int
busy(struct i2c_bus_priv* dev)
{
    return !!(dev->regs->status & I2CSTAT_BUSY);
}

static inline int
acked(struct i2c_bus_priv* dev)
{
    return !(dev->regs->status & I2CSTAT_ACK);
}

static inline int
enabled(struct i2c_bus_priv* dev)
{
    return !!(dev->regs->status & I2CSTAT_ENABLE);
}

static void
master_txstart(struct i2c_bus_priv* dev, int slave)
{
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
master_rxstart(struct i2c_bus_priv* dev, int slave)
{
    dev->regs->control |= I2CCON_ACK_EN;
    /** Configure Master Rx mode **/
    dev->regs->status = I2CSTAT_ENABLE | I2CSTAT_MODE_MRX;
    /* Write slave address */
    dev->regs->data = I2CDATA_READ(slave);
    /* Write 0xB0 (M/R Start) to I2CSTAT */
    dev->regs->status |= I2CSTAT_BUSY;
}

static void
slave_init(struct i2c_bus_priv* dev, char addr)
{
    dev->regs->address = addr;
    /** Configure Master Rx mode **/
    dev->regs->status = I2CSTAT_ENABLE | I2CSTAT_MODE_SRX;
}

int
exynos_i2c_read(i2c_bus_t* i2c_bus, void* vdata, size_t len, i2c_callback_fn cb, void* token)
{
    struct i2c_bus_priv* dev = i2c_bus_get_priv(i2c_bus);

    dev->rx_buf = (char*)vdata;
    dev->rx_len = len;
    dev->rx_count = 0;
    i2c_bus->cb = cb;
    i2c_bus->token = token;

    dprintf("Reading %d bytes as slave 0x%x\n", len, dev->regs->address);
    dev->regs->control |= I2CCON_ACK_EN;

    if (cb == NULL) {
        /* Read bytes */
        while (dev->rx_count < dev->rx_len && busy(dev)) {
            i2c_handle_irq(i2c_bus);
        }
        dprintf("read %d bytes\n", dev->rx_count);
        return dev->rx_count;
    } else {
        /* Let the ISR handle it */
        return 0;
    }
}

static int
exynos_i2c_write(i2c_bus_t* i2c_bus, const void* vdata, size_t len, i2c_callback_fn cb, void* token)
{
    struct i2c_bus_priv* dev = i2c_bus_get_priv(i2c_bus);

    dev->tx_buf = (char*)vdata;
    dev->tx_len = len;
    dev->tx_count = 0;
    i2c_bus->cb = cb;
    i2c_bus->token = token;

    dprintf("Writing %d bytes as slave 0x%x\n", len, dev->regs->address);
    dev->regs->control |= I2CCON_ACK_EN;

    if (cb == NULL) {
        while (dev->tx_count < dev->tx_len && busy(dev)) {
            i2c_handle_irq(i2c_bus);
        }
        dprintf("wrote %d bytes\n", dev->tx_count);
        return dev->tx_count;
    } else {
        /* Let the ISR handle it */
        return 0;
    }
}

static int
exynos_i2c_master_stop(i2c_bus_t* i2c_bus)
{
    assert(!"Not implemented");
    return -1;
}

/* Exynos4 manual Figure 14-6 p14-9 */
static int
exynos_i2c_start_read(i2c_bus_t* i2c_bus, int slave, void* vdata, size_t len, i2c_callback_fn cb, void* token)
{
    struct i2c_bus_priv* dev;
    dev = i2c_bus_get_priv(i2c_bus);
    dprintf("Reading %d bytes from slave@0x%02x\n", len, slave);
    master_rxstart(dev, slave);

    /* Setup the RX descriptor */
    dev->rx_buf = (char*)vdata;
    dev->rx_len = len;
    dev->rx_count = -1;
    i2c_bus->cb = cb;
    i2c_bus->token = token;

    /* Wait for completion */
    if (cb == NULL) {
        while (busy(dev)) {
            i2c_handle_irq(i2c_bus);
        }
    } else {
        clear_pending(dev); // Clears any interrupts
        return dev->rx_len;
    }
    return dev->rx_count;
}

/* Exynos4 manual Figure 14-6 p14-8 */
static int
exynos_i2c_start_write(i2c_bus_t* i2c_bus, int slave, const void* vdata, size_t len, i2c_callback_fn cb, void* token)
{
    struct i2c_bus_priv* dev;
    dev = i2c_bus_get_priv(i2c_bus);
    dprintf("Writing %d bytes to slave@0x%02x\n", len, slave);
    master_txstart(dev, slave);

    dev->tx_count = -1;
    dev->tx_len = len;
    dev->tx_buf = (const char*)vdata;
    i2c_bus->cb = cb;
    i2c_bus->token = token;

    if (cb == NULL) {
        while (busy(dev)) {
            i2c_handle_irq(i2c_bus);
        }
    } else {
        clear_pending(dev); // Clears any interrupts
        return dev->tx_len;
    }
    return dev->tx_count;
}

static void
exynos_i2c_send_stop(i2c_bus_t* i2c_bus, enum i2c_stat status)
{
    struct i2c_bus_priv *dev = i2c_bus_get_priv(i2c_bus);
    dev->regs->status &= ~(I2CSTAT_BUSY);
    clear_pending(dev);
    if (i2c_bus->cb) {
        i2c_bus->cb(i2c_bus, status, dev->tx_count, i2c_bus->token);
    }
}

static void
exynos_i2c_handle_irq(i2c_bus_t* i2c_bus)
{
    struct i2c_bus_priv* dev;
    uint32_t v;
    uint32_t status;
    dev = i2c_bus_get_priv(i2c_bus);

    /* AddressedAsSlave seems to be read_to_clear so cache it immediately */
    status = dev->regs->status;
    if (!(status & I2CSTAT_ENABLE) || !irq_pending(dev)) {
        return;
    }

    /* Handle possible addressed as slave */
    if (status & I2CSTAT_ADDR_SLAVE) {
        dprintf("Addressed as slave.\n");
        /* Call out to the handler */
        if (i2c_bus->aas_cb) {
            enum i2c_mode mode;
            if ((dev->regs->status & I2CSTAT_MODE_MASK) == I2CSTAT_MODE_STX) {
                mode = I2CMODE_TX;
            } else {
                mode = I2CMODE_RX;
            }
            /* Is there a transfer in flight? */
            if (i2c_bus->cb && (dev->tx_len || dev->rx_len)) {
                size_t bytes;
                if (mode == I2CMODE_TX) {
                    bytes = dev->tx_count;
                } else {
                    bytes = dev->rx_count;
                }
                i2c_bus->cb(i2c_bus, I2CSTAT_INTERRUPTED, bytes, i2c_bus->token);
            }
            i2c_bus->aas_cb(i2c_bus, mode, i2c_bus->aas_token);
            /* Dummy read of address */
            if (mode == I2CMODE_RX) {
                (void)dev->regs->data;
                clear_pending(dev);
                return;
            } else {
                /* Fallthrough to transfer handler to write the first byte */
            }
        }
    }
    /* Handle transfer */
    switch (dev->regs->status & I2CSTAT_MODE_MASK) {
    case I2CSTAT_MODE_MRX:
        if (dev->rx_count < 0) {
            if (acked(dev)) {
                /* slave responded to the address */
                dev->rx_count = 0;
            } else {
                /* No response: Abort */
                exynos_i2c_send_stop(i2c_bus, I2CSTAT_ERROR);
            }
        } else if (dev->regs->control & I2CCON_ACK_EN) {
            /* Read from slave */
            v = dev->regs->data;
            *dev->rx_buf++ = v;
            dev->rx_count++;
            /* Don't ACK the last byte */
            if (dev->rx_count == dev->rx_len) {
                dev->regs->control &= ~(I2CCON_ACK_EN);
            }
        } else {
            /* Finally, send stop */
            exynos_i2c_send_stop(i2c_bus, I2CSTAT_COMPLETE);
        }
        break;
    case I2CSTAT_MODE_MTX:
        if (acked(dev)) {
            /* Start pumping out data */
            v = *dev->tx_buf++;
            dev->regs->data = v;
            dev->tx_count++;
            if (dev->tx_count == dev->tx_len) {
                /* Write 0xD0 (M/T Stop) to I2CSTAT */
                exynos_i2c_send_stop(i2c_bus, I2CSTAT_COMPLETE);
            }
        } else {
            /* No response: Abort */
            exynos_i2c_send_stop(i2c_bus, I2CSTAT_ERROR);
        }
        break;
    case I2CSTAT_MODE_SRX:
        /* Read in the data */
        *dev->rx_buf++ = dev->regs->data;
        dev->rx_count++;
        /* Last chance for user to supply another buffer before NACK */
        if (i2c_bus->cb && (dev->rx_count + 1 == dev->rx_len)) {
            i2c_bus->cb(i2c_bus, I2CSTAT_INCOMPLETE, dev->rx_count, i2c_bus->token);
        }
        /* If this is STILL the last byte, NACK it */
        if (dev->rx_count + 1 == dev->rx_len) {
            dev->regs->control &= ~I2CCON_ACK_EN;
        } else if (dev->rx_count == dev->rx_len) {
            dev->rx_len = 0;
            if (i2c_bus->cb) {
                i2c_bus->cb(i2c_bus, I2CSTAT_COMPLETE, dev->rx_count, i2c_bus->token);
            }
            /* Start ACKing again, ready to be addressed as slave */
            dev->regs->control |= I2CCON_ACK_EN;
        }

        break;
    case I2CSTAT_MODE_STX:
        if (dev->tx_count < dev->tx_len) {
            dev->regs->data = *dev->tx_buf++;
            dev->tx_count++;
        }
        /* Last byte? */
        if (!acked(dev)) {
            enum i2c_stat stat;
            if (dev->tx_len == dev->tx_count) {
                stat = I2CSTAT_COMPLETE;
            } else {
                stat = I2CSTAT_INCOMPLETE;
            }
            /* Now we need to handle the stop. For some reason, we need to prime
             * the data register first, but this byte will not actually be sent. */
            dev->regs->data = 0xff;
            dev->tx_len = 0;
            /* Signal the application */
            if (i2c_bus->cb) {
                i2c_bus->cb(i2c_bus, stat, dev->tx_count, i2c_bus->token);
            }
        } else if (dev->tx_count == dev->tx_len) {
            dev->tx_len = 0;
            if (i2c_bus->cb) {
                i2c_bus->cb(i2c_bus, I2CSTAT_COMPLETE, dev->tx_count, i2c_bus->token);
            }
        }
        break;
    default:
        assert(!"Unknown I2C mode");
    }
    clear_pending(dev);
}

static long
exynos_i2c_set_speed(i2c_bus_t* i2c_bus, long bps)
{
    assert(!"Not implemented\n");
    return -1;
}

static int
exynos_i2c_set_address(i2c_bus_t* i2c_bus, int addr, i2c_aas_callback_fn aas_cb, void* aas_token)
{
    struct i2c_bus_priv* dev;
    dev = i2c_bus_get_priv(i2c_bus);
    i2c_bus->aas_cb = aas_cb;
    i2c_bus->aas_token = aas_token;
    slave_init(dev, addr);
    dev->regs->line_control = BIT(2) | 0x3 ;
    return 0;
}

static int
i2c_init_common(mux_sys_t* mux, i2c_bus_t* i2c, struct i2c_bus_priv* dev)
{
    /* Check that our memory was mapped */
    if (dev->regs == NULL) {
        return -2;
    }
    dprintf("Memory for regs mapped\n");
    /* Configure MUX */
    if (mux_feature_enable(mux, dev->mux)) {
        dprintf("Warning: failed to configure MUX\n");
    }

    /* TODO setup clocks */
    //gate = ps_io_map(&io_ops->io_mapper, 0x1003C000, 0x1000, 0, PS_MEM_NORMAL);
    //printf("gates: 0x%08x\n", gate[0x950/4]);


    /* I2C setup */
    dev->regs->control = I2CCON_ACK_EN | 0 * I2CCON_CLK_SRC
                         | I2CCON_PRESCALE(7) | I2CCON_IRQ_EN;
    dev->regs->line_control = I2CLC_SDA_DELAY15CLK | I2CLC_FILT_EN;
    dev->regs->address = 0x54;

    i2c->start_read  = exynos_i2c_start_read;
    i2c->start_write = exynos_i2c_start_write;
    i2c->read        = exynos_i2c_read;
    i2c->write       = exynos_i2c_write;
    i2c->set_speed   = exynos_i2c_set_speed;
    i2c->set_address = exynos_i2c_set_address;
    i2c->master_stop = exynos_i2c_master_stop;
    i2c->handle_irq  = exynos_i2c_handle_irq;
    i2c->priv        = (void*)dev;

    i2c->cb = NULL;
    i2c->token = NULL;
    i2c->aas_cb = NULL;
    i2c->aas_token = NULL;
    return 0;
}

int
exynos_i2c_init(enum i2c_id id, void* base, mux_sys_t* mux, i2c_bus_t* i2c)
{
    struct i2c_bus_priv* dev = _i2c + id;
    dprintf("Mapping i2c %d\n", id);
    dev->regs = base;
    return i2c_init_common(mux, i2c, dev);
}

int
i2c_init(enum i2c_id id, ps_io_ops_t* io_ops, i2c_bus_t* i2c)
{
    struct i2c_bus_priv* dev = _i2c + id;
    mux_sys_t* mux = &io_ops->mux_sys;
    /* Map memory */
    dprintf("Mapping i2c %d\n", id);
    switch (id) {
    case I2C0:
        MAP_IF_NULL(io_ops, EXYNOS_I2C0,  dev->regs);
        break;
    case I2C1:
        MAP_IF_NULL(io_ops, EXYNOS_I2C1,  dev->regs);
        break;
    case I2C2:
        MAP_IF_NULL(io_ops, EXYNOS_I2C2,  dev->regs);
        break;
    case I2C3:
        MAP_IF_NULL(io_ops, EXYNOS_I2C3,  dev->regs);
        break;
    case I2C4:
        MAP_IF_NULL(io_ops, EXYNOS_I2C4,  dev->regs);
        break;
    case I2C5:
        MAP_IF_NULL(io_ops, EXYNOS_I2C5,  dev->regs);
        break;
    case I2C6:
        MAP_IF_NULL(io_ops, EXYNOS_I2C6,  dev->regs);
        break;
    case I2C7:
        MAP_IF_NULL(io_ops, EXYNOS_I2C7,  dev->regs);
        break;
#if defined(PLAT_EXYNOS4)
#elif defined(PLAT_EXYNOS5)
    case I2C8:
        MAP_IF_NULL(io_ops, EXYNOS_I2C8,  dev->regs);
        break;
    case I2C9:
        MAP_IF_NULL(io_ops, EXYNOS_I2C9,  dev->regs);
        break;
    case I2C10:
        MAP_IF_NULL(io_ops, EXYNOS_I2C10, dev->regs);
        break;
    case I2C11:
        MAP_IF_NULL(io_ops, EXYNOS_I2C11, dev->regs);
        break;
#else  /* EXYNOS? */
#error Unknown Exynos based platform
#endif /* EXYNOSX */
    default :
        return -1;
    }
    return i2c_init_common(mux, i2c, dev);
}

