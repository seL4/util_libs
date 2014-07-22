/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <platsupport/io.h>
#include <stdio.h>
#include <stdlib.h>

#define _printf(...) printf(__VA_ARGS__)

#define _malloc(x) malloc(x)
#define _free(x)   free(x)

#define RESOURCE(op, id) ps_io_map(&(op->io_mapper),  (uintptr_t) id##_PADDR, id##_SIZE, 0, PS_MEM_NORMAL)

#define MAP_IF_NULL(op, id, ptr)               \
    do {                                         \
        if(ptr == NULL){                        \
            ptr = RESOURCE(op, id);             \
        }                                        \
    }while(0)


#define MAPCHECK(a, v) assert(((uintptr_t)(a) & 0xfff) == (v))

