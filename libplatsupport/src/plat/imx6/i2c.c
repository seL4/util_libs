/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <platsupport/i2c.h>
#include <platsupport/mux.h>
#include <platsupport/plat/mux.h>
#include <platsupport/clock.h>
#include <utils/util.h>
#include "../../services.h"
#include "../../arch/arm/clock.h"

#include <string.h>

#define IMX6_I2C_DEFAULT_FREQ (400 * KHZ)

#define IMX6_I2C1_PADDR 0x021A0000
#define IMX6_I2C2_PADDR 0x021A4000
#define IMX6_I2C3_PADDR 0x021A8000

#define IMX6_I2CX_SIZE  0x1000
#define IMX6_I2C1_SIZE  IMX6_I2CX_SIZE
#define IMX6_I2C2_SIZE  IMX6_I2CX_SIZE
#define IMX6_I2C3_SIZE  IMX6_I2CX_SIZE

struct imx6_i2c_regs {
#define I2CADDR(x)         (((x) & 0xff) * BIT(0))
#define I2CADDR_MASK       I2CADDR(0xff)
    uint16_t address;
    uint16_t res0;
    /* Can't find a pattern here for the divider... May need a look up table */
    uint16_t div;
    uint16_t res1;
#define I2CCON_ENABLE      BIT(7)
#define I2CCON_IRQ_ENABLE  BIT(6)
#define I2CCON_MASTER      BIT(5)
#define I2CCON_TXEN        BIT(4)
#define I2CCON_ACK_EN      BIT(3)
#define I2CCON_RSTART      BIT(2)
    uint16_t control;
    uint16_t res2;
#define I2CSTAT_XFER       BIT(7)
#define I2CSTAT_IAAS       BIT(6)
#define I2CSTAT_BUSY       BIT(5)
#define I2CSTAT_ARBLOST    BIT(4)
#define I2CSTAT_SLAVETX    BIT(2)
#define I2CSTAT_IRQ_PEND   BIT(1)
#define I2CSTAT_NAK        BIT(0)
    uint16_t status;
    uint16_t res3;
#define I2CDATA(x)           (((x) & 0xff) * BIT(0))
#define I2CDATA_MASK         I2CDATA(0xff)
#define I2CDATA_READ(addr)   I2CDATA(((addr) & 0xfe) | 1)
#define I2CDATA_WRITE(addr)  I2CDATA(((addr) & 0xfe) | 0)
    uint16_t data;
    uint16_t res4;
};

struct i2c_bus_priv {
    volatile struct imx6_i2c_regs* regs;
    char* rx_buf;
    int rx_count;
    int rx_len;
    const char* tx_buf;
    int tx_count;
    int tx_len;
    int mode_tx;

    i2c_callback_fn cb;
    void* token;

    mux_feature_t mux;
    enum clock_gate clk_gate;
    struct clock clock;
};

/********************
 *** I2C clocking ***
 ********************/

static struct i2c_bus_priv*
i2c_clk_get_priv(clk_t* clk) {
    return (struct i2c_bus_priv*)clk->priv;
}

/* Well this is annoying... Is there a magical algorigm that can be used to
 * build this table? */
static const int _i2c_div_map[64] = {
    30,   32,   36,   42,   48,   52,   60,   72,   80,   88,  104,  128,  144,
    160,  192,  240,  288,  320,  384,  480,  576,  640,  768,  960, 1152, 1280,
    1536, 1920, 2304, 2560, 3072, 3840,   22,   24,   26,   28,   32,   36,   40,
    44,   48,   56,   64,   72,   80,   96,  112,  128,  160,  192,  224,  256,
    320,  384,  448,  512,  640,  768,  896, 1024, 1280, 1536, 1792, 2048
};

static int
_i2c_prescale_decode(int div)
{
    int error = 0xffff;
    int match;
    int i;
    /* By default, just choose the lowest frequency */
    match = 0;
    /* Scan for a prescaler which minimizes error */
    for (i = 0; i < sizeof(_i2c_div_map) / sizeof(*_i2c_div_map); i++) {
        int this_error;
        this_error = _i2c_div_map[i] - div;
        /* Absolute error value */
        if (this_error < 0) {
            this_error *= -1;
        }
        /* Update prescale value, early exit if exact match */
        if (this_error == 0) {
            return i;
        } else if (this_error < error) {
            error = this_error;
            match = i;
        }
    }
    return match;
}

static clk_t*
_i2c_clk_init(clk_t* clk)
{
    struct i2c_bus_priv* dev = i2c_clk_get_priv(clk);
    assert(dev != NULL);
    if (clk->parent == NULL) {
        clk_t* parent = clk_get_clock(clk->clk_sys, CLK_PERCLK);
        assert(parent != NULL);
        clk_register_child(parent, clk);
    }
    clk_set_freq(clk, IMX6_I2C_DEFAULT_FREQ);
    clk_gate_enable(clk->clk_sys, dev->clk_gate, CLKGATE_ON);
    return clk;
}

static freq_t
_i2c_clk_get_freq(clk_t* clk)
{
    freq_t fin = clk_get_freq(clk->parent);
    struct i2c_bus_priv* dev = i2c_clk_get_priv(clk);
    int div = _i2c_div_map[dev->regs->div];
    return fin / div;
}

static freq_t
_i2c_clk_set_freq(clk_t* clk, freq_t hz)
{
    freq_t fin = clk_get_freq(clk->parent);
    struct i2c_bus_priv* dev = i2c_clk_get_priv(clk);
    uint32_t div = fin / hz;
    assert((div > 22 && div <= 3840) || !"Parent calibration not implemented");
    dev->regs->div = _i2c_prescale_decode(div);
    return clk_get_freq(clk);
}

static void
_i2c_clk_recal(clk_t* clk)
{
    assert(!"IMPLEMENT ME");
}

/***********************
 **** Device config ****
 ***********************/

static struct i2c_bus_priv _i2c[NI2C] = {
    {
        .regs = NULL,
        .mux = MUX_I2C1,
        .clk_gate = i2c1_serial,
        .clock = {
            CLK_OPS_CUSTOM("I2C1", i2c_clk, &_i2c[0])
        }
    },
    {
        .regs = NULL,
        .mux = MUX_I2C2,
        .clk_gate = i2c2_serial,
        .clock = {
            CLK_OPS_CUSTOM("I2C2", i2c_clk, &_i2c[1])
        }
    },
    {
        .regs = NULL,
        .mux = MUX_I2C3,
        .clk_gate = i2c3_serial,
        .clock = {
            CLK_OPS_CUSTOM("I2C3", i2c_clk, &_i2c[2])
        }
    }
};

/******************
 **** I2C Core ****
 ******************/

static inline struct i2c_bus_priv*
i2c_bus_get_priv(i2c_bus_t* i2c_bus) {
    return (struct i2c_bus_priv*)i2c_bus->priv;
}

static inline int
busy(struct i2c_bus_priv* dev)
{
    return !!(dev->regs->status & I2CSTAT_BUSY);
}

static inline int
irq_pending(struct i2c_bus_priv* dev)
{
    return !!(dev->regs->status & I2CSTAT_IRQ_PEND);
}

static inline void
clear_pending(struct i2c_bus_priv* dev)
{
    dev->regs->status &= ~(I2CSTAT_IRQ_PEND);
}

static inline int
acked(struct i2c_bus_priv* dev)
{
    return !(dev->regs->status & I2CSTAT_NAK);
}

static inline void
master_stop(struct i2c_bus_priv* dev)
{
    /* Send stop signal */
    dev->regs->control &= ~I2CCON_MASTER;
    /* Wait for idle bus */
    while (busy(dev));
    /* Disable the bus */
    dev->regs->control &= ~(I2CCON_MASTER | I2CCON_TXEN      |
                            I2CCON_ENABLE | I2CCON_IRQ_ENABLE);
}

static void
master_start(struct i2c_bus_priv* dev, char addr)
{
    /* Enable the bus */
    dev->regs->control |= I2CCON_ENABLE | I2CCON_IRQ_ENABLE;
    while (busy(dev));
    /* Enter master TX mode */
    dev->regs->control |= I2CCON_MASTER | I2CCON_TXEN;
    while (!busy(dev));
    clear_pending(dev);
    /* Write slave address */
    dev->regs->data = addr;
}

static void
internal_slave_init(struct i2c_bus_priv* dev, char addr)
{
    dev->regs->address = addr;
    /* Enable the bus */
    dev->regs->control |= I2CCON_ENABLE | I2CCON_IRQ_ENABLE;
    while (busy(dev));
    /* Enter slave mode TX mode */
    dev->regs->control &= ~I2CCON_MASTER;
}

static void
imx6_i2c_handle_irq(i2c_bus_t* i2c_bus)
{
    struct i2c_bus_priv* dev;
    dev = i2c_bus_get_priv(i2c_bus);
    if (irq_pending(dev)) {
        /* Clear IF */
        clear_pending(dev);
        /* Master Mode? */
        if (dev->regs->control & I2CCON_MASTER) {
            if (dev->regs->control & I2CCON_TXEN) {
                /** Master TX **/
                if (!acked(dev)) {
                    /* RXAK != 0 */
                    ZF_LOGD("NACK from slave");
                    master_stop(dev);
                } else if (dev->mode_tx && dev->tx_count == dev->tx_len) {
                    /* Last byte transmitted successfully */
                    master_stop(dev);
                } else if (!dev->mode_tx) {
                    /* End of address cycle for master RX */
                    dev->regs->control &= ~I2CCON_TXEN;
                    (void)dev->regs->data;
                    dev->rx_count = 0;
                } else {
                    /* Write next byte */
                    dev->regs->data = *dev->tx_buf++;
                    dev->tx_count++;
                }
            } else {
                /** Master RX **/
                if (dev->rx_count < dev->rx_len) {
                    if (dev->rx_count + 2 == dev->rx_len) {
                        /* Second last byte to be read: Generate NACK */
                        dev->regs->control |= I2CCON_ACK_EN;
                    }
                    if (dev->rx_count + 1 == dev->rx_len) {
                        /* Last byte to be read: Generate stop */
                        dev->regs->control &= ~I2CCON_MASTER;
                    }
                    *dev->rx_buf++ = dev->regs->data;
                    dev->rx_count++;
                    if (dev->rx_count == dev->rx_len) {
                        /* Transfer complete */
                        master_stop(dev);
                    }
                } else {
                    ZF_LOGD("Master RX IRQ but RX complete!");
                }
            }
        } else {
            /* Slave mode */
        }
    }
}

static inline void
master_txstart(struct i2c_bus_priv* dev, int slave)
{
    master_start(dev, I2CDATA_WRITE(slave));
}

static inline void
master_rxstart(struct i2c_bus_priv* dev, int slave)
{
    master_start(dev, I2CDATA_READ(slave));
}

static int
imx6_i2c_read(i2c_bus_t* i2c_bus, void* data, size_t len, UNUSED bool send_stop, i2c_callback_fn cb, void* token)
{
    ZF_LOGF("Not implemented");
    return -1;
}

static int
imx6_i2c_write(i2c_bus_t* i2c_bus, const void* data, size_t len, UNUSED bool send_stop, i2c_callback_fn cb, void* token)
{
    ZF_LOGF("Not implemented");
    return -1;
}

static int
imx6_i2c_master_stop(i2c_bus_t* i2c_bus)
{
    ZF_LOGF("Not implemented");
    return -1;
}

static int
imx6_i2c_start_write(i2c_slave_t* sl,
                     const void* vdata, size_t len,
                     UNUSED bool end_with_repeat_start,
                     i2c_callback_fn cb, void* token)
{
    struct i2c_bus_priv* dev;

    assert(sl != NULL && sl->bus != NULL);

    dev = i2c_bus_get_priv(sl->bus);
    ZF_LOGD("Writing %d bytes to slave@0x%02x", len, sl->address);
    master_txstart(dev, sl->address);

    dev->tx_count = 0;
    dev->tx_buf = (const char*)vdata;
    dev->tx_len = len;
    dev->mode_tx = 1;
    dev->cb = cb;
    dev->token = token;

    if (cb == NULL) {
        while (busy(dev)) {
            i2c_handle_irq(sl->bus);
        }
        return dev->tx_count;
    } else {
        return len;
    }
}

static int
imx6_i2c_start_read(i2c_slave_t* sl,
                    void* vdata, size_t len,
                    UNUSED bool end_with_repeat_start,
                    i2c_callback_fn cb, void* token)
{
    struct i2c_bus_priv* dev;

    assert(sl != NULL && sl->bus != NULL);

    dev = i2c_bus_get_priv(sl->bus);
    ZF_LOGD("Reading %d bytes from slave@0x%02x", len, sl->address);
    if (sl->address == dev->regs->address) {
        return -1;
    }
    master_rxstart(dev, sl->address);

    dev->rx_count = -1;
    dev->rx_buf = (char*)vdata;
    dev->rx_len = len;
    dev->mode_tx = 0;
    dev->cb = cb;
    dev->token = token;

    if (cb == NULL) {
        while (busy(dev)) {
            i2c_handle_irq(sl->bus);
        }
        return dev->rx_count;
    } else {
        return len;
    }
}

static int
imx6_i2c_set_address(i2c_bus_t* i2c_bus, int addr)
{
    struct i2c_bus_priv* dev;
    dev = i2c_bus_get_priv(i2c_bus);

    internal_slave_init(dev, addr);
    return 0;
}

void
imx6_i2c_register_slave_event_handler(i2c_bus_t *bus,
                                      i2c_aas_callback_fn cb, void *token)
{
    assert(bus != NULL);
    bus->aas_cb = cb;
    bus->aas_token = token;
}

static const uint32_t i2c_speed_freqs[] = {
    [I2C_SLAVE_SPEED_STANDARD] = 100000,
    [I2C_SLAVE_SPEED_FAST] = 400000,
    [I2C_SLAVE_SPEED_FASTPLUS] = 1000000,
    [I2C_SLAVE_SPEED_HIGHSPEED] = 3400000
};

static long
imx6_i2c_set_speed(i2c_bus_t* i2c_bus, enum i2c_slave_speed speed)
{
    struct i2c_bus_priv* dev;

    if (speed < I2C_SLAVE_SPEED_STANDARD || speed > I2C_SLAVE_SPEED_HIGHSPEED) {
        ZF_LOGE("imx6: I2C: Unsupported speed %d.", speed);
        return -1;
    }

    dev = i2c_bus_get_priv(i2c_bus);
    /* "speed" is validated in the library code in arch_include/i2c.h. */
    return clk_set_freq(&dev->clock, i2c_speed_freqs[speed]);
}

int
imx6_i2c_slave_init(i2c_bus_t* i2c_bus, int address,
                    enum i2c_slave_address_size address_size,
                    enum i2c_slave_speed max_speed,
                    uint32_t flags,
                    i2c_slave_t* sl)
{
    assert(sl != NULL);

    if (address_size == I2C_SLAVE_ADDR_7BIT) {
        address = i2c_extract_address(address);
    }

    sl->address = address;
    sl->address_size = address_size;
    sl->max_speed = max_speed;
    sl->i2c_opts = flags;
    sl->bus = i2c_bus;

    sl->slave_read      = &imx6_i2c_start_read;
    sl->slave_write     = &imx6_i2c_start_write;

    return 0;
}

int
i2c_init(enum i2c_id id, ps_io_ops_t* io_ops, i2c_bus_t* i2c)
{
    struct i2c_bus_priv* dev = _i2c + id;
    int err;
    clk_t* i2c_clk;
    /* Map memory */
    ZF_LOGD("Mapping i2c %d\n", id);
    switch (id) {
    case I2C1:
        MAP_IF_NULL(io_ops, IMX6_I2C1, dev->regs);
        break;
    case I2C2:
        MAP_IF_NULL(io_ops, IMX6_I2C2, dev->regs);
        break;
    case I2C3:
        MAP_IF_NULL(io_ops, IMX6_I2C3, dev->regs);
        break;
    default :
        return -1;
    }
    /* Check that our memory was mapped */
    if (dev->regs == NULL) {
        return -2;
    }

    /* Configure MUX */
    err = mux_feature_enable(&io_ops->mux_sys, dev->mux, MUX_DIR_NOT_A_GPIO);
    if (err) {
        assert(!"Failed to configure I2C mux");
        return -1;
    }

    /* Init clock */
    dev->clock.clk_sys = &io_ops->clock_sys;
    i2c_clk = clk_init(&dev->clock);
    if (i2c_clk == NULL) {
        assert(!"Failed to initialise I2C clock");
        return -1;
    }
    /* I2C setup */
    dev->regs->control &= ~(I2CCON_ENABLE | I2CCON_TXEN);
    dev->regs->address = 0x00;
    dev->regs->control = I2CCON_ACK_EN;

    i2c->read        = imx6_i2c_read;
    i2c->write       = imx6_i2c_write;
    i2c->set_speed   = imx6_i2c_set_speed;
    i2c->set_self_slave_address = imx6_i2c_set_address;
    i2c->register_slave_event_handler = imx6_i2c_register_slave_event_handler;
    i2c->master_stop = imx6_i2c_master_stop;
    i2c->handle_irq  = imx6_i2c_handle_irq;
    i2c->priv        = (void*)dev;
    i2c->slave_init  = imx6_i2c_slave_init;
    return 0;
}
