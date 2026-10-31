// 6.4-1 默认实参 / inline / constexpr / assert 的编译期行为
//
// 编译（默认）：      clang++ -std=c++17 -O2 6.4-1-special-features.cpp -o sp && ./sp
// 编译（关断言）：    clang++ -std=c++17 -O2 -DNDEBUG 6.4-1-special-features.cpp -o spn && ./spn
// 看汇编：            clang++ -std=c++17 -O2 -S 6.4-1-special-features.cpp -o sp.s
#include <cassert>
#include <cstdio>

// ---- 默认实参：只在"声明"处出现一次，由调用点补上 ----
int add(int a, int b = 10) { return a + b; }

// ---- inline：建议内联 + ODR 豁免（可放头文件）----
inline int square(int x) { return x * x; }

// ---- constexpr：实参是常量表达式时在编译期求值 ----
constexpr int fact(int n) { return n <= 1 ? 1 : n * fact(n - 1); }

// ---- assert 里的副作用在 NDEBUG 下会整句消失 ----
int g_bump = 0;
int bump() { ++g_bump; return g_bump; }

int main(int argc, char**) {
    printf("① 默认实参\n");
    printf("  add(5)     = %d\n", add(5));      // 调用点补 10
    printf("  add(5, 20) = %d\n", add(5, 20));

    printf("② inline（-O2 下展开成一条 mul，-O0 下仍会 bl 调用）\n");
    volatile int vx = argc + 4;
    printf("  square(%d) = %d\n", vx, square(vx));

    printf("③ constexpr\n");
    constexpr int f5 = fact(5);          // 编译期：汇编里是 mov w0, #120
    int arr[fact(3)];                    // 编译期常量作数组长度 = 6
    printf("  fact(5)      = %d\n", f5);
    printf("  sizeof(arr)  = %zu (%zu 个 int)\n", sizeof(arr), sizeof(arr) / sizeof(int));
    int n = argc + 5;
    printf("  fact(n=%d)   = %d   ← 运行期实参，退化成普通调用\n", n, fact(n));

    printf("④ assert / NDEBUG\n");
    assert(bump() > 0);                  // 副作用：只有 assert 生效时才执行
    printf("  g_bump = %d  %s\n", g_bump,
#ifdef NDEBUG
           "(NDEBUG：assert 被移除，副作用也没了)");
#else
           "(断言生效：副作用执行了)");
#endif
    return 0;
}
