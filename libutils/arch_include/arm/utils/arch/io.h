/*
* Copyright 2017, Data61
* Commonwealth Scientific and Industrial Research Organisation (CSIRO)
* ABN 41 687 119 230.
* This software may be distributed and modified according to the terms of
* the BSD 2-Clause license. Note that NO WARRANTY is provided.
* See "LICENSE_BSD2.txt" for details.
* @TAG(D61_BSD)
*/

#pragma once

#define DMB()     asm volatile("dmb" ::: "memory")
#define DSB()     asm volatile("dsb" ::: "memory")
#define ISB()     asm volatile("isb" ::: "memory")
#define IO_READ_MEM_BARRIER()	DMB()
#define IO_WRITE_MEM_BARRIER()	DMB()
