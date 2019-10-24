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
static void toConstantSum(mpz_t i, int32_t t, int32_t n, int32_t s, 
						  int *output) 
{
	if (t == 1){
		output[0] = s;
		return;
	}
	uint32_t k = 0;
	mpz_t aux;
	mpz_init(aux);
	mpz_t left;
	mpz_init(left);
	mpz_t right;
	constantSumLen(t - 1, n, s, right);
	while ( !( mpz_cmp(i,left)>=0 && mpz_cmp(i,right)<0) ) {
		k++; 
		mpz_set(left,right);
		constantSumLen(t-1,n,s-k,aux);
		mpz_add(right,right,aux);
	}
	output[t-1] = k;
	mpz_sub(i,i,left);
	toConstantSum(i, t - 1, n, s-k, output);
}

#endif
