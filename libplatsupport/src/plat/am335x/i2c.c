/*
 * Copyright 2019, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

/* warning: use of callbacks is NOT tested with this driver */

#include <platsupport/i2c.h>
#include <platsupport/pmem.h>
#include <platsupport/plat/i2c.h>

struct omap4_i2c_dev {
    void *regs;
    int irq_id;
    enum i2c_slave_speed speed;
    enum i2c_mode mode;
    enum i2c_stat status;
    uint16_t fifo_threshold;
    uint8_t *buf;
    size_t buf_len;
    size_t buf_pos;
    bool repeat_start;
    bool interrupts_enabled;
    volatile bool busy;
};
typedef struct omap4_i2c_dev omap4_i2c_dev_t;

static pmem_region_t pmems[] = {
    {
        .type = PMEM_TYPE_DEVICE,
        .base_addr = AM335X_I2C0_PADDR,
        .length = PAGE_SIZE_4K
    },
    {
        .type = PMEM_TYPE_DEVICE,
        .base_addr = AM335X_I2C1_PADDR,
        .length = PAGE_SIZE_4K
    },
    {
        .type = PMEM_TYPE_DEVICE,
        .base_addr = AM335X_I2C2_PADDR,
        .length = PAGE_SIZE_4K
    }
};

static ps_irq_t irqs[] = {
    {
        .type = PS_INTERRUPT,
        .irq.number = AM335X_I2C0_IRQ
    },
    {
        .type = PS_INTERRUPT,
        .irq.number = AM335X_I2C1_IRQ
    },
    {
        .type = PS_INTERRUPT,
        .irq.number = AM335X_I2C2_IRQ
    }
};

static const uint32_t i2c_speed_freqs[] = {
    [I2C_SLAVE_SPEED_STANDARD] = 100000,
    [I2C_SLAVE_SPEED_FAST] = 400000
};

static inline uint16_t omap4_i2c_reg_read(omap4_i2c_dev_t *dev, int addr)
{
    return *(volatile uint16_t *)(dev->regs + addr);
}

static inline void omap4_i2c_reg_write(omap4_i2c_dev_t *dev, int addr, uint16_t val)
{
    *(volatile uint16_t *)(dev->regs + addr) = val;
}

static void omap4_i2c_enable_interrupts(omap4_i2c_dev_t *dev)
{
    dev->interrupts_enabled = true;
    omap4_i2c_reg_write(dev, OMAP4_I2C_IRQENABLE_SET, IRQENABLE_XDR | IRQENABLE_RDR |
                        IRQENABLE_XRDY | IRQENABLE_RRDY | IRQENABLE_ARDY | IRQENABLE_NACK);
}

static void omap4_i2c_disable_interrupts(omap4_i2c_dev_t *dev)
{
    dev->interrupts_enabled = false;
    omap4_i2c_reg_write(dev, OMAP4_I2C_IRQENABLE_CLR, IRQENABLE_XDR | IRQENABLE_RDR |
                        IRQENABLE_XRDY | IRQENABLE_RRDY | IRQENABLE_ARDY | IRQENABLE_NACK);
}

static void omap4_i2c_wait_for_bb(i2c_bus_t *bus)
{
    omap4_i2c_dev_t *dev = bus->priv;
    while (omap4_i2c_reg_read(dev, OMAP4_I2C_IRQSTATUS_RAW) & IRQSTATUS_BB);
}

static int omap4_i2c_controller_init(i2c_bus_t *bus)
{
    omap4_i2c_dev_t *dev = bus->priv;

    /* set prescaler and SCL timings */
    int internal_clock;
    uint16_t scll, sclh;
    uint16_t prescale;

    /* these values are taken from Table 23-9, OMAP4460 Technical Reference Manual, version AB */
    switch (dev->speed) {
    case I2C_SLAVE_SPEED_STANDARD:
        internal_clock = 4000000;
        scll = 13;
        sclh = 15;
        break;
    case I2C_SLAVE_SPEED_FAST:
        internal_clock = 9600000;
        scll = 7;
        sclh = 5;
        break;
    default:
        ZF_LOGE("Unsupported I2C speed!");
        return -1;
    }

    prescale = AM335X_I2C_SCLK / internal_clock - 1;

    /* disable I2C module for reconfiguration */
    omap4_i2c_reg_write(dev, OMAP4_I2C_CON, 0);

    /* write clock configuration */
    omap4_i2c_reg_write(dev, OMAP4_I2C_PSC, prescale);
    omap4_i2c_reg_write(dev, OMAP4_I2C_SCLL, scll);
    omap4_i2c_reg_write(dev, OMAP4_I2C_SCLH, sclh);

    /* clamp FIFO depth to maximum possible size */
    dev->fifo_threshold = MIN(dev->fifo_threshold, AM335X_I2C_MAX_FIFODEPTH);
    /* configure FIFO */
    uint16_t buf_reg = ((dev->fifo_threshold - 1) << BUF_RXTRSH_OFFSET) & BUF_RXTRSH_MASK;
    buf_reg |= (dev->fifo_threshold - 1) & BUF_TXTRSH_MASK;
    omap4_i2c_reg_write(dev, OMAP4_I2C_BUF, buf_reg);

    /* enable module */
    omap4_i2c_reg_write(dev, OMAP4_I2C_CON, CON_I2C_EN);

    return 0;
}

static int omap4_i2c_do_xfer(i2c_slave_t *slave, void *data, size_t size, bool write,
                             bool repeat_start, i2c_callback_fn cb, void *token)
{
    ZF_LOGV("%s %zu bytes from slave 0x%x", write ? "writing" : "reading", size, slave->address);
    i2c_bus_t *bus = slave->bus;
    omap4_i2c_dev_t *dev = bus->priv;

    if (dev->busy) {
        ZF_LOGE("i2c bus is busy ");
        return -1;
    }

    if (size == 0) {
        cb(bus, I2CSTAT_COMPLETE, size, token);
        return 0;
    }

    /* skip polling for bus-busy if the device is in repeat-start mode */
    if (!dev->repeat_start) {
        omap4_i2c_wait_for_bb(bus);
    }

    if (slave->max_speed < dev->speed) {
        uint32_t freq = i2c_set_speed(bus, slave->max_speed);
        if (freq != i2c_speed_freqs[slave->max_speed]) {
            ZF_LOGE("failed to set speed");
            return -1;
        }
    }

    omap4_i2c_reg_write(dev, OMAP4_I2C_SA, slave->address);

    if (write) {
        dev->mode = I2CMODE_TX;
    } else {
        dev->mode = I2CMODE_RX;
    }
    dev->buf = data;
    dev->buf_len = size;
    dev->buf_pos = 0;
    dev->repeat_start = repeat_start;
    dev->status = I2CSTAT_COMPLETE;

    omap4_i2c_reg_write(dev, OMAP4_I2C_CNT, size);

    /* clear FIFO */
    uint16_t buf_reg = omap4_i2c_reg_read(dev, OMAP4_I2C_BUF);
    buf_reg |= BUF_RXFIFO_CLR | BUF_TXFIFO_CLR;
    omap4_i2c_reg_write(dev, OMAP4_I2C_BUF, buf_reg);

    uint16_t con_reg = CON_I2C_EN | CON_MST | CON_STT;
    if (!repeat_start) {
        con_reg |= CON_STP;
    }
    if (write) {
        con_reg |= CON_TRX;
    }

    omap4_i2c_reg_write(dev, OMAP4_I2C_CON, con_reg);

    dev->busy = true;
    if (cb == NULL) {
        /* synchronous */
        while (dev->busy) {
            i2c_handle_irq(bus);
        }
        return dev->buf_pos;
    } else {
        /* asynchronous */
        bus->cb = cb;
        bus->token = token;

        omap4_i2c_enable_interrupts(dev);
        return size;
    }
    return 0;
}

static int omap4_i2c_slave_read(i2c_slave_t *slave, void *data, size_t size,
                                bool repeat_start, i2c_callback_fn cb, void *token)
{
    return omap4_i2c_do_xfer(slave, data, size, false, repeat_start, cb, token);
}

static int omap4_i2c_slave_write(i2c_slave_t *slave, const void *data, size_t size,
                                 bool repeat_start, i2c_callback_fn cb, void *token)
{
    return omap4_i2c_do_xfer(slave, (void *) data, size, true, repeat_start, cb, token);
}

static int omap4_i2c_slave_init(i2c_bus_t *bus, int address, enum i2c_slave_address_size address_size,
                                enum i2c_slave_speed max_speed, uint32_t i2c_opts, i2c_slave_t *i2c_slave)
{
    assert(i2c_slave != NULL);

    if (address_size == I2C_SLAVE_ADDR_7BIT) {
        address = i2c_extract_address(address);
    }

    *i2c_slave = (i2c_slave_t) {
        .address = address,
        .address_size = address_size,
        .max_speed = max_speed,
        .i2c_opts = i2c_opts,
        .bus = bus,

        .slave_read = &omap4_i2c_slave_read,
        .slave_write = &omap4_i2c_slave_write
    };

    return 0;
}

static long omap4_i2c_set_speed(i2c_bus_t *bus, enum i2c_slave_speed speed)
{
    omap4_i2c_dev_t *dev = bus->priv;
    dev->speed = speed;

    omap4_i2c_wait_for_bb(bus);
    int err = omap4_i2c_controller_init(bus);
    if (err) {
        return err;
    }

    return i2c_speed_freqs[speed];
}

static int omap4_i2c_master_stop(i2c_bus_t *bus)
{
    omap4_i2c_dev_t *dev = bus->priv;

    uint16_t con_reg = omap4_i2c_reg_read(dev, OMAP4_I2C_CON);
    con_reg |= CON_STP;
    omap4_i2c_reg_write(dev, OMAP4_I2C_CON, con_reg);

    omap4_i2c_wait_for_bb(bus);

    omap4_i2c_reg_write(dev, OMAP4_I2C_CON, 0);
    return 0;
}

static void omap4_i2c_handle_irq(i2c_bus_t *bus)
{
    omap4_i2c_dev_t *dev = bus->priv;
    size_t bytes = 0;

    uint16_t irq_status = omap4_i2c_reg_read(dev, OMAP4_I2C_IRQSTATUS_RAW);
    if (dev->interrupts_enabled) {
        uint16_t irq_enabled = omap4_i2c_reg_read(dev, OMAP4_I2C_IRQENABLE_SET);

        /* mask disabled interrupts */
        irq_status &= irq_enabled;
    }

    ZF_LOGV("IRQSTATUS = 0x%x", irq_status);

    if (irq_status & IRQSTATUS_NACK) {
        /* NACK from slave */
        ZF_LOGV("NACK");
        dev->status = I2CSTAT_NACK;
        omap4_i2c_reg_write(dev, OMAP4_I2C_IRQSTATUS, IRQSTATUS_NACK);
    }

    if (irq_status & IRQSTATUS_ARDY) {
        /* transfer complete */
        ZF_LOGV("ARDY");

        omap4_i2c_reg_write(dev, OMAP4_I2C_IRQSTATUS, IRQSTATUS_ARDY | IRQSTATUS_RRDY | IRQSTATUS_XRDY |
                            IRQSTATUS_RDR | IRQSTATUS_XDR);

        /* run callback */
        dev->mode = I2CMODE_IDLE;
        dev->busy = false;
        if (bus->cb) {
            bus->cb(bus, dev->status, dev->buf_pos, bus->token);
            bus->cb = NULL;
            bus->token = NULL;
        }

        if (dev->interrupts_enabled) {
            omap4_i2c_disable_interrupts(dev);
        }

        return;
    }

    if (dev->mode == I2CMODE_RX) {
        if (irq_status & IRQSTATUS_RDR) {
            /* receive drain */
            ZF_LOGV("RDR");
            bytes = dev->buf_len - dev->buf_pos;
        } else if (irq_status & IRQSTATUS_RRDY) {
            /* receive ready */
            ZF_LOGV("RRDY");
            bytes = MIN(dev->fifo_threshold, dev->buf_len - dev->buf_pos);
        }

        for (size_t i = 0; i < bytes; i++) {
            dev->buf[dev->buf_pos] = omap4_i2c_reg_read(dev, OMAP4_I2C_DATA);
            dev->buf_pos++;
        }

        if (irq_status & IRQSTATUS_RDR) {
            omap4_i2c_reg_write(dev, OMAP4_I2C_IRQSTATUS, IRQSTATUS_RDR);
        }
        if (irq_status & IRQSTATUS_RRDY) {
            omap4_i2c_reg_write(dev, OMAP4_I2C_IRQSTATUS, IRQSTATUS_RRDY);
        }
    }

    if (dev->mode == I2CMODE_TX) {
        if (irq_status & IRQSTATUS_XDR) {
            /* transmit drain */
            ZF_LOGV("XDR");
            bytes = dev->buf_len - dev->buf_pos;
        } else if (irq_status & IRQSTATUS_XRDY) {
            /* transmit ready */
            ZF_LOGV("XRDY");
            bytes = MIN(dev->fifo_threshold, dev->buf_len - dev->buf_pos);
        }

        for (size_t i = 0; i < bytes; i++) {
            omap4_i2c_reg_write(dev, OMAP4_I2C_DATA, dev->buf[dev->buf_pos]);
            dev->buf_pos++;
        }

        if (irq_status & IRQSTATUS_XDR) {
            omap4_i2c_reg_write(dev, OMAP4_I2C_IRQSTATUS, IRQSTATUS_XDR);
        }
        if (irq_status & IRQSTATUS_XRDY) {
            omap4_i2c_reg_write(dev, OMAP4_I2C_IRQSTATUS, IRQSTATUS_XRDY);
        }
    }
}

int omap4_i2c_init(void *vaddr, int irq_id, ps_io_ops_t *io_ops, i2c_bus_t *i2c_bus)
{
    struct omap4_i2c_dev *dev;

    int error = ps_malloc(&io_ops->malloc_ops, sizeof(omap4_i2c_dev_t), (void **) &dev);
    if (error) {
        ZF_LOGE("Failed to allocate device");
        return -1;
    }

    *dev = (omap4_i2c_dev_t) {
        .regs = vaddr,
        .irq_id = irq_id,
        .speed = I2C_SLAVE_SPEED_FAST,
        .fifo_threshold = AM335X_I2C_MAX_FIFODEPTH - 1
    };
    *i2c_bus = (i2c_bus_t) {
        .slave_init = omap4_i2c_slave_init,
        .set_speed = omap4_i2c_set_speed,
        .master_stop = omap4_i2c_master_stop,
        .handle_irq = omap4_i2c_handle_irq,
        .priv = dev
    };

    error = omap4_i2c_controller_init(i2c_bus);
    if (error) {
        ZF_LOGE("Failed to initialise I2C controller");
        ps_free(&io_ops->malloc_ops, sizeof(omap4_i2c_dev_t), dev);
        i2c_bus->priv = NULL;
        return -1;
    }

    return 0;
}

int i2c_init(enum i2c_id id, ps_io_ops_t *io_ops, i2c_bus_t *i2c_bus)
{
    void *vaddr;
    int irq_id;

    assert(io_ops != NULL);
    assert(i2c_bus != NULL);

    switch (id) {
    case AM335X_I2C0:
    case AM335X_I2C1:
    case AM335X_I2C2:
        vaddr = ps_pmem_map(io_ops, pmems[id], false, PS_MEM_NORMAL);
        if (vaddr == NULL) {
            ZF_LOGE("Failed to map I2C controller %d", id);
            return -1;
        }

        irq_id = ps_irq_register(&io_ops->irq_ops, irqs[id], i2c_handle_irq_wrapper, i2c_bus);
        if (irq_id < 0) {
            ZF_LOGE("Failed to register IRQ handler for I2C controller %d", id);
            ps_pmem_unmap(io_ops, pmems[id], vaddr);
            return -1;
        }

        break;
    default:
        ZF_LOGE("Unknown I2C controller %d", id);
        return -1;
    }

    return omap4_i2c_init(vaddr, irq_id, io_ops, i2c_bus);
}
