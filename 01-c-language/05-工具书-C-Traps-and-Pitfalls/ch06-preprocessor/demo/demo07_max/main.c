#include <stdio.h>
#include <stdint.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static inline int max_int(int a, int b)
{
    return a > b ? a : b;
}

int main(void)
{
    int a = 3, b = 7;
    printf("MAX(a,b)=%d max_int(a,b)=%d\n", MAX(a, b), max_int(a, b));

    /* macro accepts mixed types — compiles, nonsense at runtime */
    printf("MAX(\"abc\", 123)=%ld (pointer vs int, meaningless)\n",
           (long)(uintptr_t)MAX("abc", 123));

    return 0;
}
