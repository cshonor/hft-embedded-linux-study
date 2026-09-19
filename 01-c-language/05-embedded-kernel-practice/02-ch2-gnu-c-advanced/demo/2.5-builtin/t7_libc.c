/* T7: libc builtins - strlen, memcpy, memset, strcmp compile-time folding */
#include <stdio.h>
#include <string.h>

/* constant strlen: does compiler fold it? */
__attribute__((noinline))
size_t do_strlen_const(void)
{
    return strlen("hello");  /* should fold to 5 */
}

/* runtime strlen: still calls real function */
__attribute__((noinline))
size_t do_strlen_rt(const char *s)
{
    return strlen(s);
}

/* constant memcpy: inline to mov instructions */
__attribute__((noinline))
void do_memcpy_const(char *dst)
{
    char src[] = "ABCDE";
    memcpy(dst, src, 5);  /* size known at compile time */
}

/* runtime memcpy: calls real function */
__attribute__((noinline))
void do_memcpy_rt(char *dst, const char *src, size_t n)
{
    memcpy(dst, src, n);
}

/* large constant memcpy: still inline or call? */
__attribute__((noinline))
void do_memcpy_large(char *dst)
{
    char src[256] = {0};
    memcpy(dst, src, 256);
}

/* __builtin_memcpy vs memcpy: same? */
__attribute__((noinline))
void do_builtin_memcpy(char *dst)
{
    __builtin_memcpy(dst, "world", 5);
}

/* strcmp constant folding */
__attribute__((noinline))
int do_strcmp_const(void)
{
    return strcmp("abc", "abc");  /* should fold to 0 */
}

/* memset constant */
__attribute__((noinline))
void do_memset_const(char *buf)
{
    memset(buf, 0, 16);
}

int main(void)
{
    char dst[32];
    printf("strlen_const = %zu\n", do_strlen_const());
    printf("strlen_rt(hello) = %zu\n", do_strlen_rt("hello"));
    do_memcpy_const(dst);
    printf("memcpy_const = %.5s\n", dst);
    do_builtin_memcpy(dst);
    printf("builtin_memcpy = %.5s\n", dst);
    printf("strcmp_const = %d\n", do_strcmp_const());
    do_memset_const(dst);
    printf("memset_const done\n");
    return 0;
}
