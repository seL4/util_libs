/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 */
#include <utils/debug.h>
#include <stdint.h>
#include <utils/arith.h>
#include <utils/util.h>
#include <utils/zf_log.h>
#include <string.h>
#include <inttypes.h>


#define MD_BYTES_PER_LINE (sizeof(uint32_t) * 4)
#define MD_GROUPING       4

static void
md_print_line(void* address, int word_size)
{
    uint8_t line[MD_BYTES_PER_LINE];
    int num_objects = MD_BYTES_PER_LINE / word_size;
    int object;
    printf("%p: ", address);
    for (object = 0; object < num_objects; object++) {
        int object_offset = object * word_size;
        if (MD_GROUPING > word_size && object % ((MD_GROUPING) / word_size) == 0) {
            putchar(' ');
        }
        switch (word_size) {
        case 1: {
            uint8_t temp = *(volatile uint8_t*)(address + object_offset);
            printf("0x%02x ", temp);
            memcpy(&line[object_offset], &temp, sizeof(temp));
            break;
        }
        case 2: {
            uint16_t temp = *(volatile uint16_t*)(address + object_offset);
            printf("0x%04x ", temp);
            memcpy(&line[object_offset], &temp, sizeof(temp));
            break;
        }
        case 4: {
            uint32_t temp = *(volatile uint32_t*)(address + object_offset);
            printf("0x%08x ", temp);
            memcpy(&line[object_offset], &temp, sizeof(temp));
            break;
        }
        case 8: {
            uint64_t temp = *(volatile uint64_t*)(address + object_offset);
            printf("0x%016"PRIx64" ", temp);
            memcpy(&line[object_offset], &temp, sizeof(temp));
            break;
        }
        }
    }
    /* Print ASCII string */
    printf("    |");
    for (object = 0; object < MD_BYTES_PER_LINE; object++) {
        if (line[object] < 32 || line[object] > 126) {
            putchar('.');
        } else {
            putchar(line[object]);
        }
    }
    printf("|\n");
}

void
utils_memory_dump(void* address, size_t bytes, int word_size)
{
    void* a;
    if (word_size == 1 || word_size == 2 || word_size == 4 || word_size == 8) {
        /* Notify the caller if 'bytes' is not a multple of MD_BYTES_PER_LINE */
        if (bytes % MD_BYTES_PER_LINE) {
            int extra_bytes = MD_BYTES_PER_LINE - (bytes % MD_BYTES_PER_LINE);
            LOG_INFO("Rounding displayed bytes from %zu up to %zu", bytes, bytes + extra_bytes);
            bytes += extra_bytes;
        }
        /* Print each line */
        for (a = address; a < address + bytes; a += MD_BYTES_PER_LINE) {
            md_print_line(a, word_size);
        }
    } else {
        LOG_ERROR("Invalid word size (%d). Valid options are [1, 2, 4, 8]", word_size);
    }
}
