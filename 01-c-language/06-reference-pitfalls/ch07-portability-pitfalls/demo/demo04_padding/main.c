#include <stddef.h>
#include <stdio.h>

struct Test {
    char a;
    int b;
};

struct Packed {
    char a;
    int b;
} __attribute__((packed));

int main(void)
{
    printf("struct Test: sizeof=%zu offsetof(b)=%zu\n",
           sizeof(struct Test), offsetof(struct Test, b));
    printf("struct Packed: sizeof=%zu offsetof(b)=%zu (may trap on strict-align CPUs)\n",
           sizeof(struct Packed), offsetof(struct Packed, b));

    return 0;
}
