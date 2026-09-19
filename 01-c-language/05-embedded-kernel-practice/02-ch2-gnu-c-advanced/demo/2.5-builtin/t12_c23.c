/* T12: C23 standard builtins (__builtin_stdc_*) */
#include <stdio.h>
#include <stdint.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#  define HAS_C23 1
#else
#  define HAS_C23 0
#endif

int main(void)
{
    printf("C23 (__STDC_VERSION__ = %ldL, HAS_C23=%d)\n",
           (long)__STDC_VERSION__, HAS_C23);

    /* C23 standardizes: __builtin_stdc_memcmp, __builtin_stdc_memmove,
       __builtin_stdc_memset, __builtin_stdc_strlen, __builtin_stdc_strcmp,
       __builtin_stdc_strcpy, __builtin_stdc_strchr, etc.
       These are NOT the same as __builtin_memset etc.
       They are the freestanding-safe subset of libc that C23
       requires even in freestanding mode. */

    /* GCC 14+ and Clang 19+ support these.
       Let's check which compiler we have and whether they work. */
    printf("GNUC=%d CLANG=%d\n", (int)__GNUC__, (int)__has_feature /* dummy */ (0));

    /* try __builtin_stdc_memset */
    char buf[16] = {0};
    __builtin_memset(buf, 'X', 8);  /* old builtin still works */
    printf("builtin_memset: %.8s\n", buf);

    /* Does the C23 variant compile? */
    /* __builtin_stdc_memset(buf, 'Y', 4); */ /* uncomment to test */

    return 0;
}
