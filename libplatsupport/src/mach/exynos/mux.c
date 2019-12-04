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
#include <platsupport/gen_config.h>
#include <stdint.h>
#include <platsupport/mux.h>
#include <platsupport/gpio.h>
#include <utils/util.h>
#include "../../services.h"
#include "mux.h"

#define BITFIELD_SHIFT(x, bits)    ((x) * (bits))
#define BITFIELD_MASK(x, bits)     (MASK(bits) << BITFIELD_SHIFT(x, bits))

#define EINTCON_LOW  0x0
#define EINTCON_HIGH 0x1
#define EINTCON_FALL 0x2
#define EINTCON_RISE 0x3
#define EINTCON_EDGE 0x4
#define EINTCON_MASK 0x7
#define EINTCON_BITS 4

#define PORTS_PER_BANK 56
#define GPX_IDX_OFFSET 96

static volatile struct mux_bank* _bank[GPIO_NBANKS];

static struct mux_bank**
mux_priv_get_banks(const mux_sys_t* mux) {
    assert(mux);
    return (struct mux_bank**)mux->priv;
}

static struct mux_cfg*
get_mux_cfg(const mux_sys_t* mux, int port) {
    struct mux_bank** bank;
    int b, p;
    bank = mux_priv_get_banks(mux);
    b = GPIOPORT_GET_BANK(port);
    p = GPIOPORT_GET_PORT(port);
    assert(b >= 0 && b < GPIO_NBANKS);
    return &bank[b]->gp[p];
}

static void
exynos_mux_set_con(struct mux_cfg* _cfg, int pin, int func)
{
    volatile struct mux_cfg* cfg = (volatile struct mux_cfg*)_cfg;
    uint32_t v;
    v = cfg->con;
    v &= ~BITFIELD_MASK(pin, 4);
    v |= func << BITFIELD_SHIFT(pin, 4);
    ZF_LOGD("con.%d @ 0x%08x : 0x%08x->0x%08x\n", pin, (uint32_t)&cfg->con, cfg->con, v);
    cfg->con = v;
    cfg->conpdn |= 0x3 << BITFIELD_SHIFT(pin, 2);
}

static void
exynos_mux_set_dat(struct mux_cfg* _cfg, int pin, int val)
{
    volatile struct mux_cfg* cfg = (volatile struct mux_cfg*)_cfg;
    uint32_t v;
    v = cfg->dat;
    v &= ~BITFIELD_MASK(pin, 1);
    if (val) {
        v |= BIT(BITFIELD_SHIFT(pin, 1));
    }
    ZF_LOGD("dat.%d @ 0x%08x : 0x%08x->0x%08x\n", pin, (uint32_t)&cfg->dat, cfg->dat, v);
    cfg->dat = v;
}

static int
exynos_mux_get_dat(struct mux_cfg* _cfg, int pin)
{
    volatile struct mux_cfg* cfg = (volatile struct mux_cfg*)_cfg;
    uint32_t val;
    val = cfg->dat;
    val &= BITFIELD_MASK(pin, 1);
    return !!val;
}

static void
exynos_mux_set_pud(struct mux_cfg* cfg, int pin, int pud)
{
    uint32_t v;
    v = cfg->pud;
    v &= ~BITFIELD_MASK(pin, 2);
    v |= pud << BITFIELD_SHIFT(pin, 2);
    ZF_LOGD("pud.%d @ 0x%08x : 0x%08x->0x%08x\n", pin, (uint32_t)&cfg->pud, cfg->pud, v);
    cfg->pud = v;
    cfg->pudpdn = v;
}

static void
exynos_mux_set_drv(struct mux_cfg* _cfg, int pin, int _drv)
{
    volatile struct mux_cfg* cfg = (volatile struct mux_cfg*)_cfg;
    uint32_t v;
    int drv;
    if (_drv < 1) {
        _drv = 1;
    } else if (_drv > 4) {
        _drv = 4;
    }
    switch (_drv) {
    case 1:
        drv = DRV1X;
        break;
    case 2:
        drv = DRV2X;
        break;
    case 3:
        drv = DRV3X;
        break;
    case 4:
        drv = DRV4X;
        break;
    default:
        assert(!"Invalid drive strength");
        drv = 0;
    }
    v = cfg->drv;
    v &= BITFIELD_MASK(pin, 2);
    v |= drv << BITFIELD_SHIFT(pin, 2);
    ZF_LOGD("drv @ 0x%08x : 0x%08x->0x%08x\n", (uint32_t)&cfg->drv, cfg->drv, v);
    cfg->drv = v;
}

static void
exynos_mux_configure(struct mux_cfg* cfg, int pin,
                     int con, int pud, int drv)
{
    exynos_mux_set_pud(cfg, pin, pud);
    exynos_mux_set_drv(cfg, pin, drv);
    exynos_mux_set_con(cfg, pin, con);
}

static int
exynos_mux_feature_enable(const mux_sys_t* mux, enum mux_feature mux_feature,
                          enum mux_gpio_dir mgd UNUSED)
{
    struct mux_feature_data* data = feature_data[mux_feature];
    (void)mux;
    for (; data->port != GPIOPORT_NONE; data++) {
        struct mux_cfg*  cfg;
        /* Apply */
        ZF_LOGD("Enabling feature: bank %d, port %d, pin %d\n",
             GPIOPORT_GET_BANK(data->port),
             GPIOPORT_GET_PORT(data->port),
             data->pin);

        cfg = get_mux_cfg(mux, data->port);
        assert(cfg);

        exynos_mux_configure(cfg, data->pin,
                             MUXVALUE_CON(data->value),
                             MUXVALUE_PUD(data->value),
                             MUXVALUE_DRV(data->value));
        ZF_LOGD("con.%d @ 0x%08x : 0x%08x (conpdn:0x%08x)\n", data->pin, (uint32_t)&cfg->con, cfg->con, cfg->conpdn);

    }
    return 0;
}

static int exynos_mux_init_common(mux_sys_t* mux)
{
    mux->priv = &_bank;
    mux->feature_enable = &exynos_mux_feature_enable;
    return 0;
}

int
exynos_mux_init(void* gpioleft, void* gpioright, void* gpioc2c,
                void* gpioaudio, mux_sys_t* mux)
{
    if (gpioleft) {
        _bank[GPIO_LEFT_BANK ] = gpioleft;
    }
    if (gpioright) {
        _bank[GPIO_RIGHT_BANK] = gpioright;
    }
    if (gpioc2c) {
        _bank[GPIO_C2C_BANK  ] = gpioc2c;
    }
    if (gpioaudio) {
        _bank[GPIO_AUDIO_BANK] = gpioaudio;
    }
    return exynos_mux_init_common(mux);
}

int
mux_sys_init(const ps_io_ops_t* io_ops, void *dependencies UNUSED,
             mux_sys_t* mux)
{

    MAP_IF_NULL(io_ops, EXYNOS_GPIOLEFT,  _bank[GPIO_LEFT_BANK]);
    MAP_IF_NULL(io_ops, EXYNOS_GPIORIGHT, _bank[GPIO_RIGHT_BANK]);
    MAP_IF_NULL(io_ops, EXYNOS_GPIOC2C,   _bank[GPIO_C2C_BANK]);
    MAP_IF_NULL(io_ops, EXYNOS_GPIOAUDIO, _bank[GPIO_AUDIO_BANK]);
    return exynos_mux_init_common(mux);
}

/****************** GPIO ******************/

static inline mux_sys_t*
gpio_sys_get_mux(const gpio_sys_t* gpio_sys)
{
    assert(gpio_sys);
    assert(gpio_sys->priv);
    return (mux_sys_t*)gpio_sys->priv;
}

static inline mux_sys_t*
gpio_get_mux(const gpio_t *gpio)
{
    assert(gpio);
    return gpio_sys_get_mux(gpio->gpio_sys);
}

static struct mux_cfg*
get_gpio_cfg(gpio_t* gpio) {
    mux_sys_t* mux;
    assert(gpio);
    mux = gpio_get_mux(gpio);
    return get_mux_cfg(mux, GPIOID_PORT(gpio->id));
}

static struct mux_bank*
gpio_get_bank(gpio_t* gpio) {
    struct mux_bank **banks;
    mux_sys_t* mux;
    int portid, bank;

    portid = GPIOID_PORT(gpio->id);
    bank = GPIOPORT_GET_BANK(portid);

    assert(gpio);
    mux = gpio_get_mux(gpio);
    assert(mux);
    banks = mux_priv_get_banks(mux);
    assert(banks);

    return banks[bank];
}

static int
gpio_is_gpx(gpio_t *gpio)
{
    int portid;
    portid = GPIOID_PORT(gpio->id);
    return (portid >= GPX0 && portid <= GPX3);
}

static int
gpio_get_xextint_idx(gpio_t *gpio)
{
    if (!gpio_is_gpx(gpio)) {
        return -1;
    } else {
        int portid, port;
        portid = GPIOID_PORT(gpio->id);
        port = GPIOPORT_GET_PORT(portid);
        return port - GPX_IDX_OFFSET;
    }
}

static int
gpio_get_extint_idx(gpio_t *gpio)
{
    int portid, port;
    portid = GPIOID_PORT(gpio->id);
    port = GPIOPORT_GET_PORT(portid);

    /* Special cases. */
    if (portid == GPV2 || portid == GPV3) {
        return port - 1;
    } else if (portid == GPV4) {
        return port - 2;
#ifdef CONFIG_PLAT_EXYNOS5
        /* GPC4 on EXYNOS5 is very special indeed. */
    } else if (portid == GPC4) {
        return 13;
#endif
        /* General case */
    } else if (port >= 0 && port <= PORTS_PER_BANK) {
        return port;
        /* All other cases, including GPX range */
    } else {
        return -1;
    }
}

static int
gpio_dir_get_intcon(enum gpio_dir dir)
{
    switch (dir) {
    case GPIO_DIR_IRQ_LOW:
        return 0x0;
    case GPIO_DIR_IRQ_HIGH:
        return 0x1;
    case GPIO_DIR_IRQ_FALL:
        return 0x2;
    case GPIO_DIR_IRQ_RISE:
        return 0x3;
    case GPIO_DIR_IRQ_EDGE:
        return 0x4;
    default:
        return -1;
    }
}

static int
exynos_pending_status(gpio_t* gpio, int clear)
{
    volatile struct mux_bank* bank;
    uint32_t pend;
    int pin;

    bank = gpio_get_bank(gpio);
    assert(bank);

    pin = GPIOID_PIN(gpio->id);
    if (gpio_is_gpx(gpio)) {
        int idx;
        /* You HAD to be different GPX... */
        idx = gpio_get_xextint_idx(gpio);
        if (idx < 0) {
            return -1;
        }
        pend = (bank->ext_xint_pend[idx] & ~bank->ext_xint_mask[idx]) & BIT(pin);
        if (clear) {
            bank->ext_xint_pend[idx] = BIT(pin);
        }
    } else {
        int idx;
        idx = gpio_get_extint_idx(gpio);
        if (idx < 0) {
            return -1;
        }
        pend = (bank->ext_int_pend[idx] & ~bank->ext_int_mask[idx]) & BIT(pin);
        if (clear) {
            bank->ext_int_pend[idx] = BIT(pin);
        }
    }
    return pend;
}

static int
exynos_gpio_int_configure(gpio_t *gpio, int int_con)
{
    volatile struct mux_bank* bank;
    int pin;

    /* Configure the int */
    bank = gpio_get_bank(gpio);
    assert(bank);

    pin = GPIOID_PIN(gpio->id);
    if (gpio_is_gpx(gpio)) {
        /* You HAD to be different GPX... */
        uint32_t v;
        int idx;
        idx = gpio_get_xextint_idx(gpio);
        if (idx < 0) {
            return -1;
        }
        v = bank->ext_xint_con[idx];
        v &= BITFIELD_MASK(pin, 4);
        v |= int_con << BITFIELD_SHIFT(pin, 4);
        bank->ext_xint_con[idx] = v;
        bank->ext_xint_mask[idx] &= ~BIT(pin);
        bank->ext_xint_pend[idx] = BIT(pin); /* Set to clear */
        bank->ext_xint_fltcon[idx][idx & 0x1] = 0;
    } else {
        uint32_t v;
        int idx;
        idx = gpio_get_extint_idx(gpio);
        if (idx < 0) {
            return -1;
        }
        v = bank->ext_int_con[idx];
        v &= BITFIELD_MASK(pin, 4);
        v |= int_con << BITFIELD_SHIFT(pin, 4);
        bank->ext_int_con[idx] = v;
        bank->ext_int_mask[idx] &= ~BIT(pin);
        bank->ext_int_pend[idx] = BIT(pin); /* Set to clear */
        /* These features are not supported yet */
        bank->ext_int_fltcon[idx][idx & 0x1] = 0;
        bank->ext_int_grppri_xa = 0;
        bank->ext_int_priority_xa = 0;
        bank->ext_int_service_xa = 0;
        bank->ext_int_service_pend_xa = 0;
        bank->ext_int_grpfixpri_xa = 0;
        bank->ext_int_fixpri[idx] = 0;
    }
    return 0;
}

static int
exynos_gpio_init(gpio_sys_t* gpio_sys, int id, enum gpio_dir dir, gpio_t* gpio)
{
    struct mux_cfg* cfg;
    assert(gpio);

    ZF_LOGD("Configuring GPIO on port %d pin %d\n", GPIOID_PORT(id), GPIOID_PIN(id));

    gpio->id = id;
    gpio->gpio_sys = gpio_sys;
    gpio->next = NULL;
    cfg = get_gpio_cfg(gpio);
    if (cfg == NULL) {
        return -1;
    }

    if (dir == GPIO_DIR_IN) {
        exynos_mux_configure(cfg, GPIOID_PIN(id), 0x0, PUD_PULLUP, 1);
    } else if (dir == GPIO_DIR_OUT) {
        exynos_mux_configure(cfg, GPIOID_PIN(id), 0x1, PUD_NONE, 1);
    } else {
        int con;
        con = gpio_dir_get_intcon(dir);
        if (con < 0) {
            return -1;
        }
        exynos_mux_configure(cfg, GPIOID_PIN(id), 0xf, PUD_PULLUP, 1);
        return exynos_gpio_int_configure(gpio, con);
    }
    return 0;
}

static int
exynos_gpio_write(gpio_t* gpio, const char* data, int len)
{
    int count;
    for (count = 0; count < len && gpio; count++) {
        struct mux_cfg* cfg;
        cfg = get_gpio_cfg(gpio);
        exynos_mux_set_dat(cfg, GPIOID_PIN(gpio->id), *data++);
        gpio = gpio->next;
    }
    return count;
}

static int
exynos_gpio_read(gpio_t* gpio, char* data, int len)
{
    int count;
    for (count = 0; count < len && gpio; count++) {
        struct mux_cfg* cfg;
        cfg = get_gpio_cfg(gpio);
        if (exynos_mux_get_dat(cfg, GPIOID_PIN(gpio->id))) {
            *data++ = 0xff;
        } else {
            *data++ = 0x00;
        }
        gpio = gpio->next;
    }
    return count;
}

int
exynos_gpio_sys_init(mux_sys_t* mux_sys, gpio_sys_t* gpio_sys)
{
    assert(gpio_sys);
    assert(mux_sys);
    if (!mux_sys_valid(mux_sys)) {
        return -1;
    } else {
        /* GPIO is done through the MUX on exynos */
        gpio_sys->priv = mux_sys;
        gpio_sys->read = &exynos_gpio_read;
        gpio_sys->write = &exynos_gpio_write;
        gpio_sys->pending_status = &exynos_pending_status;
        gpio_sys->init = &exynos_gpio_init;
        return 0;
    }
}

int
gpio_sys_init(ps_io_ops_t* io_ops, gpio_sys_t* gpio_sys)
{
    assert(gpio_sys);
    assert(io_ops);
    return exynos_gpio_sys_init(&io_ops->mux_sys, gpio_sys);
}
