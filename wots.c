#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "utils.h"
#include "hash.h"
#include "wots.h"
#include "hash_address.h"
#include "params.h"
//#ifdef CONSTANTSUM
//#include "constant_sum.h"
//#endif


#ifdef CONSTANTSUM


/**
 * Binomial coefficient n choose k for arbitrary precisions
 * integers, using GMP. Do not use uint, since it
 * may be called with negative values and return 0.
 *
 * MUST INIT out BEFORE CALLING binomial
 */
static void binomial(int32_t n, int32_t k, mpz_t out)
{
	mpz_set_ui(out,0);
	if(n < k || n<0 || k<0)
		return;
	mpz_bin_uiui(out, n, k);
}

/**
 * Ranking function, returns the j-th rank for params,
 * used to find bounds in the constant-sum encoding.
 */
static void rank(int32_t t, int32_t n, int32_t s, int32_t j, mpz_t out) {
	mpz_set_ui(out,0);
	int32_t aux = s/(n+1); //Floor of sum/(max+1)
	int32_t k = (t < aux)? t : aux; //min(blocks,aux)
    int32_t i;
	mpz_t a; mpz_init(a);
	mpz_t b; mpz_init(b);
	mpz_t c; mpz_init(c);
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
	mpz_clear(a);
	mpz_clear(b);
	mpz_clear(c);
}

#if defined(BCACHED) || defined(VCACHED)
void load_bcache(const int32_t t, const int32_t n, const int32_t s)
{
	int b,z,j;
	for(b = 0; b < t; b++) {
		for(z = 0; z<=s; z++){
			#pragma omp parallel for
			for(j = 0; j<= n; j++){
				if(j<=z){
					mpz_init(bcache[b][z][j]);
					rank(b+1,n,z,j,bcache[b][z][j]);
				}
			}
		}
	}

}
#endif

/**
 * Given a constant-sum encoding and the original digest, verifies
 * if encoding corresponds to the digest, similar to a checksum.
 */
int check_encoding(mpz_t I, int32_t t, int32_t n, int32_t s, 
						  int *encoding) 
{
	mpz_t left; mpz_init(left);
	mpz_t right; mpz_init(right);
	int k = 0;
	for(int b = 0; b <t; b++) {
		k = encoding[t-b-1];
#ifdef VCACHED
		(void)n;
		if(k == 0) {
			mpz_set_ui(left, 0);
		} else {
			mpz_set(left, bcache[t-b-1][s][k-1]);
		}
		mpz_set(right, bcache[t-b-1][s][k]);
#else
		rank(t-b, n, s, k-1, left);
		rank(t-b, n, s, k, right);
#endif
		if( mpz_cmp(left,I)>0 ) { 
			mpz_clear(left);
			mpz_clear(right);
			return -1; 
		}
		if( mpz_cmp(right,I)<=0 ) { 
			mpz_clear(left);
			mpz_clear(right);
			return -1; 
		}
		s -= encoding[t-b-1];
		mpz_sub(I,I,left);
	}
	mpz_clear(left);
	mpz_clear(right);
	return 0;
}

#ifdef BINARYSEARCH

/**
 * Constan-sum encoding function using binary search. Encodes an 
 * integer 0 <= i <=2^m into t partitions of integers from 0 to n 
 * and summing s. Writes the encoding into output.
 */
void toConstantSum(mpz_t I, int32_t t, int32_t n, int32_t s, 
						  int *output) 
{
	int count, k, it, step,b;
	mpz_t keep; mpz_init(keep);
	for (b = 0; b < t; b++ ) {
		count = ( n < s)?n:s;
		k = 0;
		while ( count > 0 ) {
			it = k;
			step = count/2;
			it = it + step;
#ifdef BCACHED
			//mpz_set(keep, *(*(*(bcache +t-b-1)+s)+it));
			mpz_set(keep, bcache[t-b-1][s][it]);
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
	mpz_clear(keep);
}

#else
/**
 * Computes cardinality of the set of t-tuples with values ranging
 * from 0 to n that sum s.
 */
static void constantSumLen(const int32_t t, const int32_t n, const int32_t s, 
						   mpz_t out) 
{
	mpz_set_ui(out,0);
	int32_t aux = s/(n+1); //Floor of sum/(max+1)
	int32_t k = (t < aux)? t : aux; //min(blocks,aux)
    int32_t i;
	mpz_t a; mpz_init(a);
	mpz_t b; mpz_init(b);
	for(i = 0; i <= k; i++ ) {
		binomial(t,i,a);
		binomial(s-(n+1)*i+t-1,t-1, b);
		if(i%2==0){
			mpz_addmul(out, a,b);
		}else{
			mpz_submul(out, a,b);
		}
	}
	mpz_clear(a);
	mpz_clear(b);
}


#ifdef CACHED
void load_cache(const int32_t t, const int32_t n, const int32_t s)
{
	int b,z;
	for(b = 1; b < t; b++) {
		for(z = 0; z<=s; z++){
			mpz_init(cache[b-1][z]);
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
void toConstantSum(mpz_t I, int32_t t, int32_t n, int32_t s, 
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
		mpz_set(aux, cache[t-2][s-k]);
#else
		constantSumLen(t-1,n,s-k,aux);
#endif
		mpz_add(right,right,aux);
	}
	output[t-1] = k;
	mpz_sub(I,I,left);
	mpz_clear(aux);
	mpz_clear(left);
	mpz_clear(right);
	toConstantSum(I, t - 1, n, s-k, output);
}

#endif
#endif


#ifdef ENC
unsigned long long *t_enc_start;
unsigned long long *t_enc_stop;
int t_enc_idx = 0;
unsigned long long *t_enc_start_v;
unsigned long long *t_enc_stop_v;
int t_enc_idx_v = 0;

static unsigned long long enc_cpucycles(void)
{
  unsigned long long result;
  __asm volatile(".byte 15;.byte 49;shlq $32,%%rdx;orq %%rdx,%%rax"
    : "=a" (result) ::  "%rdx");
  return result;
}

static int cmp_llu(const void *a, const void*b)
{
    if (*(unsigned long long *)a < *(unsigned long long *)b) return -1;
    if (*(unsigned long long *)a > *(unsigned long long *)b) return 1;
    return 0;
}

static unsigned long long median(unsigned long long *l, size_t llen)
{
    qsort(l, llen, sizeof(unsigned long long), cmp_llu);

    if (llen % 2) return l[llen / 2];
    else return (l[llen/2 - 1] + l[llen/2]) / 2;
}

static unsigned long long average(unsigned long long *t, size_t tlen)
{
    unsigned long long acc=0;
    size_t i;
    for(i = 0; i < tlen; i++) {
        acc += t[i];
    }
    return acc/(tlen);
}

static void print_results(unsigned long long *t_start, unsigned long long *t_stop, size_t tlen)
{
  size_t i;
  for (i = 0; i < tlen; i++) {
    t_start[i] = t_stop[i] - t_start[i];
  }
  printf("\tmedian        : %llu cycles\n", median(t_start, tlen+1));
  printf("\taverage       : %llu cycles\n", average(t_start, tlen));
  printf("\n");
}

void init_enc(size_t len)
{
	t_enc_start = malloc(sizeof(unsigned long long) * len);
	t_enc_stop = malloc(sizeof(unsigned long long) * len);
	t_enc_start_v = malloc(sizeof(unsigned long long) * len);
	t_enc_stop_v = malloc(sizeof(unsigned long long) * len);
}
void print_results_enc(size_t len)
{
	print_results(t_enc_start, t_enc_stop, len);
}
void print_results_enc_v(size_t len)
{
	print_results(t_enc_start_v, t_enc_stop_v, len);
}
#endif


















/**
 * Helper method for pseudorandom key generation.
 * Expands an n-byte array into a len*n byte array using the `prf` function.
 */
static void expand_seed(const xmss_params *params,
                        unsigned char *outseeds, const unsigned char *inseed)
{
    uint32_t i;
    unsigned char ctr[32];

    for (i = 0; i < params->wots_len; i++) {
        ull_to_bytes(ctr, 32, i);
        prf(params, outseeds + i*params->n, ctr, inseed);
    }
}

/**
 * Computes the chaining function.
 * out and in have to be n-byte arrays.
 *
 * Interprets in as start-th value of the chain.
 * addr has to contain the address of the chain.
 */
static void gen_chain(const xmss_params *params,
                      unsigned char *out, const unsigned char *in,
                      unsigned int start, unsigned int steps,
                      const unsigned char *pub_seed, uint32_t addr[8])
{
    uint32_t i;

    /* Initialize out with the value at position 'start'. */
    memcpy(out, in, params->n);

    /* Iterate 'steps' calls to the hash function. */
    for (i = start; i < (start+steps) && i < params->wots_w; i++) {
        set_hash_addr(addr, i);
        thash_f(params, out, out, pub_seed, addr);
    }
}

#ifndef CONSTANTSUM
/**
 * base_w algorithm as described in draft.
 * Interprets an array of bytes as integers in base w.
 * This only works when log_w is a divisor of 8.
 */
static void base_w(const xmss_params *params,
                   int *output, const int out_len, const unsigned char *input)
{
    int in = 0;
    int out = 0;
    unsigned char total;
    int bits = 0;
    int consumed;

    for (consumed = 0; consumed < out_len; consumed++) {
        if (bits == 0) {
            total = input[in];
            in++;
            bits += 8;
        }
        bits -= params->wots_log_w;
        output[out] = (total >> bits) & (params->wots_w - 1);
        out++;
    }
}

/* Computes the WOTS+ checksum over a message (in base_w). */
static void wots_checksum(const xmss_params *params,
                          int *csum_base_w, const int *msg_base_w)
{
    int csum = 0;
    unsigned char csum_bytes[(params->wots_len2 * params->wots_log_w + 7) / 8];
    unsigned int i;

    /* Compute checksum. */
    for (i = 0; i < params->wots_len1; i++) {
        csum += params->wots_w - 1 - msg_base_w[i];
    }

    /* Convert checksum to base_w. */
    /* Make sure expected empty zero bits are the least significant bits. */
    csum = csum << (8 - ((params->wots_len2 * params->wots_log_w) % 8));
    ull_to_bytes(csum_bytes, sizeof(csum_bytes), csum);
    base_w(params, csum_base_w, params->wots_len2, csum_bytes);
}

/* Takes a message and derives the matching chain lengths. */
static void chain_lengths(const xmss_params *params,
                          int *lengths, const unsigned char *msg)
{
    base_w(params, lengths, params->wots_len1, msg);
    wots_checksum(params, lengths + params->wots_len1, lengths);
}
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
                const unsigned char *pub_seed, uint32_t addr[8])
{
    uint32_t i;

    /* The WOTS+ private key is derived from the seed. */
    expand_seed(params, pk, seed);

    for (i = 0; i < params->wots_len; i++) {
        set_chain_addr(addr, i);
#ifdef CONSTANTSUM
        gen_chain(params, pk + i*params->n, pk + i*params->n,
                  0, params->wots_w, pub_seed, addr);
#else
        gen_chain(params, pk + i*params->n, pk + i*params->n,
                  0, params->wots_w - 1, pub_seed, addr);
#endif

    }
}

/**
 * Takes a n-byte message and the 32-byte seed for the private key to compute a
 * signature that is placed at 'sig'.
 */
void wots_sign(const xmss_params *params,
               unsigned char *sig, const unsigned char *msg,
               const unsigned char *seed, const unsigned char *pub_seed,
               uint32_t addr[8])
{
    int lengths[params->wots_len];
    uint32_t i;
#ifdef ENC
	t_enc_start[t_enc_idx] = enc_cpucycles();
#endif

#ifdef CONSTANTSUM
	mpz_t I; mpz_init(I); mpz_import(I, params->n,1,1,0,0, msg);
	toConstantSum(I, params->wots_len, params->wots_w, params->wots_s, lengths);
	mpz_clear(I);
#ifdef VERIFY
		short aux;
#endif
#else
    chain_lengths(params, lengths, msg);
#endif
#ifdef ENC
	t_enc_stop[t_enc_idx] = enc_cpucycles();
	t_enc_idx++;
#endif

    /* The WOTS+ private key is derived from the seed. */
    expand_seed(params, sig, seed);

	for (i = 0; i < params->wots_len; i++) {
        set_chain_addr(addr, i);
#ifdef CONSTANTSUM
        gen_chain(params, sig + i*params->n, sig + i*params->n,
                  0, params->wots_w - lengths[i], pub_seed, addr); 
#ifdef VERIFY
		aux = (short)lengths[i];
		memcpy(sig + params->wots_len*params->n + 2*i, &aux, 2);//TODO This appends the encoding to the signature. Used for faster verification.
#endif
#else
        gen_chain(params, sig + i*params->n, sig + i*params->n,
                  0, lengths[i], pub_seed, addr);
#endif
    }
}

/**
 * Takes a WOTS signature and an n-byte message, computes a WOTS public key.
 *
 * Writes the computed public key to 'pk'.
 */
void wots_pk_from_sig(const xmss_params *params, unsigned char *pk,
                      const unsigned char *sig, const unsigned char *msg,
                      const unsigned char *pub_seed, uint32_t addr[8])
{
    int lengths[params->wots_len];
    uint32_t i;
#ifdef ENC
	t_enc_start_v[t_enc_idx_v] = enc_cpucycles();
#endif
#ifdef CONSTANTSUM
	mpz_t I; mpz_init(I); mpz_import(I, params->n,1,1,0,0, msg);
#ifdef VERIFY

	short aux;
    for (i = 0; i < params->wots_len; i++) {
		memcpy(&aux, sig + params->wots_len*params->n + 2*i, 2);
		lengths[i] = aux;
	}

	if(check_encoding(I, params->wots_len, params->wots_w, params->wots_s, lengths)){
		mpz_clear(I);
		return;
	}
	mpz_clear(I);
#else
	toConstantSum(I, params->wots_len, params->wots_w, params->wots_s, lengths);
	mpz_clear(I);
#endif
#else
    chain_lengths(params, lengths, msg);
#endif
#ifdef ENC
	t_enc_stop_v[t_enc_idx_v] = enc_cpucycles();
	t_enc_idx_v++;
#endif

    for (i = 0; i < params->wots_len; i++) {
        set_chain_addr(addr, i);
#ifdef CONSTANTSUM
        gen_chain(params, pk + i*params->n, sig + i*params->n,
                  params->wots_w - lengths[i], lengths[i], pub_seed, addr);
#else
        gen_chain(params, pk + i*params->n, sig + i*params->n,
                  lengths[i], params->wots_w - 1 - lengths[i], pub_seed, addr);
#endif
    }
}

