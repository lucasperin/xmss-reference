#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../xmss.h"
#include "../params.h"
#include "../randombytes.h"


#define XMSS_MLEN 32

#ifndef XMSS_SIGNATURES
    #define XMSS_SIGNATURES 100
#endif

#ifdef XMSSMT
    #define XMSS_PARSE_OID xmssmt_parse_oid
    #define XMSS_STR_TO_OID xmssmt_str_to_oid
    #define XMSS_KEYPAIR xmssmt_keypair
    #define XMSS_SIGN xmssmt_sign
    #define XMSS_SIGN_OPEN xmssmt_sign_open
#else
    #define XMSS_PARSE_OID xmss_parse_oid
    #define XMSS_STR_TO_OID xmss_str_to_oid
    #define XMSS_KEYPAIR xmss_keypair
    #define XMSS_SIGN xmss_sign
    #define XMSS_SIGN_OPEN xmss_sign_open
#endif

#ifndef XMSS_VARIANT
    #ifdef XMSSMT
        #define XMSS_VARIANT "XMSSMT-SHA2_20/2_256"
    #else
        #define XMSS_VARIANT "XMSS-SHA2_10_256"
    #endif
#endif

static unsigned long long cpucycles(void)
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

static void print_results(unsigned long long *t, size_t tlen)
{
  size_t i;
  for (i = 0; i < tlen-1; i++) {
    t[i] = t[i+1] - t[i];
  }
  printf("\tmedian        : %llu cycles\n", median(t, tlen));
  printf("\taverage       : %llu cycles\n", average(t, tlen-1));
  printf("\n");
}

int main()
{
	printf("-----------------------\n");
    /* Make stdout buffer more responsive. */
    setbuf(stdout, NULL);

    xmss_params params;
    uint32_t oid;
    int ret = 0;
    int i;

    // TODO test more different variants
    if (XMSS_STR_TO_OID(&oid, XMSS_VARIANT)) {
#ifdef XMSSMT
        printf("XMSSMT variant %s not recognized!\n", XMSS_VARIANT);
#else
        printf("XMSS variant %s not recognized!\n", XMSS_VARIANT);
#endif
        return -1;
    }
    XMSS_PARSE_OID(&params, oid);
	
#ifdef CONSTANTSUM
	printf("##### CONSTANT SUM\n");
	printf("##### PARAMS: t=%d n=%d s=%d\n", params.wots_len, params.wots_w, params.wots_s);
#ifdef BINARYSEARCH
	printf("##### Binary Search!\n");
#endif
#ifdef CACHED
	printf("##### Using memoization!\n");
#endif
#ifdef VERIFY
	printf("##### Using V !\n");
#endif
#ifdef BCACHED
	printf("##### Using BS memoization!\n");
#endif
#ifdef VCACHED
	printf("##### Using V memoization!\n");
#endif
#else
	printf("##### rfc-XMSS \n ##### Params: t=%d n=%d\n", params.wots_len, params.wots_w);
#endif

#ifdef CACHED
	load_cache(params.wots_len,params.wots_w,params.wots_s);
#endif

#if defined(BCACHED) || defined(VCACHED)
	load_bcache(params.wots_len,params.wots_w,params.wots_s);
#endif


    unsigned char pk[XMSS_OID_LEN + params.pk_bytes];
    unsigned char sk[XMSS_OID_LEN + params.sk_bytes];
    /*unsigned char *m = malloc(XMSS_MLEN);
    unsigned char *sm = malloc(params.sig_bytes + XMSS_MLEN);
    unsigned char *mout = malloc(params.sig_bytes + XMSS_MLEN);*/
    unsigned char m[XMSS_SIGNATURES][XMSS_MLEN];
    unsigned char sm[XMSS_SIGNATURES][params.sig_bytes + XMSS_MLEN];
    unsigned char mout[XMSS_SIGNATURES][params.sig_bytes + XMSS_MLEN];
    unsigned long long smlen;
    unsigned long long mlen;

    unsigned long long t0, t1;
    unsigned long long *t = malloc(sizeof(unsigned long long) * XMSS_SIGNATURES);
    struct timespec start, stop;
    double result;

    for (i = 0; i < XMSS_SIGNATURES; i++) {
		randombytes(m[i], XMSS_MLEN);
	}

    printf("Benchmarking variant %s\n", XMSS_VARIANT);

    printf("Generating keypair.. ");

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
    t0 = cpucycles();
    XMSS_KEYPAIR(pk, sk, oid);
    t1 = cpucycles();
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);
    result = (stop.tv_sec - start.tv_sec) * 1e6 + (stop.tv_nsec - start.tv_nsec) / 1e3;
    printf("took %lf us (%.2lf sec), %llu cycles\n", result, result / 1e6, t1 - t0);

    printf("Creating %d signatures..\n", XMSS_SIGNATURES);

    for (i = 0; i < XMSS_SIGNATURES; i++) {
        t[i] = cpucycles();
        XMSS_SIGN(sk, sm[i], &smlen, m[i], XMSS_MLEN);
    }
    print_results(t, XMSS_SIGNATURES);

    printf("Verifying %d signatures..\n", XMSS_SIGNATURES);

	//randombytes(m[0], XMSS_MLEN);
    for (i = 0; i < XMSS_SIGNATURES; i++) {
        t[i] = cpucycles();
        ret |= XMSS_SIGN_OPEN(mout[i], &mlen, sm[i], smlen, pk);
    }
    print_results(t, XMSS_SIGNATURES);

    if (ret) {
        printf("DETECTED VERIFICATION ERRORS!\n");
    }

    printf("Signature size: %d (%.2f KiB)\n", params.sig_bytes, params.sig_bytes / 1024.0);
    printf("Public key size: %d (%.2f KiB)\n", params.pk_bytes, params.pk_bytes / 1024.0);
    printf("Secret key size: %llu (%.2f KiB)\n", params.sk_bytes, params.sk_bytes / 1024.0);

	printf("-----------------------\n");
    return ret;
}
