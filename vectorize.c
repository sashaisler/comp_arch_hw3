// bigmul_baseline.c  — Part 1: arbitrary-precision multiply (no SIMD, no libs)

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define BASE 1000000000u  // limb base = 10^9, fits in uint32_t

typedef struct {
    uint32_t *v;   // little-endian limbs (v[0] is least significant)
    size_t n;      // used limbs
    size_t cap;    // allocated limbs
} bigint;

static void die(const char *m){ fprintf(stderr,"%s\n",m); exit(1); }

static void bi_init(bigint *x, size_t cap){
    x->cap = cap ? cap : 1;
    x->n = 0;
    x->v = (uint32_t*)calloc(x->cap, sizeof(uint32_t));
    if (!x->v) die("OOM");
}
static void bi_reserve(bigint *x, size_t cap){
    if (cap <= x->cap) return;
    x->v = (uint32_t*)realloc(x->v, cap*sizeof(uint32_t));
    if (!x->v) die("OOM");
    memset(x->v + x->cap, 0, (cap - x->cap)*sizeof(uint32_t));
    x->cap = cap;
}
static void bi_trim(bigint *x){
    while (x->n && x->v[x->n-1]==0) x->n--;
}
static void bi_free(bigint *x){ free(x->v); x->v=NULL; x->n=x->cap=0; }

// Parse decimal string s (non-negative) into base-1e9 limbs
static void bi_from_string(bigint *x, const char *s){
    size_t len = strlen(s), i = 0;
    while (i < len && s[i]=='0') i++;           // skip leading zeros
    if (i == len){ bi_init(x,1); return; }      // zero
    size_t digits = len - i;
    bi_init(x, (digits+8)/9 + 2);

    size_t first = digits % 9; if (first==0) first=9;
    uint32_t chunk = 0;
    for (size_t k=0;k<first;k++,i++) chunk = chunk*10u + (uint32_t)(s[i]-'0');
    x->v[x->n++] = chunk;

    while (i < len){
        // multiply current by BASE
        uint64_t carry = 0;
        for (size_t j=0;j<x->n;j++){
            uint64_t t = (uint64_t)x->v[j]*BASE + carry;
            x->v[j] = (uint32_t)(t % BASE);
            carry = t / BASE;
        }
        if (carry){ bi_reserve(x, x->n+1); x->v[x->n++] = (uint32_t)carry; }

        // add next 9-digit chunk
        chunk = 0;
        for (int k=0;k<9 && i<len;k++,i++) chunk = chunk*10u + (uint32_t)(s[i]-'0');
        uint64_t c = chunk; size_t j=0;
        while (c){
            if (j >= x->n){ bi_reserve(x, x->n+1); x->v[x->n++] = 0; }
            uint64_t t = (uint64_t)x->v[j] + c;
            x->v[j] = (uint32_t)(t % BASE);
            c = t / BASE;
            j++;
        }
    }
    bi_trim(x);
}

// Convert to decimal string (caller frees)
static char* bi_to_string(const bigint *x){
    if (x->n==0){ char *z=malloc(2); strcpy(z,"0"); return z; }

    bigint t; bi_init(&t, x->n); memcpy(t.v, x->v, x->n*sizeof(uint32_t)); t.n=x->n;
    size_t cap = x->n*10 + 2; char *buf=(char*)malloc(cap); size_t pos=cap; buf[--pos]='\0';

    while (t.n){
        uint64_t rem = 0;
        for (ssize_t i=(ssize_t)t.n-1;i>=0;i--){
            uint64_t cur = t.v[i] + rem*BASE;
            t.v[i] = (uint32_t)(cur / BASE);
            rem = cur % BASE;
        }
        bi_trim(&t);
        if (t.n==0){
            char tmp[16]; int w=snprintf(tmp,sizeof(tmp), "%llu", (unsigned long long)rem);
            pos -= (size_t)w; memcpy(buf+pos, tmp, (size_t)w);
        }else{
            for (int k=0;k<9;k++){ buf[--pos]=(char)('0'+(rem%10)); rem/=10; }
        }
    }
    char *out = strdup(buf+pos);
    free(buf); bi_free(&t);
    return out;
}

// O(n*m) schoolbook multiply with 64-bit accumulation
static void bi_mul(const bigint *a, const bigint *b, bigint *c){
    if (a->n==0 || b->n==0){ bi_init(c,1); return; }
    size_t n=a->n, m=b->n;
    bi_init(c, n+m+1); c->n = n+m+1;
    memset(c->v, 0, c->cap*sizeof(uint32_t));

    for (size_t i=0;i<n;i++){
        uint64_t carry = 0, ai = a->v[i];
        for (size_t j=0;j<m;j++){
            uint64_t t = (uint64_t)c->v[i+j] + ai*(uint64_t)b->v[j] + carry;
            c->v[i+j] = (uint32_t)(t % BASE);
            carry = t / BASE;
        }
        size_t k = i + m;
        while (carry){
            uint64_t t = (uint64_t)c->v[k] + carry;
            c->v[k] = (uint32_t)(t % BASE);
            carry = t / BASE;
            k++;
        }
        if (k > c->n) c->n = k;
    }
    bi_trim(c);
}

int main(int argc, char **argv){
    if (argc != 3){
        fprintf(stderr, "Usage: %s A B\n", argv[0]);
        return 2;
    }
    bigint A,B,C;
    bi_from_string(&A, argv[1]);
    bi_from_string(&B, argv[2]);
    bi_mul(&A,&B,&C);

    char *s = bi_to_string(&C);
    puts(s);

    free(s); bi_free(&A); bi_free(&B); bi_free(&C);
    return 0;
}
