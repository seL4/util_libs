/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef _PLATSUPPORT_RTC_H
#define _PLATSUPPORT_RTC_H

#include <platsupport/io.h>
#include <platsupport/plat/acpi/acpi.h>

typedef struct rtc_time_date {
    unsigned int second;
    unsigned int minute;
    unsigned int hour;
    unsigned int day;
    unsigned int month;
    unsigned int year;
} rtc_time_date_t;

/* Retrieve the current time from the RTC. This can be passed a pre calculated
 * century register, or 0 if the century should be 'guessed'. Century guessing
 * works by having compiled in the current year at build time and using that
 * to guess what the century most likely is. Should work for ~99 years.
 * Recommended that a wrapper is used that retrieves century_register from ACPI
 *
 * @param io_port_ops io port operations for accessing the CMOS RTC
 * @param century_register Offset of the century register in the CMOS, or 0 if doesn't exist
 * @param time_date time and date structure to fill in
 * @return returns 0 on success
 */
int rtc_get_time_date_reg(ps_io_port_ops_t *io_port_ops, unsigned int century_reg, rtc_time_date_t *time_date);

/* Retrieves the century register from the ACPI tables. You can then use this
 * to pass to the rtc_get_time_date_century function without needing to keep
 * acpi tables around
 *
 * @param acpi acpi tables for detecting century register
 * @return returns century register offset, or 0 if not found
 */
unsigned int rtc_get_century_register(acpi_t *acpi);

/* Retrieve the current time from the RTC. This is the 'nicest', does everything
 * for you function. But has dependency on ACPI
 *
 * @param io_port_ops io port operations for accessing the CMOS RTC
 * @param acpi acpi tables for detecting century register
 * @param time_date time and date structure to fill in
 * @return returns 0 on success
 */
static inline int rtc_get_time_date(ps_io_port_ops_t *io_port_ops, acpi_t *acpi, rtc_time_date_t *time_date) {
    return rtc_get_time_date_reg(io_port_ops, rtc_get_century_register(acpi), time_date);
}

#endif /* _PLATSUPPORT_RTC_H */

