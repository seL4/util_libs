/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */

#include <abort.h>
#include <types.h>

#define DFSR_FS_MASK                0x40f
#define DFSR_FS_ASYNC_EXT_ABORT     0x406
#define DFSR_FS_ASYNC_PARITY_ERR    0x408

void check_data_abort_exception(word_t dfsr, word_t dfar)
{
    //* Check if the data exception is asynchronous external abort or
    //* asynchronous parity error on memory access */
    word_t fs = dfsr & DFSR_FS_MASK;

    if ((fs == DFSR_FS_ASYNC_EXT_ABORT) ||
       (fs == DFSR_FS_ASYNC_PARITY_ERR)) {
        return;
    }
    abort();
    (void) dfar;
}

void valid_exception(void)
{

}

void invalid_exception(void)
{
    abort();
}
