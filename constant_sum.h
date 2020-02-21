#ifndef WOTS_CONSTANTSUM_H
#define WOTS_CONSTANTSUM_H
#include "gmp.h"

#ifndef T
    #define T 34
#endif
#ifndef N
    #define N 226
#endif
#ifndef S
    #define S 3643
#endif

/**
 * Binomial coefficient n choose k for arbitrary precisions
 * integers, using GMP. Do not use uint, since it
 * may be called with negative values and return 0.
 */
static void binomial(int32_t n, int32_t k, mpz_t out)
{
	mpz_init(out);
	if(n < k || n<0 || k<0)
		return;
	mpz_bin_uiui(out, n, k);
}

/**
 * Ranking function, returns the j-th rank for params,
 * used to find bounds in the constant-sum encoding.
 */
static void rank(int32_t t, int32_t n, int32_t s, int32_t j, mpz_t out) {
	mpz_init(out);
	int32_t aux = s/(n+1); //Floor of sum/(max+1)
	int32_t k = (t < aux)? t : aux; //min(blocks,aux)
    int32_t i;
	mpz_t a;
	mpz_t b;
	mpz_t c;
	for(i = 0; i <= k; i++ ) { 
		binomial(t-1,i,a);
		binomial(s - (n+1)*i + t-1, t-1, b);
		binomial(s - (n+1)*i + t -2 -j, t-1, c);
		mpz_sub(b,b,c);
		if(i%2==0){
			mpz_addmul(out, a,b);
		}else{
			mpz_submul(out, a,b);
		}
	}
}

#if defined(BCACHED) || defined (VCACHED)
static mpz_t bcache[T][S+1][N+1];

static void load_bcache(const int32_t t, const int32_t n, const int32_t s)
{
	int b,z,j;
	for(b = 0; b < t; b++) {
		for(z = 0; z<=s; z++){
			#pragma omp parallel for
			for(j = 0; j<= n; j++){
				if(j<=z)
					rank(b+1,n,z,j,bcache[b][z][j]);
			}
		}
	}

}
#endif

/**
 * Given a constant-sum encoding and the original digest, verifies
 * if encoding corresponds to the digest, similar to a checksum.
 */
static int check_encoding(mpz_t I, int32_t t, int32_t n, int32_t s, 
						  int *encoding) 
{
	mpz_t left;
	mpz_t right;
	int k = 0;
	for(int b = 0; b <t; b++) {
		k = encoding[t-b-1];
#ifdef VCACHED
		(void)n;
		if(k == 0) {
			mpz_init(left);
		} else {
			mpz_set(left, *(*(*(bcache +t-b-1)+s)+k-1 ));
		}
		mpz_set(right, *(*(*(bcache +t-b-1)+s)+k));
#else
		rank(t-b, n, s, k-1, left);
		rank(t-b, n, s, k, right);
#endif
		if( mpz_cmp(left,I)>0 ) { return -1; }
		if( mpz_cmp(right,I)<=0 ) { return -1; }
		s -= encoding[t-b-1];
		mpz_sub(I,I,left);
	}
	return 1;
}

#ifdef BINARYSEARCH

/**
 * Constan-sum encoding function using binary search. Encodes an 
 * integer 0 <= i <=2^m into t partitions of integers from 0 to n 
 * and summing s. Writes the encoding into output.
 */
static void toConstantSum(mpz_t I, int32_t t, int32_t n, int32_t s, 
						  int *output) 
{
	int count, k, it, step, b;
	mpz_t keep;
	for (b = 0; b < t; b++ ) {
		count = ( n < s)?n:s;
		k = 0;
		while ( count > 0 ) {
			it = k;
			step = count/2;
			it = it + step;
#ifdef BCACHED
			mpz_set(keep, *(*(*(bcache +t-b-1)+s)+it));
#else
			rank(t-b, n, s, it, keep);
#endif
			if ( mpz_cmp(I,keep)>=0) {
				k = ++it;
				count -= step + 1;
			} else {
				count = step;
			}
		}
		output[t-b-1] = k;
		if(k>0) {
			rank(t-b, n, s, k-1, keep);
			mpz_sub(I,I,keep);
			s -= k;
		}
	}
}

#else
#ifdef CKYI
/**
 * Constan-sum encoding function. Encodes an integer 0 <= i <=2^m
 * into t partitions of integers from 0 to n and summing s. Writes
 * the encoding into output. Algorithm as presented in CKY, with 
 * inverted search.
 */
static void toConstantSum(mpz_t I, int32_t t, int32_t n, int32_t s, 
						  int *output) 
{
	if (t == 1){
		output[0] = s;
		return;
	}
	int32_t k = 0;
	mpz_t left;	mpz_init(left);
	mpz_t right; binomial(s+t-2, s, right);
	mpz_t aux; mpz_init(aux); mpz_set(aux,right);
	while ( !( mpz_cmp(I,left)>=0 && mpz_cmp(I,right)<0) ) {
		k++; 
		mpz_set(left,right);
		mpz_mul_ui(aux, aux, s-k+1);
		mpz_divexact_ui(aux, aux, (s-k+1)+t-2);
		mpz_add(right,right,aux);
	}
	output[t-1] = k;
	mpz_sub(I,I,left);
	toConstantSum(I, t - 1, n, s-k, output);
}
#else
#ifdef CKY

/**
 * Constan-sum encoding function. Encodes an integer 0 <= i <=2^m
 * into t partitions of integers from 0 to n and summing s. Writes
 * the encoding into output. Algorithm as presented in CKY.
 */
static void toConstantSum(mpz_t I, int32_t t, int32_t n, int32_t s, 
						  int *output) 
{
	if (t == 1){
		output[0] = s;
		return;
	}
	int32_t k = 0;
	mpz_t left;	mpz_init(left);
	mpz_t right; mpz_init(right); mpz_set_ui(right, 1);
	mpz_t aux; mpz_init(aux); mpz_set(aux,right);
	while ( !( mpz_cmp(I,left)>=0 && mpz_cmp(I,right)<0) ) {
		k++; 
		mpz_set(left,right);
		mpz_mul_ui(aux, aux, k+t-2);
		mpz_divexact_ui(aux, aux, k);
		mpz_add(right,right,aux);
	}
	output[t-1] = s-k;
	mpz_sub(I,I,left);
	toConstantSum(I, t - 1, n, k, output);
}

#else
/**
 * Computes cardinality of the set of t-tuples with values ranging
 * from 0 to n that sum s.
 */
static void constantSumLen(const int32_t t, const int32_t n, const int32_t s, 
						   mpz_t out) 
{
	mpz_init(out);
	int32_t aux = s/(n+1); //Floor of sum/(max+1)
	int32_t k = (t < aux)? t : aux; //min(blocks,aux)
    int32_t i;
	mpz_t a;
	mpz_t b;
	for(i = 0; i <= k; i++ ) {
		binomial(t,i,a);
		binomial(s-(n+1)*i+t-1,t-1, b);
		if(i%2==0){
			mpz_addmul(out, a,b);
		}else{
			mpz_submul(out, a,b);
		}
	}
}


#ifdef CACHED
static mpz_t cache[T-1][S+1];

static void load_cache(const int32_t t, const int32_t n, const int32_t s)
{
	int b,z;
	for(b = 1; b < t; b++) {
		for(z = 0; z<=s; z++){
			constantSumLen(b,n,z,cache[b-1][z]);
		}
	}

}
#endif

/**
 * Constan-sum encoding function. Encodes an integer 0 <= i <=2^m
 * into t partitions of integers from 0 to n and summing s. Writes
 * the encoding into output.
 */
static void toConstantSum(mpz_t I, int32_t t, int32_t n, int32_t s, 
						  int *output) 
{
	if (t == 1){
		output[0] = s;
		return;
	}
	int32_t k = 0;
	mpz_t aux; mpz_init(aux);
	mpz_t left;	mpz_init(left);
	mpz_t right; mpz_init(right);
	constantSumLen(t - 1, n, s, right);
	while ( !( mpz_cmp(I,left)>=0 && mpz_cmp(I,right)<0) ) {
		k++; 
		mpz_set(left,right);
#ifdef CACHED
		//mpz_set(aux, cache[t-2][s-k]);
		mpz_set(aux, *(*(cache +t-2)+s-k));
#else
		constantSumLen(t-1,n,s-k,aux);
#endif
		mpz_add(right,right,aux);
	}
	output[t-1] = k;
	mpz_sub(I,I,left);
	toConstantSum(I, t - 1, n, s-k, output);
}

#endif
#endif
#endif
#endif
