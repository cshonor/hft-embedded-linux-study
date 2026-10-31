// 6.5-3-signature-match.cpp —— 什么才算「同一个函数类型」（函数签名匹配规则全表）
//
// 编译运行：
//   clang++ -std=c++17 -Wall -O2 6.5-3-signature-match.cpp -o a && ./a
//
// 单独复现某个编译错误（下面表格里标 ❌ 的用例）：
//   clang++ -std=c++17 -DCASE=1 -fsyntax-only 6.5-3-signature-match.cpp
//   CASE 取值见文件末尾 switch 注释（1..9）。
//
// 结论先看这张表（本机 Apple clang 23.1.0 / arm64 / -std=c++17）：
//
//   规则                                   是否同一类型   依据
//   -------------------------------------  ------------  --------------------------------
//   参数名不同                              ✅ 同一       [dcl.fct] 参数名不是类型的一部分
//   顶层 const（void(const int)）           ✅ 同一       顶层 cv 在参数类型调整时丢弃
//   数组参数 int[10] → int*                 ✅ 同一       参数类型调整（退化）
//   函数参数 void()  → void(*)()            ✅ 同一       同上
//   引用 vs 值（int& vs int）               ❌ 不同       引用本身不退化
//   底层 const（const int* vs int*）        ❌ 不同       指针指向的 cv 是类型的一部分
//   noexcept（C++17 起）                    ❌ 不同（单向） noexcept 是函数类型的一部分
//   返回类型                                ❌ 不同       函数指针不做返回类型协变
//   成员函数的 const / 引用限定             ❌ 不同       隐式对象参数是签名的一部分

#include <cstdio>
#include <type_traits>
#include <functional>

// ---------- 1. 参数名无关 ----------
using F1 = int(int, double);
using F2 = int(int a, double b);          // 只是起了名字
static_assert(std::is_same<F1, F2>::value, "参数名不是函数类型的一部分");

// ---------- 2. 顶层 const 被丢弃 ----------
static_assert(std::is_same<void(int), void(const int)>::value,
              "形参的顶层 const 在「参数类型调整」时被丢弃");
// 因此下面两个是**同一个函数**，写两遍定义会 redefinition：
//   void k(int);
//   void k(const int);      // ✅ 合法重复声明（同一函数）
//   void k(int) {}          // 再写一份定义 → error: redefinition of 'k'

// ---------- 3. 数组 / 函数参数退化 ----------
static_assert(std::is_same<void(int[10]), void(int*)>::value, "数组参数退化为指针");
static_assert(std::is_same<void(int[3]),  void(int[10])>::value, "维度也被丢弃，不是类型的一部分");
static_assert(std::is_same<void(void()),  void(void(*)())>::value, "函数参数退化为函数指针");

// ---------- 4. 引用不退化 ----------
static_assert(!std::is_same<void(int), void(int&)>::value, "引用 vs 值是不同类型");
static_assert(!std::is_same<void(int), void(const int&)>::value, "const 引用也是不同类型");

// ---------- 5. 底层 const 保留 ----------
static_assert(!std::is_same<void(const char*), void(char*)>::value, "指向的 const 是类型的一部分");
static_assert(std::is_same<void(char* const), void(char*)>::value, "指针自身的 const 是顶层 const，被丢弃");

// ---------- 6. noexcept 是类型的一部分（C++17 起），且单向兼容 ----------
#if __cplusplus >= 201703L
static_assert(!std::is_same<void(), void() noexcept>::value, "C++17 起 noexcept 进类型");
#endif
void f_noexcept() noexcept {}
void f_maythrow() {}
void (*p_plain)()            = f_noexcept;   // ✅ noexcept → 可抛：允许（缩小承诺）
// void (*p_nx)() noexcept    = f_maythrow;   // ❌ 见 CASE=1

// ---------- 7. 返回类型：函数指针不做协变 ----------
struct Base { int b; };
struct Derived : Base { int d; };
Derived* mk() { static Derived d; return &d; }
// Base* (*p_cov)() = &mk;                    // ❌ 见 CASE=2（返回类型必须完全一致）
std::function<Base*()> fn_cov = &mk;          // ✅ std::function 的调用运算符允许协变返回

// ---------- 8. 成员函数：const / 引用限定也算签名的一部分 ----------
struct Widget {
    int get()       { return 1; }
    int get() const { return 2; }             // ✅ 合法重载：隐式对象参数的 const 不同
};
static_assert(!std::is_same<int (Widget::*)(), int (Widget::*)() const>::value,
              "const 限定是成员函数指针类型的一部分");

// ---------- 9. 重载取地址必须给出目标类型 ----------
void h(int)  {}
void h(double) {}
int  (*pi)(int)          = nullptr;
void (*pv)(int)          = &h;                // ✅ 目标类型挑出了 void h(int)
auto pick = static_cast<void (*)(double)>(&h); // ✅ 另一种写法
// auto bad = &h;                             // ❌ 见 CASE=3

// ---------- 10. 模板函数取地址要先实例化 ----------
template <class T> T twice(T x) { return x + x; }
int (*pt)(int) = &twice<int>;                 // ✅ 显式指定模板实参
// int (*pt2)(int) = &twice;                  // ❌ 见 CASE=4（除非能从目标类型推导，见下）
void take(int (*f)(int)) {}
// 形参是具体函数指针类型时，模板实参可以从目标类型推导出来：
//   take(&twice);                            // ✅ 见 main()，推出 T=int

// ---------- 11. lambda 到函数指针：只有无捕获的能转 ----------
void (*p_lam)(int) = [](int x) { (void)x; };  // ✅ 无捕获 lambda 有到函数指针的转换
int g_global = 7;                             // 全局：静态存储期
// void (*p_lam_g)(int) = [g_global](int x){ (void)x; };  // ❌ 见 CASE=6
//   ↑ 注意这条报错跟「能不能转函数指针」无关，是更前面的一层规则：
//     'g_global' cannot be captured because it does not have automatic storage duration
//     全局变量/静态变量压根不能进捕获列表——它们不需要捕获，lambda 里直接读就行。

// ---------- 12. 变参 / 默认实参都不进类型 ----------
static_assert(std::is_same<void(int, ...), void(int, ...)>::value, "");
void with_default(int, int = 0);
void (*p_def)(int, int) = &with_default;      // ✅ 默认实参不是类型的一部分，调用点才填
void with_default(int, int) {}

int main() {
    printf("=== 6.5-3 函数签名匹配规则（clang %d.%d.%d, c++%ld）===\n\n",
           __clang_major__, __clang_minor__, __clang_patchlevel__, __cplusplus);

    printf("规则                                   同类型?   实测/依据\n");
    printf("-------------------------------------  --------  -------------------------------\n");
    printf("%-37s  %-8s  参数名非类型成分\n", "int(int,double) vs int(int a,double b)",
           std::is_same<F1, F2>::value ? "✅ 是" : "❌ 否");
    printf("%-37s  %-8s  顶层 const 被丢弃\n", "void(int) vs void(const int)",
           std::is_same<void(int), void(const int)>::value ? "✅ 是" : "❌ 否");
    printf("%-37s  %-8s  数组参数退化\n", "void(int[10]) vs void(int*)",
           std::is_same<void(int[10]), void(int*)>::value ? "✅ 是" : "❌ 否");
    printf("%-37s  %-8s  维度也被丢弃\n", "void(int[3]) vs void(int[10])",
           std::is_same<void(int[3]), void(int[10])>::value ? "✅ 是" : "❌ 否");
    printf("%-37s  %-8s  函数参数退化\n", "void(void()) vs void(void(*)())",
           std::is_same<void(void()), void(void(*)())>::value ? "✅ 是" : "❌ 否");
    printf("%-37s  %-8s  引用不退化\n", "void(int) vs void(int&)",
           std::is_same<void(int), void(int&)>::value ? "✅ 是" : "❌ 否");
    printf("%-37s  %-8s  底层 const 保留\n", "void(const char*) vs void(char*)",
           std::is_same<void(const char*), void(char*)>::value ? "✅ 是" : "❌ 否");
    printf("%-37s  %-8s  指针自身 const 被丢弃\n", "void(char* const) vs void(char*)",
           std::is_same<void(char* const), void(char*)>::value ? "✅ 是" : "❌ 否");
    printf("%-37s  %-8s  C++17 起进类型\n", "void() vs void() noexcept",
           std::is_same<void(), void() noexcept>::value ? "✅ 是" : "❌ 否");
    printf("%-37s  %-8s  const 限定进类型\n", "int(W::*)() vs int(W::*)() const",
           std::is_same<int (Widget::*)(), int (Widget::*)() const>::value ? "✅ 是" : "❌ 否");

    printf("\n=== 运行时行为 ===\n");
    Widget w;
    const Widget cw;
    int (Widget::*pm)()       = &Widget::get;
    int (Widget::*pmc)() const = &Widget::get;
    printf("Widget::get()      非 const 对象 -> %d\n", (w.*pm)());
    printf("Widget::get() const const  对象 -> %d\n", (cw.*pmc)());
    printf("重载取地址 void h(int)  -> %p\n", (void*)pv);
    printf("重载取地址 void h(double)-> %p\n", (void*)pick);
    printf("模板实例 twice<int>(21) = %d\n", pt(21));
    printf("无捕获 lambda 转函数指针 -> %p\n", (void*)p_lam);
    printf("std::function 协变返回 Base* = %p（Derived* 转 Base* 合法）\n", (void*)fn_cov());
    printf("默认实参不是类型成分：p_def(5) 调用后不报错 = %p\n", (void*)p_def);
    take(&twice);                             // ✅ 目标类型 int(*)(int) → 推出 T=int
    printf("模板实参从目标类型推导：take(&twice) 编译通过\n");

    // lambda → 函数指针 / std::function：只看有没有捕获列表
    int loc = 7;                              // 局部：automatic storage，可以捕获
    void (*fp_nocap)(int) = [](int x) { printf("  无捕获 lambda 被调用 x=%d\n", x); };
    std::function<void(int)> sf_cap = [loc](int x) {
        printf("  有捕获 lambda 被调用 loc=%d x=%d\n", loc, x);
    };
    // void (*fp_cap)(int) = [loc](int x){ ... };   // ❌ 见 CASE=5
    fp_nocap(1);
    sf_cap(2);
    // 全局变量不用捕获，直接用；写了 [g_global] 反而报错（CASE=6）
    auto use_global = [] { return g_global; };   // ✅ [] 里什么都不写也能读全局
    printf("  无捕获 lambda 读全局变量 g_global = %d\n", use_global());
    printf("  （无捕获 lambda 才装得进 8 字节：sizeof=%zu）\n", sizeof(fp_nocap));

    printf("\n=== sizeof 对照 ===\n");
    printf("sizeof(void(*)(int))        = %zu\n", sizeof(void (*)(int)));
    printf("sizeof(int(Widget::*)())    = %zu   ← 成员函数指针，装得下虚表偏移/this 调整\n",
           sizeof(int (Widget::*)()));
    printf("sizeof(std::function<void(int)>) = %zu\n", sizeof(std::function<void(int)>));
    return 0;
}

// ---------------------------------------------------------------------------
// 下面这些是「签名看着像、其实是不同类型」的负例，取消注释即可看到编译器原话。
// 默认全部不编译，避免 examples 整体编译失败。
//
// CASE=1: void (*p_nx)() noexcept = f_maythrow;
//   error: assigning to 'void (*)() noexcept' from incompatible type 'void ()':
//          different exception specifications
//
// CASE=2: Base* (*p_cov)() = &mk;
//   error: cannot initialize a variable of type 'Base *(*)()' with an rvalue of
//          type 'Derived *(*)()': different return type ('Base *' vs 'Derived *')
//
// CASE=3: auto bad = &h;
//   error: variable 'bad' declared with deduced type 'auto' cannot be deduced
//          from an overloaded function / initializer is an overloaded function
//
// CASE=4: int (*pt2)(int) = &twice;   （当没有可推导的目标类型时）
//   error: no matching function for call ... / cannot initialize ... <overloaded
//          function type>（模板未实例化，取不到地址）
//
// CASE=5: void (*p_lam2)(int) = [loc](int x){ (void)x; };   // loc 是局部变量
//   error: no viable conversion from '(lambda at ...)' to 'void (*)(int)'
//   → 有捕获的 lambda 是「带成员的对象」，8 字节指针装不下状态，必须 std::function
//
// CASE=6: void (*p_lam_g)(int) = [g_global](int x){ (void)x; };   // g_global 是全局变量
//   error: 'g_global' cannot be captured because it does not have automatic storage duration
//   → 这条和「转函数指针」无关：静态存储期的变量压根不许进捕获列表，直接用即可
//
// 记忆口诀：**名字不算、顶层 const 不算、数组/函数的壳不算；引用、底层 const、
// noexcept、返回类型、成员的 const 限定都算。**
