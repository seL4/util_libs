/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#include <stdint.h>
#include <platsupport/mux.h>
#include <platsupport/gpio.h>
#include "../../services.h"
#include "mux.h"

#if defined(PLAT_EXYNOS5250)
/* These are for the arndale */
#define EXYNOS_GPIOLEFT_PADDR    0x11400000
#define EXYNOS_GPIORIGHT_PADDR   0x13400000
#define EXYNOS_GPIOC2C_PADDR     0x10D10000
#define EXYNOS_GPIOAUDIO_PADDR   0x03860000
#elif defined(PLAT_EXYNOS5410)
/* These are for Odroid-XU */
#define EXYNOS_GPIOLEFT_PADDR    0x13400000
#define EXYNOS_GPIORIGHT_PADDR   0x14000000
#define EXYNOS_GPIOC2C_PADDR     0x10D10000
#define EXYNOS_GPIOAUDIO_PADDR   0x03860000
#elif defined(PLAT_EXYNOS4)
/* These are for Odroid-X */
#define EXYNOS_GPIOLEFT_PADDR    0x11400000
#define EXYNOS_GPIORIGHT_PADDR   0x11000000
#define EXYNOS_GPIOC2C_PADDR     0x106E0000
#define EXYNOS_GPIOAUDIO_PADDR   0x03860000
#else
#error Unidentified Exynos5 SoC
#endif

#define EXYNOS_GPIOX_SIZE        0x1000
#define EXYNOS_GPIOLEFT_SIZE     EXYNOS_GPIOX_SIZE
#define EXYNOS_GPIORIGHT_SIZE    EXYNOS_GPIOX_SIZE
#define EXYNOS_GPIOC2C_SIZE      EXYNOS_GPIOX_SIZE
#define EXYNOS_GPIOAUDIO_SIZE    EXYNOS_GPIOX_SIZE


#define BITFIELD_SHIFT(x, bits)    ((x) * (bits))
#define BITFIELD_MASK(x, bits)     (MASK(bits) << BITFIELD_SHIFT(x, bits))


static volatile struct mux_bank* _bank[GPIO_NBANKS];

static struct mux_bank**
mux_priv_get_banks(mux_sys_t* mux){
    assert(mux);
    return (struct mux_bank**)mux->priv;
}


static void
exynos_mux_set_con(struct mux_cfg* _cfg, int pin, int func){
    volatile struct mux_cfg* cfg = (volatile struct mux_cfg*)_cfg;
    uint32_t v;
    v = cfg->con;
    v &= ~BITFIELD_MASK(pin, 4);
    v |= func << BITFIELD_SHIFT(pin, 4);
    DMUX("con.%d @ 0%08x : 0x%08x->0x%08x\n", pin, (uint32_t)&cfg->con, cfg->con, v);
    cfg->con = cfg->conpdn = v;
}

static void
exynos_mux_set_dat(struct mux_cfg* _cfg, int pin, int val){
    volatile struct mux_cfg* cfg = (volatile struct mux_cfg*)_cfg;
    uint32_t v;
    v = cfg->dat;
    v &= ~BITFIELD_MASK(pin, 1);
    if(val){
        v |= 1 << BITFIELD_SHIFT(pin, 1);
    }
    DMUX("dat.%d @ 0%08x : 0x%08x->0x%08x\n", pin, (uint32_t)&cfg->dat, cfg->dat, v);
    cfg->dat = v;
}

static int
exynos_mux_get_dat(struct mux_cfg* _cfg, int pin){
    volatile struct mux_cfg* cfg = (volatile struct mux_cfg*)_cfg;
    uint32_t val;
    val = cfg->dat;
    val &= BITFIELD_MASK(pin, 1);
    return !!val;
}

static void
exynos_mux_set_pud(struct mux_cfg* cfg, int pin, int pud){
    uint32_t v;
    v = cfg->pud;
    v &= ~BITFIELD_MASK(pin, 2);
    v |= pud << BITFIELD_SHIFT(pin, 2);
    DMUX("pud.%d @ 0%08x : 0x%08x->0x%08x\n", pin, (uint32_t)&cfg->pud, cfg->pud, v);
    cfg->pud = cfg->pudpdb = v;
}

static void
exynos_mux_set_drv(struct mux_cfg* _cfg, int pin, int _drv){
    volatile struct mux_cfg* cfg = (volatile struct mux_cfg*)_cfg;
    uint32_t v;
    int drv;
    if(_drv < 1){
        _drv = 1;
    }else if(_drv > 4){
        _drv = 4;
    }
    switch(_drv){
    case 1: drv = DRV1X; break;
    case 2: drv = DRV2X; break;
    case 3: drv = DRV3X; break;
    case 4: drv = DRV4X; break;
    default:
        assert(!"Invalid drive strength");
        drv = 0;
    }
    v = cfg->drv;
    v &= BITFIELD_MASK(pin, 2);
    v |= drv << BITFIELD_SHIFT(pin, 2);
    DMUX("drv @ 0%08x : 0x%08x->0x%08x\n", (uint32_t)&cfg->drv, cfg->drv, v);
    cfg->drv = v;
}

static struct mux_cfg*
get_mux_cfg(mux_sys_t* mux, int port){
    struct mux_bank** bank;
    int b, p;
    bank = mux_priv_get_banks(mux);
    b = GPIOPORT_GET_BANK(port);
    p = GPIOPORT_GET_PORT(port);
    assert(b >= 0 && b < GPIO_NBANKS);
    return &bank[b]->gp[p]; 
}

static void
exynos_mux_configure(struct mux_cfg* cfg, int pin, 
                     int con, int pud, int drv){
    exynos_mux_set_pud(cfg, pin, pud);
    exynos_mux_set_drv(cfg, pin, drv);
    exynos_mux_set_con(cfg, pin, con);
}

static int
exynos_mux_feature_enable(mux_sys_t* mux, enum mux_feature mux_feature)
{
    struct mux_feature_data* data = feature_data[mux_feature];
    (void)mux;
    for(; data->port != GPIOPORT_NONE; data++){
        struct mux_cfg*  cfg;
        /* Apply */
        DMUX("Enabling feature: bank %d, port %d, pin %d\n", 
                GPIOPORT_GET_BANK(data->port),
                GPIOPORT_GET_PORT(data->port),
                data->pin);

        cfg = get_mux_cfg(mux, data->port);
        assert(cfg);

        exynos_mux_configure(cfg, data->pin, 
                                MUXVALUE_CON(data->value),
                                MUXVALUE_PUD(data->value),
                                MUXVALUE_DRV(data->value));
    }
    return 0;
}

static int exynos_mux_init_common(mux_sys_t* mux){
    mux->priv = &_bank;
    mux->feature_enable = &exynos_mux_feature_enable;
    return 0;
}

int
exynos_mux_init(void* gpioleft, void* gpioright, void* gpioc2c,
                  void* gpioaudio, mux_sys_t* mux){
    if(gpioleft)  _bank[GPIO_LEFT_BANK ] = gpioleft;
    if(gpioright) _bank[GPIO_RIGHT_BANK] = gpioright;
    if(gpioc2c)   _bank[GPIO_C2C_BANK  ] = gpioc2c;
    if(gpioaudio) _bank[GPIO_AUDIO_BANK] = gpioaudio;
    return exynos_mux_init_common(mux);
}


int
mux_sys_init(ps_io_ops_t* io_ops, mux_sys_t* mux)
{

    MAP_IF_NULL(io_ops, EXYNOS_GPIOLEFT,  _bank[GPIO_LEFT_BANK]);
    MAP_IF_NULL(io_ops, EXYNOS_GPIORIGHT, _bank[GPIO_RIGHT_BANK]);
    MAP_IF_NULL(io_ops, EXYNOS_GPIOC2C,   _bank[GPIO_C2C_BANK]);
    MAP_IF_NULL(io_ops, EXYNOS_GPIOAUDIO, _bank[GPIO_AUDIO_BANK]);
    return exynos_mux_init_common(mux);
}


/****************** GPIO ******************/

static inline mux_sys_t*
gpio_sys_get_mux(const gpio_sys_t* gpio_sys){
    assert(gpio_sys);
    assert(gpio_sys->priv);
    return (mux_sys_t*)gpio_sys->priv;
}

static inline mux_sys_t*
gpio_get_mux(const gpio_t *gpio){
    assert(gpio);
    return gpio_sys_get_mux(gpio->gpio_sys);
}


static struct mux_cfg*
get_gpio_cfg(gpio_t* gpio){
    mux_sys_t* mux;
    assert(gpio);
    mux = gpio_get_mux(gpio);
    return get_mux_cfg(mux, GPIOID_PORT(gpio->id));
}

static int
exynos_gpio_init(gpio_sys_t* gpio_sys, int id, enum gpio_dir dir, gpio_t* gpio){
    struct mux_cfg* cfg;
    int pud, con;
    assert(gpio);

    DGPIO("Configuring GPIO on port %d pin %d\n", port, pin);

    gpio->id = id;
    gpio->gpio_sys = gpio_sys;
    gpio->next = NULL;
    cfg = get_gpio_cfg(gpio);
    if(cfg == NULL){
        return -1;
    }

    con = (dir == GPIO_DIR_OUT)? 1 : 0;
    pud = (dir == GPIO_DIR_IN)? PUD_PULLUP : PUD_NONE;
    exynos_mux_configure(cfg, GPIOID_PIN(id), con, pud, 1);
    return 0; 
}

static int
exynos_gpio_config(gpio_t* gpio, int param_list){
    assert(!"Not implemented");
    (void)gpio;
    (void)param_list;
    return -1;
}

static int
exynos_gpio_write(gpio_t* gpio, const char* data, int len){
    int count;
    for(count = 0; count < len && gpio; count++){
        struct mux_cfg* cfg;
        cfg = get_gpio_cfg(gpio);
        exynos_mux_set_dat(cfg, GPIOID_PIN(gpio->id), *data++);
        gpio = gpio->next;
    }
    return count;
}

static int
exynos_gpio_read(gpio_t* gpio, char* data, int len){
    int count;
    for(count = 0; count < len && gpio; count++){
        struct mux_cfg* cfg;
        cfg = get_gpio_cfg(gpio);
        if(exynos_mux_get_dat(cfg, GPIOID_PIN(gpio->id))){
            *data++ = 0xff;
        }else{
            *data++ = 0x00;
        }
        gpio = gpio->next;
    }
    return count;
}


int
exynos_gpio_sys_init(mux_sys_t* mux_sys, gpio_sys_t* gpio_sys){
    assert(gpio_sys);
    assert(mux_sys);
    if(!mux_sys_valid(mux_sys)){
        return -1;
    }else{
        /* GPIO is done through the MUX on exynos */
        gpio_sys->priv = mux_sys;
        gpio_sys->config = &exynos_gpio_config;
        gpio_sys->read = &exynos_gpio_read;
        gpio_sys->write = &exynos_gpio_write;
        gpio_sys->init = &exynos_gpio_init;
        return 0;
    }
}


int
gpio_sys_init(ps_io_ops_t* io_ops, gpio_sys_t* gpio_sys){
    assert(gpio_sys);
    assert(io_ops);
    return exynos_gpio_sys_init(&io_ops->mux_sys, gpio_sys);
}


