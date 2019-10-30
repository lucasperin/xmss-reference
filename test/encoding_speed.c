#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../constant_sum.h"
#include "../randombytes.h"

#define MLEN 32

#ifndef REPETITIONS
    #define REPETITIONS 10
#endif

#ifndef T
    #define T 34
#endif
#ifndef N
    #define N 256
#endif
#ifndef S
    #define S 3099
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
    /* Make stdout buffer more responsive. */
    setbuf(stdout, NULL);
	int ret = 0;

#if defined(CONSTANTSUM)
    int i;
#ifdef CKYI
	printf("CKYI t=%d s=%d\n", T,S);
#else
#ifdef CKY
	(void)binomial;
	printf("CKY t=%d s=%d\n", T,S);
#else
#ifdef BINARYSEARCH
	(void)check_encoding; //Surpress warning if not used
	printf("BS t=%d n=%d s=%d\n", T,N,S);
#else
	printf("CS t=%d n=%d s=%d\n", T,N,S);
#endif
#endif
#endif

    unsigned long long *t = malloc(sizeof(unsigned long long) * REPETITIONS);

    unsigned char *m = malloc(MLEN);
	mpz_t I[REPETITIONS];
	printf("setup..");
    for (i = 0; i < REPETITIONS; i++) {
		randombytes(m, MLEN);
		mpz_init(I[i]); mpz_import(I[i], MLEN,1,1,0,0, m);
	}

	int encoding[REPETITIONS][T];
    for (i = 0; i < REPETITIONS; i++) {
        t[i] = cpucycles();
		toConstantSum(I[i], T, N, S, encoding[i]);
    }
    print_results(t, REPETITIONS);
#ifdef BINARYSEARCH
	printf("VF t=%d n=%d s=%d\n", T,N,S);
    for (i = 0; i < REPETITIONS; i++) {
        t[i] = cpucycles();
		ret |= check_encoding(I[i], T, N, S, encoding[i]);
    }
    print_results(t, REPETITIONS);
#endif


#endif
    return ret;
}


