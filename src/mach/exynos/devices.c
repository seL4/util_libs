/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#if 0 /* this file is if'd out as it is unused and does not compile */

#include <platsupport/chardev.h>
#include "../../common.h"

#include "uart.h"


#define ALIAS(_alias, _name) {.alias = _alias, .name = _name}
const struct alias aliases[] = {
    ALIAS("uart", "uart1"),
    ALIAS(NULL, NULL)
};

const struct char_device* character_devices[] = {
    &device_uart0, &device_uart1, &device_uart2, &device_uart3,
#if defined(PLAT_EXYNOS4)
    &device_uart4,
#endif
    NULL
};


#endif 
