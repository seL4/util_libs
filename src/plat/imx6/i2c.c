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
#include <platsupport/clock.h>
#include "../../services.h"
#include "../../clock.h"

#include <string.h>

//#define I2C_DEBUG
#ifdef I2C_DEBUG
#define dprintf(...) printf("I2C: " __VA_ARGS__)
#else
#define dprintf(...) do{}while(0)
#endif

#define IMX6_I2C_DEFAULT_FREQ (20 * KHZ)

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




struct i2c_bus {
    volatile struct imx6_i2c_regs* regs;
    enum mux_feature mux;
    enum clock_gate clk_gate;
    struct clock clock;
};

static inline void
i2c_dump_regs(i2c_bus_t* dev)
{
    printf("address 0x%04x\n", dev->regs->address);
    printf("divider 0x%04x\n", dev->regs->div);
    printf("control 0x%04x\n", dev->regs->control);
    printf("status  0x%04x\n", dev->regs->status);
}

/********************
 *** I2C clocking ***
 ********************/


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
_i2c_prescale_decode(int div){
    int error = 0xffff;
    int match;
    int i;
    /* By default, just choose the lowest frequency */
    match = 0;
    /* Scan for a prescaler which minimizes error */
    for(i = 0; i < sizeof(_i2c_div_map)/sizeof(*_i2c_div_map);i++){
        int this_error;
        this_error = _i2c_div_map[i] - div;
        /* Absolute error value */
        if(this_error < 0){
            this_error *= -1;
        }
        /* Update prescale value, early exit if exact match */
        if(this_error == 0){
            return i;
        }else if(this_error < error){
            error = this_error;
            match = i;
        }
    }
    return match;
}

static clk_t*
_i2c_clk_init(clk_t* clk)
{
    i2c_bus_t* i2c = (i2c_bus_t*)clk->priv;
    assert(i2c != NULL);
    if(clk->parent == NULL){
        clk_t* parent = clk_get_clock(clk->clk_sys, CLK_PERCLK);
        assert(parent != NULL);
        clk_register_child(parent, clk);
    }
    clk_set_freq(clk, IMX6_I2C_DEFAULT_FREQ);
    clk_gate_enable(clk->clk_sys, i2c->clk_gate, CLKGATE_ON);
    return clk;
}


static freq_t
_i2c_clk_get_freq(clk_t* clk)
{
    freq_t fin = clk_get_freq(clk->parent);
    i2c_bus_t* i2c = (i2c_bus_t*)clk->priv;
    int div = _i2c_div_map[i2c->regs->div];
    return fin/div;
}

static freq_t
_i2c_clk_set_freq(clk_t* clk, freq_t hz)
{
    freq_t fin = clk_get_freq(clk->parent);
    i2c_bus_t* i2c = (i2c_bus_t*)clk->priv;
    uint32_t div = fin / hz;
    assert((div > 22 && div <= 3840) || !"Parent calibration not implemented");
    i2c->regs->div = _i2c_prescale_decode(div);
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

static i2c_bus_t _i2c[NI2C] = {
    { 
        .regs = NULL, 
        .mux = MUX_I2C1, 
        .clk_gate = i2c1_serial,
        .clock = {
            .id = CLK_CUSTOM,
            CLK_OPS(i2c_clk),
            .name = "I2C1 clk",
            .priv = (void*)&_i2c[0],
        }
    },
    { 
        .regs = NULL, 
        .mux = MUX_I2C2, 
        .clk_gate = i2c2_serial,
        .clock = {
            .id = CLK_CUSTOM,
            CLK_OPS(i2c_clk),
            .name = "I2C2 clk",
            .priv = (void*)&_i2c[1],
        }
    },
    { 
        .regs = NULL, 
        .mux = MUX_I2C3, 
        .clk_gate = i2c3_serial,
        .clock = {
            .id = CLK_CUSTOM,
            CLK_OPS(i2c_clk),
            .name = "I2C3 clk",
            .priv = (void*)&_i2c[2],
        }
    }
};

/******************
 **** I2C Core ****
 ******************/
static inline int 
busy(i2c_bus_t* dev){
    return !!(dev->regs->status & I2CSTAT_BUSY);
}

static inline int 
irq_pending(i2c_bus_t* dev){
    return !!(dev->regs->status & I2CSTAT_IRQ_PEND);
}

static inline void
clear_pending(i2c_bus_t* dev){
    dev->regs->status &= ~(I2CSTAT_IRQ_PEND);
}

static inline void
_master_stop(i2c_bus_t* dev){
    /* Send stop signal */
    dev->regs->control &= ~I2CCON_MASTER;
    /* Wait for idle bus */
    while(busy(dev));
    /* Disable the bus */
    dev->regs->control &= ~(I2CCON_MASTER | I2CCON_TXEN      |
                            I2CCON_ENABLE | I2CCON_IRQ_ENABLE);
}


static int
_do_write(i2c_bus_t* dev, const void* vdata, int len){
    int count;
    const char* data = (const char*)vdata;
    for(count = 0; count < len; count++){
        dev->regs->data = *data++;
        while(!irq_pending(dev));
        clear_pending(dev);
        /* Check ACK */
        if(dev->regs->status & I2CSTAT_NAK){
            return count;
        }
    }
    return count;
}

int
i2c_mwrite(i2c_bus_t* dev, int slave, const void* vdata, int len)
{
    int count;
    char addr;
    dprintf("Writing %d bytes to slave@0x%02x\n", len, slave);

    /* Enable the bus */
    dev->regs->control |= I2CCON_ENABLE | I2CCON_IRQ_ENABLE;
    while(busy(dev));
    /* Enter master TX mode */
    dev->regs->control |= I2CCON_MASTER | I2CCON_TXEN;
    while(!busy(dev));
    /* Write slave address */
    addr = I2CDATA_WRITE(slave);
    count = _do_write(dev, &addr, 1);
    if(count == 1){
        /* Pump out data */
        count = _do_write(dev, vdata, len);
    }else{
        dprintf("NACK from slave@0x%02x\n", slave);
        count = -1;
    }
    /* Send stop signal */
    _master_stop(dev);
    return count;
}



int
i2c_mread(i2c_bus_t* dev, int slave, void* vdata, int len)
{
    int count;
    char addr;
    uint32_t c;
    char* data = (char*)vdata;
    int ret;

    dprintf("Reading %d bytes from slave@0x%02x\n", len, slave);
    /* Enable the bus */
    dev->regs->control |= I2CCON_ENABLE | I2CCON_IRQ_ENABLE;
    while(busy(dev));
    /* For master RX mode, we enable TX for writing the address, then disable */
    dev->regs->control |= I2CCON_MASTER | I2CCON_TXEN;
    while(!busy(dev));
    clear_pending(dev);
    /* Write slave address */
    addr = I2CDATA_READ(slave);
    ret = _do_write(dev, &addr, 1);
    if(ret == 1){
        count = 0;
        /* Leave TX mode */
        dev->regs->control &= ~(I2CCON_TXEN);
        /* Read bytes */
        /* This dummy read bootstraps RX transfer */
        clear_pending(dev);
        c = dev->regs->data;
        while(!irq_pending(dev));
        clear_pending(dev);

        while(count < len){
            /* Disable ACK before reading last byte to stop transfer */
            if(count == len - 1){
                dev->regs->control |= I2CCON_ACK_EN;
            }
            /* read the byte */
            c = dev->regs->data;
            while(!irq_pending(dev));
            clear_pending(dev);
            *data++ = c;
            count++;
        }
    }else{
        count = -1;
    }
    /* Stop signal */
    _master_stop(dev);
    return count;
}

int
i2c_set_address(i2c_bus_t* i2c_bus, int addr){
    i2c_bus->regs->address = addr & 0xfe;
    return 0;
}

int
i2c_init(enum i2c_id id, ps_io_ops_t* io_ops, i2c_bus_t** i2c)

{
    i2c_bus_t* dev = _i2c + id;
    int err;
    clk_t* i2c_clk;
    /* Map memory */
    dprintf("Mapping i2c %d\n", id);
    switch(id){
    case I2C1: MAP_IF_NULL(io_ops, IMX6_I2C1, dev->regs); break;
    case I2C2: MAP_IF_NULL(io_ops, IMX6_I2C2, dev->regs); break;
    case I2C3: MAP_IF_NULL(io_ops, IMX6_I2C3, dev->regs); break;
    default :
        return -1;
    }
    /* Check that our memory was mapped */
    if(dev->regs == NULL){
        return -2;
    }

    /* Configure MUX */
    err = mux_feature_enable(&io_ops->mux_sys, dev->mux);
    if(err){
        assert(!"Failed to configure I2C mux");
        return -1;
    }

    /* Init clock */
    dev->clock.clk_sys = &io_ops->clock_sys;
    i2c_clk = clk_init(&dev->clock);
    if(i2c_clk == NULL){
        assert(!"Failed to initialise I2C clock");
        return -1;
    }
    /* I2C setup */
    dev->regs->control &= ~(I2CCON_ENABLE | I2CCON_TXEN);
    dev->regs->address = 0x00;
    dev->regs->control = I2CCON_ACK_EN;

    *i2c = dev;
    return 0;
}
