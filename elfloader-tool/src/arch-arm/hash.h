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

#ifndef _CRYPT_HASH_H
#define _CRYPT_HASH_H

#include "crypt_sha256.h"
#include "crypt_md5.h"

/* enum to store the hashing methods */
enum hash_methods {
    SHA_256,
    MD5
};

/* Structure that contains a structure for each hash type and an integer representation
 * of the hashing method used
 */
typedef struct {
    sha256_t sha_structure;
    md5_t md5_structure;
    unsigned int hash_type;
} hashes_t;

void get_hash(hashes_t hashes, const void *file_to_hash, unsigned long bytes_to_hash, uint8_t *outputted_hash);
void print_hash(uint8_t *hash_to_print, int bytes_to_print);

#endif
