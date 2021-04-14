/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <platsupport/io.h>

typedef struct exynos_tmu tmu_t;
#include <platsupport/plat/tmu.h>

typedef int temperature_t;

typedef void (*tmu_alarm_callback)(tmu_t *tmu, temperature_t temperature,
                                   int level, int rising, void *token);

struct exynos_tmu {
    enum tmu_id id;
    temperature_t t_off;
    /* IRQ callbacks */
    tmu_alarm_callback falling_alarm;
    tmu_alarm_callback rising_alarm;
    void *falling_token;
    void *rising_token;
    /* Private data */
    void *priv;
};

/**
 * Initialise a Thermal Management Unit
 * @param[in]  id    The ID of the TMU that is to be initialised
 * @param[in]  vaddr The virtual address of the TMU
 * @param[out] tmu   A TMU structure to populate
 * @return           0 on success
 */
int exynos_tmu_init(enum tmu_id id, ps_io_ops_t *io_ops, tmu_t *tmu);

/**
 * Read the current temperature
 * @param[in] tmu  A handle to the TMU to probe
 * @return         The current temperature in degrees celcius
 */
temperature_t exynos_tmu_get_temperature(tmu_t *tmu);

/**
 * Set alarms for rising temperatures
 * @param[in] tmu       A handle to the TMU to configure
 * @param[in] level0    A level for threshold0, -1 if no alarm
 * @param[in] level1    A level for threshold1, -1 if no alarm
 * @param[in] level2    A level for threshold2, -1 if no alarm
 * @param[in] cb        A callback to call when the alarm has fired
 * @param[in] token     A token to pass, unmodified, to the callback
 * @return              0 on success
 */
int exynos_tmu_set_alarms_rising(tmu_t *tmu,
                                 temperature_t level0,
                                 temperature_t level1,
                                 temperature_t level2,
                                 tmu_alarm_callback cb,
                                 void *token);

/**
 * Set alarms for falling temperatures
 * @param[in] tmu       A handle to the TMU to configure
 * @param[in] level0    A level for threshold0, -1 if no alarm
 * @param[in] level1    A level for threshold1, -1 if no alarm
 * @param[in] level2    A level for threshold2, -1 if no alarm
 * @param[in] cb        A callback to call when the alarm has fired
 * @param[in] token     A token to pass, unmodified, to the callback
 * @return              0 on success
 */
int exynos_tmu_set_alarms_falling(tmu_t *tmu,
                                  temperature_t level0,
                                  temperature_t level1,
                                  temperature_t level2,
                                  tmu_alarm_callback cb,
                                  void *token);

/**
 * Allow the TMU to handle incoming IRQs
 * @param[in] tmu   A handle to the TMU that may have fired an IRQ
 */
void exynos_tmu_handle_irq(tmu_t *tmu);

