/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/* Some code from here was taken from http://wiki.osdev.org/CMOS
   http://wiki.osdev.org/OSDev_Wiki:License indicates that such
   code is in the public domain. */

#include <string.h>
#include <platsupport/plat/rtc.h>
#include <utils/util.h>

#define UNBCD(x) (( (x) & 0x0F) + (( (x) / 16) * 10))

#define CMOS_ADDRESS 0x70
#define CMOS_DATA    0x71

static inline int current_year()
{
#ifdef __DATE__
    return atoi(&__DATE__[7]);
#else
    return 2014;
#endif
}

static unsigned char get_RTC_register(ps_io_port_ops_t *port_ops, int reg)
{
    int error UNUSED;
    error = ps_io_port_out(port_ops, CMOS_ADDRESS, 1, reg);
    assert(!error);
    uint32_t val;
    error = ps_io_port_in(port_ops, CMOS_DATA, 1, &val);
    assert(!error);
    return val;
}

static int get_update_in_progress_flag(ps_io_port_ops_t *port_ops)
{
    return get_RTC_register(port_ops, 0x0A) & 0x80;
}

/* We pack this struct so we can use memcmp */
typedef struct __attribute__((packed)) rtc_raw {
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char day;
    unsigned char month;
    unsigned char year;
    unsigned char century;
} rtc_raw_t;

static void read_rtc(ps_io_port_ops_t *port_ops, unsigned int century_reg, rtc_raw_t *time_date)
{
    /* Wait until an update isn't in progress */
    while (get_update_in_progress_flag(port_ops));

    time_date->second = get_RTC_register(port_ops, 0x00);
    time_date->minute = get_RTC_register(port_ops, 0x02);
    time_date->hour = get_RTC_register(port_ops, 0x04);
    time_date->day = get_RTC_register(port_ops, 0x07);
    time_date->month = get_RTC_register(port_ops, 0x08);
    time_date->year = get_RTC_register(port_ops, 0x09);
    if (century_reg != 0) {
        time_date->century = get_RTC_register(port_ops, century_reg);
    } else {
        time_date->century = 0;
    }
}

static int time_cmp(rtc_raw_t a, rtc_raw_t b)
{
    return memcmp(&a, &b, sizeof(a));
}

int rtc_get_time_date_reg(ps_io_port_ops_t *io_port_ops, unsigned int century_reg, rtc_time_date_t *time_date)
{
    /* Keep performing reads until we manage to do two reads in a row that are
     * the same */
    rtc_raw_t raw_time;
    rtc_raw_t temp;
    do {
        read_rtc(io_port_ops, century_reg, &raw_time);
        read_rtc(io_port_ops, century_reg, &temp);
    } while (time_cmp(raw_time, temp) != 0);

    /* Start putting the time into the final struct */
    time_date->second = raw_time.second;
    time_date->minute = raw_time.minute;
    time_date->hour = raw_time.hour;
    time_date->day = raw_time.day;
    time_date->month = raw_time.month;
    time_date->year = raw_time.year;

    unsigned char registerB;
    registerB = get_RTC_register(io_port_ops, 0x0B);

    // Convert BCD to binary values if necessary

    if (!(registerB & 0x04)) {
        time_date->second = UNBCD(time_date->second);
        time_date->minute = UNBCD(time_date->minute);
        time_date->hour = UNBCD(time_date->hour);
        time_date->day = UNBCD(time_date->day);
        time_date->month = UNBCD(time_date->month);
        time_date->year = UNBCD(time_date->year);
        if (century_reg != 0) {
            raw_time.century = UNBCD(raw_time.century);
        }
    }

    // Convert 12 hour clock to 24 hour clock if necessary

    if (!(registerB & 0x02) && (time_date->hour & 0x80)) {
        time_date->hour = ((time_date->hour & 0x7F) + 12) % 24;
    }

    // Calculate the full (4-digit) year

    if (century_reg != 0) {
        time_date->year += raw_time.century * 100;
    } else {
        time_date->year += (current_year() / 100) * 100;
        if (time_date->year < current_year()) {
            time_date->year += 100;
        }
    }
    return 0;
}

unsigned int rtc_get_century_register(acpi_t *acpi)
{
    acpi_header_t *header = acpi_find_region(acpi, ACPI_FADT);
    if (!header) {
        ZF_LOGE("ACPI has no FADT header. Your BIOS is broken");
        return 0;
    }
    acpi_fadt_t *fadt = (acpi_fadt_t *)header;
    return fadt->century;
}
