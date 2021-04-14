/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

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
    i2c_kvslave_t kvslave;
} pmic_rtc_t;

/**
 * Initialise the MAX77686 RTC
 * @parma[in]  i2c      A handle to the I2C bus that the MAX77686 is connected to
 * @param[out] pmic_rtc A handle to an rtc structure to initialise
 * @return              0 on success
 */
int pmic_rtc_init(i2c_bus_t *i2c, pmic_rtc_t *pmic_rtc);

/**
 * Read the time from the MAX77686 RTC
 * @param[in]  pmic_rtc A handle to the pmic_rtc
 * @param[out] time     A time structure to populate
 * @return              0 on success
 */
int pmic_rtc_get_time(pmic_rtc_t *pmic_rtc, struct rtc_time *time);

/**
 * Set the time on the MAX77686 RTC
 * @param[in]  pmic_rtc A handle to the pmic_rtc
 * @param[in]  time     The time to set
 * @return              0 on success
 */
int pmic_rtc_set_time(pmic_rtc_t *pmic_rtc, const struct rtc_time *time);

/**
 * Return the number of alarms that this device supports
 * @param[in]  pmic_rtc A handle to the pmic_rtc
 * @return     The number of alarms that the device supports, -1 on error
 */
int pmic_rtc_nalarms(pmic_rtc_t *pmic_rtc);

/**
 * Set an alarm on the MAX77686 RTC
 * @param[in]  pmic_rtc A handle to the pmic_rtc
 * @param[in]  id       The alarm index to read
 * @param[out] alarm    A time structure to populate
 * @return              0 on success
 */
int pmic_rtc_get_alarm(pmic_rtc_t *pmic_rtc, int id, struct rtc_time *alarm);

/**
 * Set an alarm on the MAX77686 RTC
 * @param[in]  pmic_rtc a handle to the pmic_rtc
 * @param[in]  id       The alarm index to configure
 * @param[out] alarm    a time structure to populate
 * @return              0 on success
 */
int pmic_rtc_set_alarm(pmic_rtc_t *pmic_rtc, int id, const struct rtc_time *alarm);

