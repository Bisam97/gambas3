/***************************************************************************

  hash.h

  (c) Benoît Minisini <benoit.minisini@gambas-basic.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
  MA 02110-1301, USA.

***************************************************************************/

#ifndef __HASH_H
#define __HASH_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef struct md5_ctx_t {
	uint8_t wbuffer[64]; /* always correctly aligned for uint64_t */
	void (*process_block)(struct md5_ctx_t*);
	uint64_t total64;    /* must be directly before hash[] */
	uint32_t hash[8];    /* 4 elements for md5, 5 for sha1, 8 for sha256 */
} md5_ctx_t;
typedef struct md5_ctx_t sha1_ctx_t;
typedef struct md5_ctx_t sha256_ctx_t;
typedef struct sha512_ctx_t {
	uint64_t total64[2];  /* must be directly before hash[] */
	uint64_t hash[8];
	uint8_t wbuffer[128]; /* always correctly aligned for uint64_t */
} sha512_ctx_t;
typedef struct sha3_ctx_t {
	uint64_t state[25];
	unsigned bytes_queued;
	unsigned input_block_bytes;
} sha3_ctx_t;
void md5_begin(md5_ctx_t *ctx);
void md5_hash(md5_ctx_t *ctx, const void *buffer, size_t len);
unsigned md5_end(md5_ctx_t *ctx, void *resbuf);
void sha1_begin(sha1_ctx_t *ctx);
#define sha1_hash md5_hash
unsigned sha1_end(sha1_ctx_t *ctx, void *resbuf);
void sha256_begin(sha256_ctx_t *ctx);
#define sha256_hash md5_hash
#define sha256_end  sha1_end
void sha512_begin(sha512_ctx_t *ctx);
void sha512_hash(sha512_ctx_t *ctx, const void *buffer, size_t len);
unsigned sha512_end(sha512_ctx_t *ctx, void *resbuf);
void sha3_begin(sha3_ctx_t *ctx);
void sha3_hash(sha3_ctx_t *ctx, const void *buffer, size_t len);
unsigned sha3_end(sha3_ctx_t *ctx, void *resbuf);
/* TLS benefits from knowing that sha1 and sha256 share these. Give them "agnostic" names too */
typedef struct md5_ctx_t md5sha_ctx_t;
#define md5sha_hash md5_hash
#define sha_end sha1_end
enum {
	MD5_OUTSIZE    = 16,
	SHA1_OUTSIZE   = 20,
	SHA256_OUTSIZE = 32,
	SHA512_OUTSIZE = 64,
	SHA3_OUTSIZE   = 28,
};

#endif
