#ifndef WOTS_CONSTANTSUM_H
#define WOTS_CONSTANTSUM_H
#include "gmp.h"

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

#ifdef BINARYSEARCH
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
		binomial(t,i,a);
		binomial(s - (n+1)*i + t, t, b);
		binomial(s - (n+1)*i + t -1 -j, t, c);
		mpz_sub(b,b,c);
		if(i%2==0){
			mpz_addmul(out, a,b);
		}else{
			mpz_submul(out, a,b);
		}
	}
}

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
	for (b = 1; b <= t; b++ ) {
		count = ( n < s)?n:s;
		k = 0;
		while ( count > 0 ) {
			it = k;
			step = count/2;
			it = it + step;
			rank(t-b, n, s, it, keep);
			if ( mpz_cmp(keep,I)<0) {
				k = ++it;
				count -= step + 1;
			} else {
				count = step;
			}
		}
		output[b-1] = k;
		if(k>0) {
			rank(t-b, n, s, k-1, keep);
			mpz_sub(I,I,keep);
			s -= k;
		}
	}
}

/**
 * Given a constant-sum encoding and the original digest, verifies
 * if encoding corresponds to the digest, similar to a checksum.
 */
static int check_encoding(mpz_t I, int32_t t, int32_t n, int32_t s, 
						  int *encoding) 
{
	mpz_t left;
	mpz_t right;
	for(int r = 1; r <=t; r++) {
		rank(t-r, n, s, encoding[r-1]-1, left);
		rank(t-r, n, s, encoding[r-1], right);
		//if( I < left ) { return -1; }
		//if( I > right ) { return -1; }
		if( mpz_cmp(I, left)<0 ) { return -1; }
		if( mpz_cmp(I, right)>0 ) { return -1; }
		s -= encoding[r-1];
		mpz_sub(I,I,left);
	}
	return 0;
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
	mpz_t aux;
	mpz_init(aux);
	mpz_t left;
	mpz_init(left);
	mpz_t right;
	constantSumLen(t - 1, n, s, right);
	while ( !( mpz_cmp(I,left)>=0 && mpz_cmp(I,right)<0) ) {
		k++; 
		mpz_set(left,right);
		constantSumLen(t-1,n,s-k,aux);
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
