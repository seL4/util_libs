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

#include <autoconf.h>

/* To extend support, provide platform specific i2c.h and gpio.h and delay.c */
#if defined(CONFIG_PLAT_EXYNOS5) || defined(CONFIG_PLAT_EXYNOS4) || defined(CONFIG_PLAT_IMX6)

#include <platsupport/i2c.h>
#include <platsupport/delay.h>
#include "../../services.h"

#define DEFAULT_SPEED 100*1000

static inline struct i2c_bb*
i2c_bus_get_priv(struct i2c_bus* i2c_bus) {
    return (struct i2c_bb*)i2c_bus->priv;
}

static inline void
_hold(struct i2c_bb* d)
{
    ps_udelay(1000000 / d->speed + 1);
}

static inline void
pin_l(struct i2c_bb* d, int gpio)
{
    gpio_t g;
    gpio_new(d->gpio_sys, gpio, GPIO_DIR_OUT, &g);
    gpio_clr(&g);
}

static inline int
pin_r(struct i2c_bb* d, int gpio)
{
    gpio_t g;
    gpio_new(d->gpio_sys, gpio, GPIO_DIR_IN, &g);
    return gpio_get(&g);
}

static inline void
pin_h(struct i2c_bb* d, int gpio)
{
    pin_r(d, gpio);
}

static inline int  sda_r (struct i2c_bb* d)
{
    return pin_r(d, d->sda);
}
static inline void sda_l (struct i2c_bb* d)
{
    pin_l(d, d->sda);
}
static inline void sda_h (struct i2c_bb* d)
{
    pin_h(d, d->sda);
}

static inline void scl_l (struct i2c_bb* d)
{
    pin_l(d, d->scl);
}
static inline void scl_h (struct i2c_bb* d)
{
    while (!pin_r(d, d->scl));
}

static inline void
clk(struct i2c_bb* d)
{
    _hold  (d);
    scl_h  (d);
    _hold  (d);
    scl_l  (d);
}

static inline void
i2c_bb_start(struct i2c_bb* d)
{
    /* init */
    while (!sda_r(d));
    _hold  (d);
    /* Start */
    sda_l  (d);
    _hold  (d);
    scl_l  (d);
}

static inline void
i2c_bb_stop(struct i2c_bb* d)
{
    sda_l  (d);
    _hold  (d);
    scl_h  (d);
    ps_udelay(1000);
    while (!sda_r(d));
    _hold  (d);
}

static inline void
i2c_bb_sendbit(struct i2c_bb* d, int bit)
{
    /* Setup bit */
    if (bit) {
        sda_h (d);
    } else {
        sda_l (d);
    }
    _hold  (d);
    /* Raise clock, wait for clock stretching */
    scl_h  (d);
    _hold  (d);
    /* Lower clock */
    scl_l  (d);
}

static inline int
i2c_bb_readbit(struct i2c_bb* d)
{
    int bit;
    /* Release SDA */
    sda_r  (d);
    _hold  (d);
    /* Raise clock, wait for clock stretching */
    scl_h  (d);
    /* Data now read to be read */
    bit = sda_r(d);
    _hold  (d);
    scl_l  (d);
    return bit;
}

static int
i2c_bb_sendbyte(struct i2c_bb* d, char b)
{
    int i;
    unsigned char mask = 0x80;
    for (i = 0; i < 8; i++) {
        i2c_bb_sendbit(d, !!(b & mask));
        mask >>= 1;
    }
    /* ACK */
    return i2c_bb_readbit(d);
}

static int
i2c_bb_readbyte(struct i2c_bb* d, int send_nak)
{
    int i;
    char data = 0;
    for (i = 0; i < 8; i++) {
        data <<= 1;
        data |= (i2c_bb_readbit(d)) ? 1 : 0;
    }
    i2c_bb_sendbit(d, send_nak);
    return data;
}

static int
i2c_bb_read(i2c_bus_t* bus, void* buf, size_t size,
            UNUSED bool end_with_repeat_start,
            i2c_callback_fn cb, void* token)
{
    assert(!"Not implemented\n");
    return -1;
}

static int
i2c_bb_write(i2c_bus_t* bus, const void* buf, size_t size,
             UNUSED bool end_with_repeat_start,
             i2c_callback_fn cb, void* token)
{
    assert(!"Not implemented\n");
    return -1;
}

static int
i2c_bb_master_stop(i2c_bus_t* bus)
{
    assert(!"Not implemented\n");
    return -1;
}

static int
i2c_bb_start_read(i2c_slave_t *sl,
                  void* vdata, size_t size,
                  UNUSED bool end_with_repeat_start,
                  i2c_callback_fn cb, void* token)
{
    struct i2c_bb* d;
    int nak;
    int count;
    char* data = (char*)vdata;

    assert(sl != NULL && sl->bus != NULL);

    d = i2c_bus_get_priv(sl->bus);

    i2c_bb_start(d);
    nak = i2c_bb_sendbyte(d, sl->address | 1);
    if (nak) {
        i2c_bb_stop(d);
        return -1;
    }

    for (count = 0; !nak && count < size; count++) {
        *data++ = i2c_bb_readbyte(d, count + 1 == size);
    }

    i2c_bb_stop(d);
    if (cb) {
        cb(sl->bus, I2CSTAT_COMPLETE, size, token);
    }
    return count;
}

static int
i2c_bb_start_write(i2c_slave_t *sl,
                   const void* vdata, size_t size,
                   UNUSED bool end_with_repeat_start,
                   i2c_callback_fn cb, void* token)
{
    struct i2c_bb* d;
    int nak;
    int count;
    const char* data = (const char*)vdata;

    assert(sl != NULL && sl->bus != NULL);

    d = i2c_bus_get_priv(sl->bus);

    i2c_bb_start(d);
    nak = i2c_bb_sendbyte(d, sl->address & ~1);
    if (nak) {
        return -1;
    }

    for (count = 0; !nak && count < size; count++) {
        i2c_bb_sendbyte(d, *data++);
    }

    i2c_bb_stop(d);
    if (cb) {
        cb(sl->bus, I2CSTAT_COMPLETE, size, token);
    }
    return count;
}

static void
i2c_bb_handle_irq(i2c_bus_t* i2c_bus)
{
    struct i2c_bb* d;
    /* BB is a blocking call, but in case someone tried to poll, we ignore the call */
    d = i2c_bus_get_priv(i2c_bus);
    (void)d;
}

static int
i2c_bb_set_self_slave_address(i2c_bus_t *i2c_bus, int addr)
{
    struct i2c_bb* d;
    d = i2c_bus_get_priv(i2c_bus);
    (void)d;
    assert(!"Not implemented");
    return -1;
}

static void
i2c_bb_register_slave_event_handler(i2c_bus_t *bus,
                                    i2c_aas_callback_fn aas_cb, void* aas_token)
{
    assert(bus != NULL);
    bus->aas_cb = aas_cb;
    bus->aas_token = aas_token;
}

static const uint32_t i2c_speed_freqs[] = {
    [I2C_SLAVE_SPEED_STANDARD] = 100000,
    [I2C_SLAVE_SPEED_FAST] = 400000,
    [I2C_SLAVE_SPEED_FASTPLUS] = 1000000,
    [I2C_SLAVE_SPEED_HIGHSPEED] = 3400000
};

static long
i2c_bb_set_speed(i2c_bus_t* i2c_bus, enum i2c_slave_speed speed)
{
    struct i2c_bb* d;
    d = i2c_bus_get_priv(i2c_bus);

    if (speed < I2C_SLAVE_SPEED_STANDARD || speed > I2C_SLAVE_SPEED_HIGHSPEED) {
        ZF_LOGE("lib I2C-bitbang: Unsupported speed %d.", speed);
        return -1;
    }

    d->speed = i2c_speed_freqs[speed];
    return speed;
}

int
i2c_bb_slave_init(i2c_bus_t* i2c_bus, int address,
                  enum i2c_slave_address_size address_size,
                  enum i2c_slave_speed max_speed,
                  i2c_slave_t* sl)
{
    assert(sl != NULL);

    if (address_size == I2C_SLAVE_ADDR_7BIT) {
        address = i2c_extract_address(address);
    }

    sl->address = address;
    sl->address_size = address_size;
    sl->max_speed = max_speed;
    sl->i2c_opts = 0;
    sl->bus = i2c_bus;

    sl->slave_read      = &i2c_bb_start_read;
    sl->slave_write     = &i2c_bb_start_write;

    return 0;
}

int
i2c_bb_init(gpio_sys_t* gpio_sys, gpio_id_t scl, gpio_id_t sda,
            struct i2c_bb* i2c_bb, struct i2c_bus* i2c)
{
    /* Initialise the BB structure */
    i2c_bb->scl = scl;
    i2c_bb->sda = sda;
    i2c_bb->speed = DEFAULT_SPEED;
    i2c_bb->gpio_sys = gpio_sys;
    /* Initialise the I2C bus structure */
    i2c->slave_init  = i2c_bb_slave_init;
    i2c->read        = i2c_bb_read;
    i2c->write       = i2c_bb_write;
    i2c->set_speed   = i2c_bb_set_speed;
    i2c->set_self_slave_address = i2c_bb_set_self_slave_address;
    i2c->register_slave_event_handler = i2c_bb_register_slave_event_handler;
    i2c->master_stop = i2c_bb_master_stop;
    i2c->handle_irq  = i2c_bb_handle_irq;
    i2c->priv        = (void*)i2c_bb;
    /* Done */
    return 0;
}

#endif
