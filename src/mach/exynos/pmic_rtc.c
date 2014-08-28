/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */


#include <platsupport/mach/pmic_rtc.h>
#include "../../services.h"

#define PMICRTC_DEBUG
#ifdef PMICRTC_DEBUG
#define dprintf(...) printf("PMIC RTC:" __VA_ARGS__)
#else
#define dprintf(...) do{}while(0)
#endif

#define RTCREG_INTSTAT    0x00
#define RTCREG_INTMASK    0x01
#define RTCREG_CTRLWMASK  0x02
#define RTCREG_CTRL       0x03
#define RTCREG_UPDATE     0x04
#define RTCREG_WATCHDOG   0x06

#define RTCREG_TIME       0x07
#define RTCREG_ALARM1     0x0E
#define RTCREG_ALARM(id)  (RTCREG_ALARM1 + (id) * sizeof(struct rtc_time))

#define RTC_NALARMS      2

/* RTCREG_INTSTAT, RTCREG_INTMASK */
#define RTCINT(x)         ((x) & 0x3f)
#define RTCINT_READY      (1U << 4)
/* RTCREG_CTRLWMASK, RTCREG_CTRL */
#define RTCCTRL_24HOUR    (1U << 1)
#define RTCCTRL_BCD       (1U << 0)
/* RTCREG_UPDATE */
#define RTCUPDATE_READ    (1U << 4)
#define RTCUPDATE_WRITE   (1U << 0)
/* RTCREG_WATCHDOG */
#define RTCWD_SMPL_EN     (1U << 7)
#define RTCWD_WDT_EN      (1U << 6)
#define RTCWD_SMPL_CFG(x) ((x) << 2)
#define RTCWD_WDT_CFG(x)  ((x) << 0)

/* We can set a 24 hour value for the time, but the RTC always gives us back
 * flag for AM/PM */
#define RTC_HOUR_PM       (1U << 6)

static int
id_valid(pmic_rtc_t* dev, int id)
{
    return id >= 0 && id < pmic_rtc_nalarms(dev);
}

static int
pmic_rtc_reg_read(pmic_rtc_t* dev, uint8_t reg, void* data, int count)
{
    return i2c_kvslave_read(&dev->i2c_slave, reg, data, count);
}

static int
pmic_rtc_reg_write(pmic_rtc_t* dev, uint8_t reg, const void* data, int count)
{
    return i2c_kvslave_write(&dev->i2c_slave, reg, data, count);
}

static int
pmic_rtc_update(pmic_rtc_t* dev, uint8_t flag)
{
    int ret;

    /* Write to the update register */
    ret = pmic_rtc_reg_write(dev, RTCREG_UPDATE, &flag, 1);
    if (ret != 1) {
        dprintf("Bus error\n");
        return -1;
    }
    /* Wait for completion */
    ps_msdelay(16);
    return 0;
}

static int
pmic_rtc_set_tval(pmic_rtc_t* dev, int base, const struct rtc_time* time)
{
    int count;
    count = pmic_rtc_reg_write(dev, base, time, sizeof(*time));
    return !(count == sizeof(*time));
}

static int
pmic_rtc_get_tval(pmic_rtc_t* dev, int base, struct rtc_time* time)
{
    int count;
    count = pmic_rtc_reg_read(dev, base, time, sizeof(*time));
    time->hour &= ~RTC_HOUR_PM;
    return !(count == sizeof(*time));
}

int
pmic_rtc_init(i2c_bus_t* i2c, pmic_rtc_t* pmic_rtc)
{
    uint8_t data[7];
    int ret;
    ret = i2c_kvslave_init(i2c, MAX77686RTC_BUSADDR, LITTLE8, LITTLE8,
                           &pmic_rtc->i2c_slave);
    if (ret) {
        dprintf("Failed to register I2C slave\n");
        return -1;
    }

    data[RTCREG_INTSTAT  ] = 0x00;
    data[RTCREG_INTMASK  ] = 0x3F;
    data[RTCREG_CTRLWMASK] = RTCCTRL_24HOUR | RTCCTRL_BCD;
    data[RTCREG_CTRL     ] = RTCCTRL_24HOUR;
    data[RTCREG_UPDATE   ] = 0x00;
    data[RTCREG_WATCHDOG ] = 0x00;
    ret = pmic_rtc_reg_write(pmic_rtc, RTCREG_INTSTAT, data, sizeof(data));
    if (ret != sizeof(data)) {
        dprintf("Bus error\n");
        return -1;
    }

    return pmic_rtc_update(pmic_rtc, RTCUPDATE_WRITE);
}

int
pmic_rtc_get_time(pmic_rtc_t* pmic_rtc, struct rtc_time* time)
{
    if (pmic_rtc_update(pmic_rtc, RTCUPDATE_READ)) {
        return -1;
    }
    return pmic_rtc_get_tval(pmic_rtc, RTCREG_TIME, time);
}

int
pmic_rtc_set_time(pmic_rtc_t* pmic_rtc, const struct rtc_time* time)
{
    if (pmic_rtc_set_tval(pmic_rtc, RTCREG_TIME, time)) {
        return -1;
    }
    return pmic_rtc_update(pmic_rtc, RTCUPDATE_WRITE);
}

int
pmic_rtc_nalarms(pmic_rtc_t* pmic_rtc)
{
    return RTC_NALARMS;
}

int
pmic_rtc_get_alarm(pmic_rtc_t* pmic_rtc, int id, struct rtc_time* alarm)
{
    if (!id_valid(pmic_rtc, id)) {
        return -1;
    }
    if (pmic_rtc_update(pmic_rtc, RTCUPDATE_READ)) {
        return -1;
    }
    return pmic_rtc_get_tval(pmic_rtc, RTCREG_ALARM(id), alarm);
}


int
pmic_rtc_set_alarm(pmic_rtc_t* pmic_rtc, int id, const struct rtc_time* alarm)
{
    if (!id_valid(pmic_rtc, id)) {
        return -1;
    }
    if (pmic_rtc_set_tval(pmic_rtc, RTCREG_ALARM(id), alarm)) {
        return -1;
    }
    return pmic_rtc_update(pmic_rtc, RTCUPDATE_WRITE);
}




