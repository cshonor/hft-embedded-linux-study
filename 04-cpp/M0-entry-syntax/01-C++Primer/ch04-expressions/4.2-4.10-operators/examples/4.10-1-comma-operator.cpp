// 4.10 逗号运算符：求值顺序、优先级陷阱、与函数实参逗号的区别
//
// 编译：clang++ -std=c++17 -O2 -Wall 4.10-1-comma-operator.cpp -o comma && ./comma
#include <cstdio>

static int side(const char* tag) { printf("    %s ", tag); return 0; }

static void two_args(int x, int y) { printf("    two_args(%d, %d)\n", x, y); }

static int pick2(int x, int y) { return x; }   // 只取第一个实参：用来暴露求值顺序

struct Comma { void operator,(int) const { printf("    Comma::operator,(int) 被调用\n"); } };

int main() {
    printf("① 逗号运算符：依次求值，值是最后一个\n");
    int a = (side("A"), side("B"), 3);
    printf("\n  → a = %d\n", a);
    int i = 0, j = 10;
    int b = (i = 5, j = 6, i + j);          // 前两个只是副作用，值是 i+j
    printf("  (i=5, j=6, i+j) = %d，且 i=%d j=%d\n", b, i, j);

    printf("② 函数实参里的逗号是分隔符，不是逗号运算符\n");
    two_args(1, 2);                          // 两个实参
    printf("  two_args((1, 2), 99) → ");
    two_args((1, 2), 99);                    // 加括号：第一个实参是逗号表达式 = 2

    printf("③ 下标里的逗号：arr[i, j] 等价 arr[j]\n");
    int arr[5] = {10, 20, 30, 40, 50};
    printf("  arr[1, 3] = %d  （不是 arr[1]，也不报错，是 arr[3]）\n", arr[1, 3]);

    printf("④ 逗号保证从左到右求值顺序（函数调用没有这个保证）\n");
    int k = 0, q = 0;
    int c = (q = ++k, q + 100);              // 先 ++k，再算 q + 100
    printf("  (q = ++k, q + 100) → c=%d, k=%d, q=%d\n", c, k, q);

    printf("⑤ for 里的逗号（最实用场景）\n");
    for (int m = 0, n = 4; m < n; ++m, --n)
        printf("    m=%d n=%d\n", m, n);

    printf("⑥ 优先级最低：逗号表达式作实参要加括号\n");
    int v = 1;
    printf("  v = %d → (v += 1, v * 10) = %d，最后 v=%d\n", v, (v += 1, v * 10), v);
    // 注意上面这行在 clang 下会触发 -Wunsequenced：函数实参的求值顺序没有保证

    printf("⑦ 逗号 vs 函数调用：只有逗号保证顺序\n");
    {
        int n = 0;
        auto bump = [&]() { return ++n; };
        int r1 = (bump(), bump());        // 保证先左后右：结果必是 2
        printf("  (bump(), bump())        = %d   ← 标准保证左→右，永远是 2\n", r1);

        n = 0;
        int r2 = pick2(bump(), bump());   // 实参顺序「未指定」：可能是 1 也可能是 2
        printf("  pick2(bump(), bump())   = %d   ← 顺序未指定，换编译器可能变\n", r2);
    }

    printf("⑧ 重载逗号后，顺序保证消失（变成函数调用）\n");
    {
        Comma c;
        c, 7;                             // 调用 Comma::operator,(int)
        printf("  重载后 a, b 是 operator,(a, b) 函数调用 → 顺序退化为「未指定」\n");
    }
    return 0;
}
