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

#ifndef _CRYPT_SHA256_H
#define _CRYPT_SHA256_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint64_t len;    /* processed message length */
	uint32_t h[8];   /* hash state */
	uint8_t buf[64]; /* message block buffer */
} sha256_t;

void sha256_init(sha256_t *s);
void sha256_sum(sha256_t *s, uint8_t *md);
void sha256_update(sha256_t *s, const void *m, unsigned long len);

#ifdef __cplusplus
}
#endif

#endif
