/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <platsupport/io.h>
#include <stdio.h>
#include <stdlib.h>

#define RESOURCE(op, id) ps_io_map(&(op->io_mapper),  (uintptr_t) id##_PADDR, id##_SIZE, 0, PS_MEM_NORMAL)

#define MAP_IF_NULL(op, id, ptr)               \
    do {                                         \
        if(ptr == NULL){                        \
            ptr = RESOURCE(op, id);             \
        }                                        \
    }while(0)
