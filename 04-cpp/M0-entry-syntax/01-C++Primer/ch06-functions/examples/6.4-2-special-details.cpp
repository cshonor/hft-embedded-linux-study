// 6.4-2-special-details.cpp —— 默认实参/内联/constexpr/调试辅助 的四个"真身"
//
// 编译运行（C++20，含 consteval / constinit 对照）：
//   clang++ -std=c++20 -Wall -O2 6.4-2-special-details.cpp -o a && ./a
// 对比 release（NDEBUG 会把 assert 整句删掉）：
//   clang++ -std=c++20 -O2 -DNDEBUG 6.4-2-special-details.cpp -o b && ./b

#include <cassert>
#include <cstdio>

// ---------- 1. 默认实参是"静态绑定"的：虚函数调用也不跟着变 ----------
struct Base {
    virtual void f(int x = 1) { printf("    Base::f  x=%d\n", x); }
    virtual ~Base() = default;
};
struct Derived : Base {
    void f(int x = 2) override { printf("    Derived::f  x=%d\n", x); }   // ⚠ 默认值不同
};

// ---------- 2. inline：真正的含义是 ODR 豁免，不是"请内联" ----------
inline int inline_add(int a, int b) { return a + b; }   // 可以放头文件，被多个 TU 包含
// 对比：普通函数在头文件里定义 → 链接期 duplicate symbol

// ---------- 3. constexpr / consteval / constinit ----------
constexpr int fib(int n) {                 // 编译期可求值，运行期也能用
    return n <= 1 ? n : fib(n - 1) + fib(n - 2);
}
#if __cplusplus >= 202002L
consteval int only_compile_time(int n) {   // C++20：强制编译期求值
    return n * 10;
}
constinit int g_init = 42;                 // C++20：强制静态初始化（不许动态初始化）
#endif

// ---------- 4. assert 会被 NDEBUG 整句删掉，连带副作用 ----------
static int g_counter = 0;

int main() {
    printf("=== 6.4-2 特殊用途语言特性：四个「真身」 ===\n\n");

    printf("[1] 默认实参是静态绑定（按指针/引用的静态类型取，不随虚派发）\n");
    Derived d;
    Base* pb = &d;
    d.f();          // 静态类型 Derived  -> 用 Derived 的默认值 2
    pb->f();        // 静态类型 Base     -> 用 Base 的默认值 1，但**调用的是 Derived::f**
    printf("    ⚠ 上面两行都进了 Derived::f，但 x 分别是 2 和 1\n");
    printf("    教训：虚函数的默认实参不要改，要改就用非虚接口（NVI）模式\n\n");

    printf("[2] inline 的语义是「允许多份定义」，不是「请内联」\n");
    printf("    inline_add(2,3) = %d\n", inline_add(2, 3));
    printf("    是否真的内联由优化器决定（-O0 下同样是 bl 调用）\n\n");

    printf("[3] constexpr / consteval / constinit\n");
    static_assert(fib(10) == 55, "constexpr 在编译期就算出来了");
    printf("    fib(10) = %d（编译期已算完，static_assert 通过）\n", fib(10));
    int runtime_n = 20;
    printf("    fib(%d) = %d（运行期调用同一个函数）\n", runtime_n, fib(runtime_n));
#if __cplusplus >= 202002L
    static_assert(only_compile_time(3) == 30, "consteval 必须编译期求值");
    printf("    consteval: only_compile_time(3) = %d\n", only_compile_time(3));
    printf("    constinit: g_init = %d（保证静态初始化，不会有初始化顺序问题）\n", g_init);
#endif
    printf("    区别：constexpr=可以编译期；consteval=必须编译期；constinit=必须静态初始化\n\n");

    printf("[4] assert：NDEBUG 下整句消失，副作用一起没\n");
    printf("    assert 前 g_counter = %d\n", g_counter);
    assert(++g_counter > 0);        // ⚠ 副作用写在 assert 里
    printf("    assert 后 g_counter = %d\n", g_counter);
    printf("    用 -DNDEBUG 重编：g_counter 保持 0 —— ++ 根本没执行\n");
    printf("    静态断言 static_assert 相反：编译期检查，任何构建都不会被删\n\n");

    printf("[5] 调试辅助宏（编译期常量，零运行时成本）\n");
    printf("    __func__ = %s\n", __func__);
    printf("    __FILE__ = %s\n", __FILE__);
    printf("    __LINE__ = %d\n", __LINE__);
    return 0;
}

// ---------------------------------------------------------------------------
// 汇编侧（-O2）：fib(10) 被折叠成常量 55，一行 mov 了事；
//   而 fib(runtime_n) 是真的递归调用 —— 同一个函数，两种命运，取决于实参是否编译期已知。
