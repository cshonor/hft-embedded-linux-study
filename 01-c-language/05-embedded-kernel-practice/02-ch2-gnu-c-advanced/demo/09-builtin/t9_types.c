/* T9: type/alignment builtins */
#include <stdio.h>
#include <stdint.h>

/* __builtin_types_compatible_p: compile-time type check */
struct A { int x; };
struct B { int x; };

__attribute__((noinline))
void check_types(void)
{
    printf("types_compatible(int, int) = %d\n",
           __builtin_types_compatible_p(int, int));
    printf("types_compatible(int, unsigned) = %d\n",
           __builtin_types_compatible_p(int, unsigned));
    printf("types_compatible(int, long) = %d\n",
           __builtin_types_compatible_p(int, long));
    printf("types_compatible(struct A, struct B) = %d\n",
           __builtin_types_compatible_p(struct A, struct B));
    printf("types_compatible(char*, const char*) = %d\n",
           __builtin_types_compatible_p(char *, const char *));
    /* NOTE: char* and const char* are compatible! */
}

/* __builtin_assume_aligned: tell compiler pointer is aligned */
__attribute__((noinline))
int sum_aligned(int *p, int n)
{
    int *ap = __builtin_assume_aligned(p, 16);
    int s = 0;
    for (int i = 0; i < n; i++)
        s += ap[i];
    return s;
}

/* without assume_aligned */
__attribute__((noinline))
int sum_plain(int *p, int n)
{
    int s = 0;
    for (int i = 0; i < n; i++)
        s += p[i];
    return s;
}

/* offsetof-like with __builtin_offsetof */
#include <stddef.h>
struct S {
    char a;
    int b;
    char c;
};

__attribute__((noinline))
void show_offsets(void)
{
    printf("offsetof(a) = %zu\n", __builtin_offsetof(struct S, a));
    printf("offsetof(b) = %zu\n", __builtin_offsetof(struct S, b));
    printf("offsetof(c) = %zu\n", __builtin_offsetof(struct S, c));
}

int main(void)
{
    check_types();
    int arr[64] __attribute__((aligned(16)));
    for (int i = 0; i < 64; i++) arr[i] = i;
    printf("sum_aligned = %d\n", sum_aligned(arr, 64));
    printf("sum_plain = %d\n", sum_plain(arr, 64));
    show_offsets();
    return 0;
}
