/* T3: __builtin_constant_p - compile-time constant detection */
#include <stdio.h>
#include <string.h>

/* the kernel's constant-size memset optimization pattern */
#define my_memset(s, c, n) do {                          \
    if (__builtin_constant_p(n) && (n) <= 16) {          \
        if (__builtin_constant_p(c)) {                   \
            /* compiler can unroll */                    \
            char *_p = (char *)(s);                       \
            size_t _i;                                   \
            for (_i = 0; _i < (n); _i++) _p[_i] = (c);   \
        } else {                                         \
            memset(s, c, n);                             \
        }                                                \
    } else {                                             \
        memset(s, c, n);                                 \
    }                                                    \
} while (0)

/* the kernel's bit set macro */
#define set_bit_constant(x, n) ({                         \
    int _r;                                               \
    if (__builtin_constant_p(n)) {                        \
        (x) |= (1UL << (n));                              \
        _r = 0;                                           \
    } else {                                              \
        /* runtime: use bts instruction or atomics */     \
        _r = -1; /* simplified */                          \
    }                                                      \
    _r;                                                    \
})

/* which branch survives? */
__attribute__((noinline))
int test_const(int x)
{
    if (__builtin_constant_p(x))
        return 1;  /* this branch survives only if x is always constant */
    else
        return 0;
}

__attribute__((noinline))
int test_const5(void)
{
    return __builtin_constant_p(5);     /* should be 1 */
}

__attribute__((noinline))
int test_const_var(void)
{
    volatile int v = 42;  /* volatile = not constant */
    return __builtin_constant_p(v);    /* should be 0 */
}

int main(void)
{
    char buf[32] = "hello world";
    printf("constant_p(5) = %d\n", test_const5());
    printf("constant_p(volatile v) = %d\n", test_const_var());

    /* the macro: n=5 is constant -> inline path */
    my_memset(buf, 'X', 5);
    printf("after my_memset(buf, X, 5): %.5s\n", buf);

    /* n is runtime -> memset path */
    int n = 3;
    my_memset(buf, 'Y', n);
    printf("after my_memset(buf, Y, n=%d): %.3s\n", n, buf);

    /* what about global static const? */
    static const int g = 7;
    printf("constant_p(static const g) = %d\n", __builtin_constant_p(g));

    /* what about address of local? */
    int local = 10;
    printf("constant_p(&local) = %d\n", __builtin_constant_p(&local));
    printf("constant_p(local) = %d\n", __builtin_constant_p(local));

    return 0;
}
