/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef _ARCH_MAPPING_H
#define _ARCH_MAPPING_H

#include <autoconf.h>
#include <utils/attribute.h>

/* ordered list of page sizes for this architecture */
static const UNUSED size_t utils_page_sizes[] = {
    12, 
    21, 
};

#endif /* _ARCH_MAPPING_H */
