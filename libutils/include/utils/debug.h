/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#pragma once

#include <stdbool.h>
#include <stdio.h>

#define PRINT_ONCE(...) ({ \
                        static bool __printed = 0; \
                        if(!__printed) { \
                            printf(__VA_ARGS__); \
                            __printed=1; \
                        } \
                        })

/**
 * Display memory content to screen
 * @param[in] address   The start address of memory
 * @param[in] bytes     The number of bytes to print
 * @param[in] word_size The number of bytes in a displayed word
 */
void utils_memory_dump(void* address, size_t bytes, int word_size);
