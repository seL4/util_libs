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

#include <stdint.h>
#include <platsupport/io.h>
#include <platsupport/tmu.h>
#include "xadc.h"

/* Macros for converting from the raw sensor value into degrees kelvin.
 * For a full description of this process, see
 * 7 Series FPGAs and Zynq-7000 All Programmable SoC XADC
 * Dual 12-bit 1 MSPS Analog-to-Digital Converter User Guide,
 * CH. 2: Analog-to-Digital Converter, Temperature Sensor (p32)
 */

/* The temperature is encoded in the 12 MSBs of 16 LSBs of the reported value */
#define TEMPERATURE_RAW_TO_CODE(x) (((x) & MASK(16)) >> 4u)

/* Converts a word representation of a temperature obtained from the
 * temperature sensor into degrees kelvin */
#define TEMPERATURE_RAW_TO_MILLIKELVIN(x) \
        ((TEMPERATURE_RAW_TO_CODE((x)) * 503975) / 4096)

static uint32_t get_raw_temperature(ps_tmu_t* tmu) {
    return xadc_read_register(XADC_ADDRESS_TEMPERATURE);
}

static temperature_t get_temperature_millikelvin(ps_tmu_t* tmu) {
    return TEMPERATURE_RAW_TO_MILLIKELVIN(get_raw_temperature(tmu));
}

int ps_tmu_init(enum tmu_id id, ps_io_ops_t* ops, ps_tmu_t* dev) {

    xadc_init(ops);

    dev->get_temperature = get_temperature_millikelvin;
    dev->get_raw_temperature = get_raw_temperature;

    return 0;
}
