// 6.3-1 重载决议实测：编译器到底选了谁
//
// 编译：clang++ -std=c++17 -O2 6.3-1-overload-resolution.cpp -o ovl && ./ovl
// 另可加 -DTRY_NULL 打开 g(NULL)（在 macOS clang 上是二义性，编译失败）
#include <cstddef>
#include <cstdio>

// ---------- ① 提升 vs 转换：float 实参选谁 ----------
void f(int)    { puts("  f(int)"); }
void f(double) { puts("  f(double)"); }

// ---------- ② 指针 vs 整数：0 / nullptr / NULL ----------
void g(int)         { puts("  g(int)"); }
void g(const char*) { puts("  g(const char*)"); }

// ---------- ③ 引用重载：左值 / 右值 / const ----------
void pick(int&)       { puts("  pick(int&)"); }
void pick(const int&) { puts("  pick(const int&)"); }

// ---------- ④ 顶层 const 不参与重载（声明两次合法，定义两次才报错）----------
void h(int x);
void h(const int x);   // 与上一行是同一个函数，重复"声明"没问题
void h(int x) { printf("  h(int) x=%d\n", x); }

int main() {
    puts("① 提升 vs 转换（f(int) / f(double)）");
    f(42);            // 精确匹配 int
    f(3.14);          // 精确匹配 double
    f(3.14f);         // float->double 是"提升"，优于 float->int 的"转换"
    f('a');           // char  -> int 提升
    f(true);          // bool  -> int 提升
    short s = 7;
    f(s);             // short -> int 提升

    puts("② 0 / nullptr / NULL 分别落到哪个重载");
    g(0);             // 精确 int
    g(nullptr);       // nullptr_t -> const char*（精确于 int）
#ifdef TRY_NULL
    g(NULL);          // macOS: NULL 是 long → 二义性，编译失败
#endif

    puts("③ 引用重载的左右值偏好");
    int x = 1;
    pick(x);          // 左值 → int& 优先
    pick(42);         // 右值 → 只能绑 const int&
    const int y = 2;
    pick(y);          // const 左值 → const int&

    puts("④ 顶层 const 被忽略");
    h(x);
    printf("  sizeof(NULL)=%zu  sizeof(nullptr)=%zu\n", sizeof(NULL), sizeof(nullptr));
    return 0;
}
