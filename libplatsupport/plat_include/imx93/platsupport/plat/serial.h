/*
 * Copyright 2024, Indan Zupancic
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

enum chardev_id {
    PS_SERIAL0,
    PS_SERIAL1,
    PS_SERIAL2,
    PS_SERIAL3,
    PS_SERIAL4,
    PS_SERIAL5,
    PS_SERIAL6,
    PS_SERIAL7,

    NUM_CHARDEV,

    PS_SERIAL_DEFAULT = PS_SERIAL0
};
