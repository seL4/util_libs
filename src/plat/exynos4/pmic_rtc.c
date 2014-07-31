/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */


#include <platsupport/plat/pmic_rtc.h>

#define PMICRTC_DEBUG
#ifdef PMICRTC_DEBUG
#define dprintf(...) printf("PMIC RTC:" __VA_ARGS__)
#else
#define dprintf(...) do{}while(0)
#endif

#define RTCREG_TIME      0x07
#define RTCREG_ALARM1    0x0E
#define RTCREG_ALARM(id) (RTCREG_ALARM1 + (id) * sizeof(struct rtc_time))

#define RTC_NALARMS      2

static int
id_valid(pmic_rtc_t* dev, int id){
    return id >= 0 && id < pmic_rtc_nalarms(dev);
}


static int
pmic_rtc_reg_read(pmic_rtc_t* dev, uint8_t reg, void* data, int count){
    return i2c_kvslave_read(&dev->i2c_slave, reg, data, count);
}

static int
pmic_rtc_reg_write(pmic_rtc_t* dev, uint8_t reg, const void* data, int count){
    return i2c_kvslave_write(&dev->i2c_slave, reg, data, count);
}

static int
pmic_rtc_set_tval(pmic_rtc_t* dev, int base, const struct rtc_time* time){
    int count;
    count = pmic_rtc_reg_write(dev, base, time, sizeof(*time));
    return !(count == sizeof(*time));
}

static int
pmic_rtc_get_tval(pmic_rtc_t* dev, int base, struct rtc_time* time){
    int count;
    count = pmic_rtc_reg_read(dev, base, time, sizeof(*time));
    return !(count == sizeof(*time));
}

int
pmic_rtc_init(i2c_bus_t* i2c, pmic_rtc_t* pmic_rtc){
    uint8_t data;
    int ret;
    ret = i2c_kvslave_init(i2c, MAX77686RTC_BUSADDR, LITTLE8, LITTLE8, 
                           &pmic_rtc->i2c_slave);
    if(ret){
        dprintf("Failed to register I2C slave\n");
        return -1;
    }
    /* Read some dummy data */
    ret = pmic_rtc_reg_read(pmic_rtc, 0x00, &data, 1);
    if(ret != 1){
        dprintf("Bus error\n");
        return -1;
    }
    /* TODO perform some kind of sanity check... */
    return 0;
}

int
pmic_rtc_get_time(pmic_rtc_t* pmic_rtc, struct rtc_time* time){
    return pmic_rtc_get_tval(pmic_rtc, RTCREG_TIME, time); 
}

int
pmic_rtc_set_time(pmic_rtc_t* pmic_rtc, const struct rtc_time* time){
    return pmic_rtc_set_tval(pmic_rtc, RTCREG_TIME, time); 
}

int
pmic_rtc_nalarms(pmic_rtc_t* pmic_rtc){
    return RTC_NALARMS;
}

int
pmic_rtc_get_alarm(pmic_rtc_t* pmic_rtc, int id, struct rtc_time* alarm){
    if(!id_valid(pmic_rtc, id)){
        return -1;
    }
    return pmic_rtc_get_tval(pmic_rtc, RTCREG_ALARM(id), alarm);
}


int
pmic_rtc_set_alarm(pmic_rtc_t* pmic_rtc, int id, const struct rtc_time* alarm){
    if(!id_valid(pmic_rtc, id)){
        return -1;
    }
    return pmic_rtc_set_tval(pmic_rtc, RTCREG_ALARM(id), alarm);
}




