/*
 * Copyright 2017, DornerWorks
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DORNERWORKS_GPL)
 *
 * This data was produced by DornerWorks, Ltd. of Grand Rapids, MI, USA under
 * a DARPA SBIR, Contract Number D16PC00107.
 *
 * Approved for Public Release, Distribution Unlimited.
 *
 */

#include <printf.h>
#include <types.h>

#include "../hash.h"

/* Function to perform all hash operations.
 *
 * The outputted hash is stored in the outputted_hash pointer after the "sum" operation is used.
 *
 * This way beats having a bunch of #ifdefs in the source code, and is scalable to any other
 * hashing algoritm
 */
void get_hash(hashes_t hashes, const void *file_to_hash, unsigned long bytes_to_hash, uint8_t *outputted_hash)
{
    if (hashes.hash_type == SHA_256) {
        sha256_t calculated_hash = hashes.sha_structure;
        sha256_init(&calculated_hash);
        sha256_update(&calculated_hash, file_to_hash, bytes_to_hash);
        sha256_sum(&calculated_hash, outputted_hash);
    } else {
        md5_t calculated_hash = hashes.md5_structure;
        md5_init(&calculated_hash);
        md5_update(&calculated_hash, file_to_hash, bytes_to_hash);
        md5_sum(&calculated_hash, outputted_hash);
    }
}

/* Function to print the hash */
void print_hash(uint8_t *hash_to_print, int bytes_to_print)
{
    for (int i = 0; i < bytes_to_print; i++) {
        printf("%02x", *hash_to_print++);
    }
    printf("\n");
}
