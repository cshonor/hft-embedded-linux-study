/* T4: __builtin_expect - basic block layout before/after */
#include <stdio.h>

/* WITHOUT expect */
__attribute__((noinline))
int check_plain(int *p)
{
    if (!p) return -1;
    return *p;
}

/* WITH unlikely (error path is cold) */
__attribute__((noinline))
int check_unlikely(int *p)
{
    if (__builtin_expect(!p, 0)) return -1;
    return *p;
}

/* WITH likely (success path is hot) */
__attribute__((noinline))
int check_likely(int *p)
{
    if (__builtin_expect(!p, 1)) return -1;
    return *p;  /* this is now the "cold" path! */
}

/* nested: does likely(unlikely(...)) make sense? */
__attribute__((noinline))
int check_nested(int *p, int *q)
{
    if (__builtin_expect(!!p, 1) && __builtin_expect(!!q, 0))
        return *p + *q;
    return -1;
}

/* the !! normalization */
#define my_likely(x)   __builtin_expect(!!(x), 1)
#define my_unlikely(x) __builtin_expect(!!(x), 0)

int main(void)
{
    int x = 42;
    printf("plain(NULL)  = %d\n", check_plain(NULL));
    printf("plain(&x)    = %d\n", check_plain(&x));
    printf("unlikely(&x) = %d\n", check_unlikely(&x));
    printf("likely(&x)   = %d\n", check_likely(&x));
    printf("nested(&x,&x)= %d\n", check_nested(&x, &x));

    /* !! normalization: various types */
    printf("likely(0)     = %d\n", my_likely(0));
    printf("likely(1)     = %d\n", my_likely(1));
    printf("likely(42)    = %d\n", my_likely(42));
    printf("likely(NULL)  = %d\n", my_likely((void*)0));
    return 0;
}
