/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __MACHSUPPORT_MACH_PMIC_RTC__
#define __MACHSUPPORT_MACH_PMIC_RTC__

#include <stdint.h>
#include <platsupport/i2c.h>

#define MAX77686RTC_BUSADDR 0xc

struct rtc_time {
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
/// Represented as a bitfield: (1 << #day)
    uint8_t weekday;
    uint8_t month;
    uint8_t year;
    uint8_t date;
};



typedef struct pmic_rtc {
    i2c_slave_t i2c_slave;
} pmic_rtc_t;

/**
 * Initialise the MAX77686 RTC
 * @parma[in]  i2c      A handle to the I2C bus that the MAX77686 is connected to
 * @param[out] pmic_rtc A handle to an rtc structure to initialise
 * @return              0 on success
 */
int pmic_rtc_init(i2c_bus_t* i2c, pmic_rtc_t* pmic_rtc);

/**
 * Read the time from the MAX77686 RTC
 * @param[in]  pmic_rtc A handle to the pmic_rtc
 * @param[out] time     A time structure to populate
 * @return              0 on success
 */
int pmic_rtc_get_time(pmic_rtc_t* pmic_rtc, struct rtc_time* time);

/**
 * Set the time on the MAX77686 RTC
 * @param[in]  pmic_rtc A handle to the pmic_rtc
 * @param[in]  time     The time to set
 * @return              0 on success
 */
int pmic_rtc_set_time(pmic_rtc_t* pmic_rtc, const struct rtc_time* time);

/**
 * Return the number of alarms that this device supports
 * @param[in]  pmic_rtc A handle to the pmic_rtc
 * @return     The number of alarms that the device supports, -1 on error
 */
int pmic_rtc_nalarms(pmic_rtc_t* pmic_rtc);

/**
 * Set an alarm on the MAX77686 RTC
 * @param[in]  pmic_rtc A handle to the pmic_rtc
 * @param[in]  id       The alarm index to read
 * @param[out] alarm    A time structure to populate
 * @return              0 on success
 */
int pmic_rtc_get_alarm(pmic_rtc_t* pmic_rtc, int id, struct rtc_time* alarm);

/**
 * Set an alarm on the MAX77686 RTC
 * @param[in]  pmic_rtc a handle to the pmic_rtc
 * @param[in]  id       The alarm index to configure
 * @param[out] alarm    a time structure to populate
 * @return              0 on success
 */
int pmic_rtc_set_alarm(pmic_rtc_t* pmic_rtc, int id, const struct rtc_time* alarm);

#endif /* __MACHSUPPORT_MACH_RTC__ */
