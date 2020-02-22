#ifndef WOTS_CONSTANTSUM_H
#define WOTS_CONSTANTSUM_H
#include "gmp.h"
#include <stdint.h>

#ifndef T
    #define T 67
#endif
#ifndef N
    #define N 15
#endif
#ifndef S
    #define S 400
#endif

/**
 * Ranking function, returns the j-th rank for params,
 * used to find bounds in the constant-sum encoding.
 */
void rank(int32_t t, int32_t n, int32_t s, int32_t j, mpz_t out);


#if defined(BCACHED) || defined(VCACHED)
mpz_t bcache[T][S+1][N+1];

void load_bcache(const int32_t t, const int32_t n, const int32_t s);
#endif


int check_encoding(mpz_t I, int32_t t, int32_t n, int32_t s, int *encoding);


#ifdef BINARYSEARCH
void toConstantSum(mpz_t I, int32_t t, int32_t n, int32_t s,int *output);
#else


#ifdef CACHED
mpz_t cache[T-1][S+1];

void load_cache(const int32_t t, const int32_t n, const int32_t s);
#endif

void toConstantSum(mpz_t I, int32_t t, int32_t n, int32_t s, int *output);
#endif





#endif
