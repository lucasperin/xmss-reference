#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

#include "../wots.h"
#include "../randombytes.h"
#include "../params.h"


#define WOTS_VARIANT "WOTS+"

/* Cant go too much above 1000 without major changes to the way 
 * cycle counting is made. Otherwise, program will segfault during
 * memory allocation.
 */
#define WOTS_SIGNATURES 200

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
    /* Make stdout buffer more responsive. */
    //setbuf(stdout, NULL);
	int i;

    xmss_params params;
    // TODO test more different OIDs
    uint32_t oid = 0x00000001;

    /* For WOTS it doesn't matter if we use XMSS or XMSSMT. */
    xmss_parse_oid(&params, oid);
	params.wots_w = 256;
#ifdef CONSTANTSUM //TODO We can put these in params with the ifdef
	params.wots_w     = 226;
#endif
	xmss_xmssmt_initialize_params(&params);

    unsigned char seed[WOTS_SIGNATURES][params.n];
    unsigned char pub_seed[WOTS_SIGNATURES][params.n];
    unsigned char pk1[WOTS_SIGNATURES][params.wots_sig_bytes];
    unsigned char pk2[WOTS_SIGNATURES][params.wots_sig_bytes];
    unsigned char sig[WOTS_SIGNATURES][params.wots_sig_bytes];
    unsigned char m[WOTS_SIGNATURES][params.n];
    uint32_t addr[WOTS_SIGNATURES][8];
	
    printf("Benchmarking variant %s\n", WOTS_VARIANT);
    printf("n=%d, w=%d.\n", params.n, params.wots_w);

	for(i = 0; i < WOTS_SIGNATURES; i++) {
		randombytes(seed[i], params.n);
		randombytes(pub_seed[i], params.n);
		randombytes(m[i], params.n);
		//memset(addr[i], 0 , 8*sizeof(uint32_t));
		randombytes((unsigned char *)addr[i], 8 * sizeof(uint32_t));
	}

    unsigned long long *t = malloc(sizeof(unsigned long long) * WOTS_SIGNATURES);

    printf("Generating %d keypairs..\n", WOTS_SIGNATURES);
    for (i = 0; i < WOTS_SIGNATURES; i++) {
        t[i] = cpucycles();
		wots_pkgen(&params, pk1[i], seed[i], pub_seed[i], addr[i]);
    }
    print_results(t, WOTS_SIGNATURES);

    printf("Creating %d signatures..\n", WOTS_SIGNATURES);

    for (i = 0; i < WOTS_SIGNATURES; i++) {
        t[i] = cpucycles();
		wots_sign(&params, sig[i], m[i], seed[i], pub_seed[i], addr[i]);
    }
    print_results(t, WOTS_SIGNATURES);

    printf("Verifying %d signatures..\n", WOTS_SIGNATURES);

    for (i = 0; i < WOTS_SIGNATURES; i++) {
        t[i] = cpucycles();
		wots_pk_from_sig(&params, pk2[i], sig[i], m[i], pub_seed[i], addr[i]);
    }
    print_results(t, WOTS_SIGNATURES);

    for (i = 0; i < WOTS_SIGNATURES; i++) {
		if(memcmp(pk1[i], pk2[i], params.wots_sig_bytes)) {
			printf("Signature with index %d does not match!\n Failed.\n", i);
			return -1;
		}
	}

    printf("Signature size: %d (%.2f KiB)\n", params.wots_sig_bytes, params.wots_sig_bytes / 1024.0);
    printf("Public key size: %d (%.2f KiB)\n", params.n, params.n / 1024.0); //TODO fix
    printf("Secret key size: %u (%.2f KiB)\n", params.n, params.n / 1024.0);

	printf("Success!\n");
    return 0;
}
