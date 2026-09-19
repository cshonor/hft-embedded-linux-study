/* T10: -fno-builtin effect on libc folding */
#include <stdio.h>
#include <string.h>

/* with -fno-builtin, strlen("hello") won't fold to 5 */
__attribute__((noinline))
size_t my_strlen_const(void)
{
    return strlen("hello");
}

/* with -fno-builtin-strlen, only strlen is disabled */
__attribute__((noinline))
int my_strcmp_const(void)
{
    return strcmp("abc", "abd");
}

/* __builtin_strlen is NOT affected by -fno-builtin */
__attribute__((noinline))
size_t my_builtin_strlen(void)
{
    return __builtin_strlen("hello");
}

int main(void)
{
    printf("strlen(hello) = %zu\n", my_strlen_const());
    printf("strcmp(abc,abd) = %d\n", my_strcmp_const());
    printf("__builtin_strlen(hello) = %zu\n", my_builtin_strlen());
    return 0;
}
