#ifndef XMSS_WOTS_H
#define XMSS_WOTS_H

#include <stdint.h>
#include "params.h"

#ifdef CONSTANTSUM
#include "gmp.h"
#ifndef T
    #define T 67
#endif
#ifndef N
    #define N 15
#endif
#ifndef S
    #define S 400
#endif


#if defined(BCACHED) || defined(VCACHED)
mpz_t bcache[T][S+1][N+1];
#endif
#ifdef CACHED
mpz_t cache[T-1][S+1];
#endif



void load_bcache(const int32_t t, const int32_t n, const int32_t s);
void load_cache(const int32_t t, const int32_t n, const int32_t s);
int check_encoding(mpz_t I, int32_t t, int32_t n, int32_t s, int *encoding);
void toConstantSum(mpz_t I, int32_t t, int32_t n, int32_t s, int *output);

#endif




/**
 * WOTS key generation. Takes a 32 byte seed for the private key, expands it to
 * a full WOTS private key and computes the corresponding public key.
 * It requires the seed pub_seed (used to generate bitmasks and hash keys)
 * and the address of this WOTS key pair.
 *
 * Writes the computed public key to 'pk'.
 */
void wots_pkgen(const xmss_params *params,
                unsigned char *pk, const unsigned char *seed,
                const unsigned char *pub_seed, uint32_t addr[8]);

/**
 * Takes a n-byte message and the 32-byte seed for the private key to compute a
 * signature that is placed at 'sig'.
 */
void wots_sign(const xmss_params *params,
               unsigned char *sig, const unsigned char *msg,
               const unsigned char *seed, const unsigned char *pub_seed,
               uint32_t addr[8]);

/**
 * Takes a WOTS signature and an n-byte message, computes a WOTS public key.
 *
 * Writes the computed public key to 'pk'.
 */
void wots_pk_from_sig(const xmss_params *params, unsigned char *pk,
                      const unsigned char *sig, const unsigned char *msg,
                      const unsigned char *pub_seed, uint32_t addr[8]);

#endif
