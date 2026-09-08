#include <stddef.h>
#include <stdio.h>

struct Test1 {
    char a;
    int b;
    short c;
};

struct Test2 {
    char a;
    int b;
};

struct Test2_packed {
    char a;
    int b;
} __attribute__((packed));

static void dump_layout(const char *name, size_t size)
{
    printf("\n=== %s sizeof=%zu ===\n", name, size);
}

int main(void)
{
    dump_layout("Test1 (a+int+short)", sizeof(struct Test1));
    printf("  offsetof(a)=%zu offsetof(b)=%zu offsetof(c)=%zu\n",
           offsetof(struct Test1, a),
           offsetof(struct Test1, b),
           offsetof(struct Test1, c));

    dump_layout("Test2 (a+int)", sizeof(struct Test2));
    printf("  offsetof(a)=%zu offsetof(b)=%zu\n",
           offsetof(struct Test2, a),
           offsetof(struct Test2, b));

    dump_layout("Test2_packed", sizeof(struct Test2_packed));
    printf("  offsetof(a)=%zu offsetof(b)=%zu\n",
           offsetof(struct Test2_packed, a),
           offsetof(struct Test2_packed, b));

    printf("\n分步计算 Test1（本机 GCC %zuB）:\n", sizeof(struct Test1));
    printf("  a@0 +1 → b 需 4 对齐 → 填 3B → b@4 +4 → c@8 +2 → 整体 4 对齐 → 总 %zuB\n",
           sizeof(struct Test1));
    printf("  （若含 double/int64_t 等 8 对齐成员，尾部可能再填至 16B）\n");

    return 0;
}
