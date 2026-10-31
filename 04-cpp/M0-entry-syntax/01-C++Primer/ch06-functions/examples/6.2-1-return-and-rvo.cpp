// 6.2 返回类型：RVO / NRVO / std::move 反而挡掉省略 / 返回引用作左值
//
//   clang++ -std=c++17 -O0 6.2-1-return-and-rvo.cpp -o r  && ./r
//   clang++ -std=c++17 -O0 -fno-elide-constructors 6.2-1-return-and-rvo.cpp -o r2 && ./r2
//
// 重点看加 -fno-elide-constructors 后的差异：
//   ① return T(n);        prvalue，C++17 强制省略 → 关掉开关也零拷贝
//   ② return t;           具名局部对象 NRVO，只是"允许"的优化 → 关掉后退化成 MOVE
//   ③ return std::move(t); 手动 move 挡掉 NRVO → 即使不关开关也是 MOVE

#include <cstdio>
#include <string>
#include <utility>

struct T {
    int v;
    explicit T(int x = 0) : v(x) { printf("    ctor(%d)\n", v); }
    T(const T& o) : v(o.v) { printf("    COPY(%d)\n", v); }
    T(T&& o) noexcept : v(o.v) { printf("    MOVE(%d)\n", v); }
    ~T() { printf("    dtor(%d)\n", v); }
};

__attribute__((noinline)) T make_prvalue(int n) { return T(n); }          // ①
__attribute__((noinline)) T make_named(int n) { T t(n); return t; }       // ②
__attribute__((noinline)) T make_moved(int n) { T t(n); return std::move(t); }  // ③

int main() {
    printf("① return T(n);        —— prvalue，C++17 强制省略\n");
    { T a = make_prvalue(1); printf("    → a=%d\n", a.v); }

    printf("\n② return t;           —— 具名局部对象（NRVO，允许但非保证）\n");
    { T b = make_named(2); printf("    → b=%d\n", b.v); }

    printf("\n③ return std::move(t); —— 手动 move，反而挡掉 NRVO\n");
    { T c = make_moved(3); printf("    → c=%d\n", c.v); }

    printf("\n④ 返回引用作左值（v[i] = x / os << a << b 的原理）\n");
    static std::string g = "abc";
    auto front = [](std::string& s) -> char& { return s[0]; };
    front(g) = 'X';
    printf("    g = %s\n", g.c_str());

    printf("\n--- 对比方法：加 -fno-elide-constructors 重跑，看 ①是否仍零拷贝、②是否变 MOVE ---\n");
    return 0;
}
