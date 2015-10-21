/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <platsupport/mach/tmu.h>
#include "../../services.h"

#include <string.h>

#define TRIMINFO_RELOAD        (1)

#define CORE_EN                (1)
#define TRIP_EN                (1<<12)
#define TRIP_ONLYCURRENT       (0<<13)
#define TRIP_CUR_PAST3_0       (4<<13)
#define TRIP_CUR_PAST7_0       (5<<13)
#define TRIP_CUR_PAST11_0      (6<<13)
#define TRIP_CUR_PAST15_0      (7<<13)

#define INTEN_RISE0            (1)
#define INTEN_RISE1            (1<<4)
#define INTEN_RISE2            (1<<8)
#define INTEN_FALL0            (1<<16)
#define INTEN_FALL1            (1<<20)
#define INTEN_FALL2            (1<<24)

#define INT_RISE(x)            (1 << ((x) * 4))
#define INT_FALL(x)            INT_RISE(3 + (x))

#define TRIM_INFO_MASK         (0xFF)

#define INT_ALL                ( INT_RISE(0) | INT_RISE(1) | INT_RISE(2) \
                               | INT_FALL(0) | INT_FALL(1) | INT_FALL(2) )

#define EMUL_EN                (1)

/* EFUSE related definition. */
#define EFUSE_MIN_VALUE        40
#define EFUSE_MAX_VALUE        100
#define EFUSE_INIT_VALUE       55


#define TMU_SAVE_NUM           10
#define TMU_DC_OFFSET          25

/* Miscellaneous definitions. */
#define SLOPE                  0x10008802
#define MUX_ADDR_VALUE         6

/* Device access macros. */
#define TMU_REG(vbase, offset)    (*(volatile unsigned int *)(vbase + offset))


struct tmu_regs {
    uint32_t res0[5];           /* 0x00 */
    uint32_t triminfo_con;      /* 0x14 */
    uint32_t res1[2];           /* 0x18 */
    uint32_t con;               /* 0x20 */
    uint32_t res2[1];           /* 0x24 */
    uint32_t stat;              /* 0x28 */
    uint32_t sampling_interval; /* 0x2C */
    uint32_t cnt0;              /* 0x30 */
    uint32_t cnt1;              /* 0x34 */
    uint32_t res3[2];           /* 0x38 */
    uint32_t temperature;       /* 0x40 */
    uint32_t res4[3];           /* 0x44 */
    uint32_t threshold_rise;    /* 0x50 */
    uint32_t threshold_fall;    /* 0x54 */
    uint32_t res5[2];           /* 0x58 */
    uint32_t past_temp[4];      /* 0x60 */
    uint32_t int_enable;        /* 0x70 */
    uint32_t int_stat;          /* 0x74 */
    uint32_t int_clear;         /* 0x78 */
    uint32_t res6[1];           /* 0x7C */
    uint32_t emul_con;          /* 0x80 */
};
typedef volatile struct tmu_regs tmu_regs_t;

static tmu_regs_t* _tmu_regs[NTMU];

static inline tmu_regs_t*
tmu_priv_get_regs(tmu_t* tmu)
{
    return (tmu_regs_t*)tmu->priv;
}


static int
do_exynos_tmu_init(enum tmu_id id, void* vaddr, tmu_t* tmu)
{
    tmu_regs_t* regs;
    uint32_t v;
    uint32_t te1, te2;

    memset(tmu, 0, sizeof(*tmu));

    /* Check bounds */
    if (id < 0 || id >= NTMU) {
        return -1;
    }
    /* Initialise memory map */
    if (vaddr) {
        _tmu_regs[id] = vaddr;
    }
    if (_tmu_regs[id] == NULL) {
        return -1;
    }

    regs = _tmu_regs[id];
    tmu->priv = (void*)_tmu_regs[id];

    /* Reset alarms */
    regs->int_enable = 0;
    regs->int_clear = INT_ALL;
    regs->threshold_rise = 0;
    regs->threshold_fall = 0;

    /* Reload TRIMINFO_CON for using efuse. */
    regs->triminfo_con = TRIMINFO_RELOAD;
    while (regs->triminfo_con & TRIMINFO_RELOAD);
    /* Get the compensation parameter. */
    v = regs->triminfo_con;
    te1 = (v >> 0) & TRIM_INFO_MASK;
    te2 = (v >> 8) & TRIM_INFO_MASK;

    /* Ensure the parameters are in range */
    if ((EFUSE_MIN_VALUE > te1) || (te1 > EFUSE_MAX_VALUE) || (te2 != 0)) {
        te1 = EFUSE_INIT_VALUE;
    }
    tmu->t_off = te1 - TMU_DC_OFFSET;

    /* Need to initialize register setting after getting parameter info. */
    /* [28:23] vref [11:8] slope - Tunning parameter */
    regs->con = SLOPE;

    /* Enable the TMU */
    regs->con |= (MUX_ADDR_VALUE << 20) | CORE_EN;
    while (regs->temperature == 0);

    /* Clear the reading */
    v = regs->temperature;
    return 0;
}

int
exynos4_tmu_init(enum tmu_id id, void* vaddr, tmu_t* tmu)
{
    return do_exynos_tmu_init(id, vaddr, tmu);
}

int
exynos5_tmu_init(enum tmu_id id, void* vaddr, tmu_t* tmu)
{
    return do_exynos_tmu_init(id, vaddr, tmu);
}

int
exynos_tmu_init(enum tmu_id id, ps_io_ops_t* io_ops, tmu_t* tmu)
{
    /* Check bounds */
    if (id < 0 || id >= NTMU) {
        return -1;
    }
    /* Map the memory */
    MAP_IF_NULL(io_ops, EXYNOS_TMU, _tmu_regs[id]);
    if (_tmu_regs[id] == NULL) {
        return -1;
    }
    return do_exynos_tmu_init(id, (void*)_tmu_regs[id], tmu);
}

int
exynos_tmu_get_temperature(tmu_t* tmu)
{
    tmu_regs_t* regs;
    uint32_t currTemp;
    temperature_t temperature;
    regs = tmu_priv_get_regs(tmu);

    /* After reading temperature code from register, compensating
     * its value and calculating celsius temperature,
     * get current temperature.
     */
    currTemp = regs->temperature & 0xff;

    /* compensate and calculate current temperature */
    temperature = currTemp - tmu->t_off;
    if (temperature < 0) {
        /* temperature code range are between min 25 and 125 */
        LOG_ERROR("Invalid temperature from TMU");
    }

    return temperature;
}

void
exynos_tmu_handle_irq(tmu_t* tmu)
{
    tmu_regs_t* regs;
    uint32_t sts;
    temperature_t t = exynos_tmu_get_temperature(tmu);
    regs = tmu_priv_get_regs(tmu);

    sts = regs->int_stat;
    if (sts & INT_RISE(0)) {
        assert(tmu->rising_alarm);
        tmu->rising_alarm(tmu, t, 0, 1, tmu->rising_token);
    }
    if (sts & INT_RISE(1)) {
        assert(tmu->rising_alarm);
        tmu->rising_alarm(tmu, t, 1, 1, tmu->rising_token);
    }
    if (sts & INT_RISE(2)) {
        assert(tmu->rising_alarm);
        tmu->rising_alarm(tmu, t, 2, 1, tmu->rising_token);
    }
    if (sts & INT_FALL(0)) {
        assert(tmu->falling_alarm);
        tmu->rising_alarm(tmu, t, 0, 0, tmu->falling_token);
    }
    if (sts & INT_FALL(1)) {
        assert(tmu->falling_alarm);
        tmu->rising_alarm(tmu, t, 1, 0, tmu->falling_token);
    }
    if (sts & INT_FALL(2)) {
        assert(tmu->falling_alarm);
        tmu->rising_alarm(tmu, t, 2, 0, tmu->falling_token);
    }
    regs->int_clear = sts;
}

int
exynos_tmu_set_alarms_rising(tmu_t* tmu,
                             temperature_t level0,
                             temperature_t level1,
                             temperature_t level2,
                             tmu_alarm_callback cb,
                             void* token)
{
    tmu_regs_t* regs;
    uint32_t threshold;
    uint32_t int_enable;

    regs = tmu_priv_get_regs(tmu);

    threshold = 0;
    int_enable = regs->int_enable & ~(INT_RISE(0) | INT_RISE(1) | INT_RISE(2));
    if (level0 >= 0) {
        threshold |= (level0 + tmu->t_off) <<  0;
        int_enable |= INT_RISE(0);
    }
    if (level1 >= 0) {
        threshold |= (level0 + tmu->t_off) <<  0;
        int_enable |= INT_RISE(1);
    }
    if (level2 >= 0) {
        threshold |= (level0 + tmu->t_off) <<  0;
        int_enable |= INT_RISE(2);
    }
    regs->threshold_rise = threshold;
    regs->int_enable = int_enable;
    return 0;
}

int
exynos_tmu_set_alarms_falling(tmu_t* tmu,
                              temperature_t level0,
                              temperature_t level1,
                              temperature_t level2,
                              tmu_alarm_callback cb,
                              void* token)
{
    tmu_regs_t* regs;
    uint32_t threshold;
    uint32_t int_enable;

    regs = tmu_priv_get_regs(tmu);

    threshold = 0;
    int_enable = regs->int_enable & ~(INT_FALL(0) | INT_FALL(1) | INT_FALL(2));
    if (level0 >= 0) {
        threshold |= (level0 + tmu->t_off) <<  0;
        int_enable |= INT_FALL(0);
    }
    if (level1 >= 0) {
        threshold |= (level0 + tmu->t_off) <<  0;
        int_enable |= INT_FALL(1);
    }
    if (level2 >= 0) {
        threshold |= (level0 + tmu->t_off) <<  0;
        int_enable |= INT_FALL(2);
    }
    regs->threshold_fall = threshold;
    regs->int_enable = int_enable;
    return 0;
}



