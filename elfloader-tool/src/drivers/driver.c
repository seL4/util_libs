/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <elfloader_common.h>
#include <drivers/common.h>
#include <printf.h>
#include <strops.h>

#define DRIVER_COMMON 1
#include <devices_gen.h>
#undef DRIVER_COMMON


static int table_has_match(const char *compat, const struct dtb_match_table *table)
{
    for (int i = 0; table[i].compatible != NULL; i++) {
        if (!strcmp(table[i].compatible, compat)) {
            return i;
        }
    }

    return -1;
}

static int init_device(struct elfloader_device *dev)
{
    struct elfloader_driver **drvp = __start__driver_list;

    while (drvp < __stop__driver_list) {
        struct elfloader_driver *drv = *drvp;
        int ret = table_has_match(dev->compat, drv->match_table);
        if (ret >= 0) {
            dev->drv = drv;
            drv->init(dev, drv->match_table[ret].match_data);
        }
        drvp++;
    }

    return 0;

}

int initialise_devices(void)
{
    for (unsigned int i = 0; i < ARRAY_SIZE(elfloader_devices); i++) {
        int ret = init_device(&elfloader_devices[i]);
        if (ret) {
            return ret;
        }
    }

    return 0;
}
