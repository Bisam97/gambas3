/*
 * c_openssl.c - Static OpenSSL class
 *
 * Copyright (C) 2019 Tobias Boege <tobias@gambas-buch.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL. If you modify
 * file(s) with this exception, you may extend this exception to
 * your version of the file(s), but you are not obligated to do so.
 * If you do not wish to do so, delete this exception statement
 * from your version. If you delete this exception statement from
 * all source files in the program, then also delete it here.
 */

#define __C_OPENSSL_C

#include <openssl/crypto.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#include "main.h"
#include "c_openssl.h"

/*
 * OpenSSL
 */

/**G
 * Return a cryptographically strongly pseudo-random string of the
 * given *Length*.
 **/
BEGIN_METHOD(OpenSSL_RandomBytes, GB_INTEGER length)

	char *ret;
	int res;

	if (VARG(length) < 1) {
		GB.Error("Invalid Parameter: length must be greater than 0");
		return;
	}
	if (VARG(length) > 0x7FFFEFF7) {
		GB.Error("Invalid Parameter: length must be less than 2,147,479,544");
		return;
	}
	ret = GB.TempString(NULL, VARG(length));
	#if OPENSSL_VERSION_NUMBER < 0x10100000L
	res = RAND_pseudo_bytes((unsigned char *) ret, VARG(length));
	#else
	res = RAND_bytes((unsigned char *) ret, VARG(length));
	#endif
	if (res == -1) {
		GB.Error("Random number generator not supported");
		return;
	} else if (res == 0) {
		GB.Error(ERR_error_string(ERR_get_error(), NULL));
		return;
	}

	GB.ReturnString(ret);

END_METHOD


BEGIN_METHOD(OpenSSL_Pbkdf2, GB_STRING password; GB_STRING salt; GB_LONG iterations; GB_INTEGER keylength; GB_STRING method)

#if OPENSSL_VERSION_NUMBER < 0x10002000L
	GB.Error("Pbkdf2 not supported");
	return;
#else
	int ret, lKey;
	char *hash;
	const EVP_MD *emethod;

	if (VARG(iterations) < 1) {
	 GB.Error("Invalid Parameter: iterations must be greater than 0");
	 return;
	}
	lKey = VARG(keylength);
	if (lKey < 1) {
		GB.Error("Invalid Parameter: keylength must be greater than 0");
		return;
	}
	if (lKey > 0x7FFFEFF7) {
		GB.Error("Invalid Parameter: keylength must be less than 2,147,479,544");
		return;
	}
	hash = GB.TempString(NULL, lKey);
	emethod = EVP_get_digestbyname(STRING(method));
	if (!emethod) {
		GB.Error("Invalid Parameter: method not a supported digest");
		return;
	}
	memset(hash, 0, lKey);
	ret = PKCS5_PBKDF2_HMAC((const char *) STRING(password), LENGTH(password), (const unsigned char *) STRING(salt),
				LENGTH(salt), (int) VARG(iterations), emethod, lKey, (unsigned char *) hash);
	if (ret == 0) {
		 GB.Error("OpenSSL Error: Pbkdf2 call failed");
		 return;
	}
	GB.ReturnString(hash);
#endif

END_METHOD

BEGIN_METHOD(OpenSSL_Scrypt, GB_STRING password; GB_STRING salt; GB_LONG N; GB_LONG r; GB_LONG p; GB_LONG keylength)

#if OPENSSL_VERSION_NUMBER < 0x10100000L
	GB.Error("Scrypt not supported");
	return;
#else
	int ret;
	uint64_t lN, lR, lP;
	size_t lKey;
	char *hash;

	lKey = VARG(keylength);
	if (lKey < 1) {
		GB.Error("Invalid Parameter: keylength must be greater than 0");
		return;
	}
	if (lKey > 0x7FFFEFF7L) {
		GB.Error("Invalid Parameter: keylength must be less than 2,147,479,544");
		return;
	}
	hash = GB.TempString(NULL, lKey);
	lN = VARG(N);
	if (lN < 2) {
		GB.Error("Invalid Parameter: N must be greater than 1");
		return;
	}
	/* Bitwise power of 2 test */
	if (lN == 0 || (lN & (lN - 1)) != 0) {
		GB.Error("Invalid Parameter: N must be a power of 2");
		return;
	}
	lR = VARG(r);
	if (lR < 1) {
		GB.Error("Invalid Parameter: r must be greater than 0");
		return;
	}
	if (lR > 0xFFFFFFFFL) {
		GB.Error("Invalid Parameter: r must be a 32-bit number");
		return;
	}
	lP = VARG(p);
	if (lP < 1) {
		GB.Error("Invalid Parameter: p must be greater than 0");
		return;
	}
	if (lP > 0xFFFFFFFFL) {
		GB.Error("Invalid Parameter: p must be a 32-bit number");
		return;
	}
	if (EVP_PBE_scrypt(NULL, 0, NULL, 0, lN, lR, lP, 0, NULL, 0) == 0) {
		GB.Error("Invalid Parameter: The combination of N, r, and p was rejected by OpenSSL");
		return;
	}
	memset(hash, 0, lKey);
	ret = EVP_PBE_scrypt((const char *) STRING(password), LENGTH(password), (const unsigned char *) STRING(salt),
				LENGTH(salt), lN, lR, lP, 0, (unsigned char *) hash, lKey);
	if (ret == 0) {
		GB.Error("OpenSSL Error: Scrypt call failed");
		return;
	}
	GB.ReturnString(hash);
#endif

END_METHOD

GB_DESC COpenSSL[] = {
	GB_DECLARE_STATIC("OpenSSL"),

	GB_STATIC_METHOD("RandomBytes", "s", OpenSSL_RandomBytes, "(Length)i"),

	GB_STATIC_METHOD("Pbkdf2", "s", OpenSSL_Pbkdf2, "(Password)s(Salt)s(Iterations)l(KeyLength)i(Method)s"),

	GB_STATIC_METHOD("Scrypt", "s", OpenSSL_Scrypt, "(Password)s(Salt)s(N)l(R)l(P)l(KeyLength)l"),

	GB_END_DECLARE
};
