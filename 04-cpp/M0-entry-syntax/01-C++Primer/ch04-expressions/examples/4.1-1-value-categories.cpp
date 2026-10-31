// 4.1-1-value-categories.cpp —— 表达式的两个属性：类型 + 值类别（lvalue / xvalue / prvalue）
//
// 编译运行：
//   clang++ -std=c++17 -Wall -O2 4.1-1-value-categories.cpp -o a && ./a
//
// 为什么要在意值类别：它决定 ① 能不能取地址 ② 能绑给哪种引用 ③ 走拷贝还是移动。
// 重载决议里"左值走 const T&、右值走 T&&"分派的就是它。

#include <cstdio>
#include <string>
#include <utility>
#include <type_traits>

// ---------- 用重载探测值类别：编译器替我们分类 ----------
const char* probe(int&)        { return "lvalue     (能绑 int&)"; }
const char* probe(const int&)  { return "lvalue/xvalue/prvalue (都能绑 const int&)"; }
const char* probe(int&&)       { return "xvalue/prvalue (能绑 int&&)"; }

// ---------- 返回值类别由返回类型决定 ----------
int   g_val();                 // 返回非引用 → 调用表达式是 prvalue
int&  g_lref();                // 返回左值引用 → lvalue
int&& g_rref();                // 返回右值引用 → xvalue
static int g_storage = 0;
int   g_val()  { return 1; }
int&  g_lref() { return g_storage; }
int&& g_rref() { return std::move(g_storage); }

int main() {
    int x = 10;

    printf("=== 4.1-1 表达式的值类别（clang %d.%d, c++%ld）===\n\n",
           __clang_major__, __clang_minor__, __cplusplus);

    printf("[1] 用重载探测：编译器自己会分类\n");
    printf("    变量 x            -> %s\n", probe(x));
    printf("    字面量 42         -> %s\n", probe(42));
    printf("    x + 1             -> %s\n", probe(x + 1));
    printf("    std::move(x)      -> %s\n", probe(std::move(x)));
    printf("    ⚠ const int& 版本能接住所有类别，所以探测顺序很重要：\n");
    printf("      上面 int&& 版本更特化，才把 prvalue/xvalue 挑了出来\n\n");

    printf("[2] 函数返回值：返回类型决定值类别\n");
    printf("    g_val()  返回 int   -> %s\n", probe(g_val()));
    printf("    g_lref() 返回 int&  -> %s\n", probe(g_lref()));
    printf("    g_rref() 返回 int&& -> %s\n", probe(g_rref()));
    printf("    这就是为什么返回 int& 的函数可以写在赋值左边：g_lref() = 99;\n");
    g_lref() = 99;
    printf("    实测：g_lref() = 99 之后 g_storage = %d\n\n", g_storage);

    printf("[3] decltype 的两条规则（最容易记混的一对）\n");
    printf("    decltype(x)       是 int    -> %s\n",
           std::is_same<decltype(x), int>::value ? "int ✅" : "?");
    printf("    decltype((x))     是 int&   -> %s\n",
           std::is_same<decltype((x)), int&>::value ? "int& ✅" : "?");
    printf("    规则：对**不带括号**的变量名给出声明类型；加了括号就当普通表达式，\n");
    printf("          而「变量」作为表达式是左值 → 得到 T&\n\n");

    printf("[4] 字符串字面量是左值（数组类型，不是指针）\n");
    printf("    sizeof(\"abc\")     = %zu   ← 数组大小 4（含 \\0），不是指针大小 8\n",
           sizeof("abc"));
    printf("    &\"abc\"            = %p   ← 能取地址 = 它是左值\n", (const void*)&"abc");
    printf("    decltype(\"abc\")   = const char(&)[4] -> %s\n",
           std::is_same<decltype("abc"), const char(&)[4]>::value ? "✅" : "?");
    printf("    ⚠ 注意是**引用**：字符串字面量是左值，decltype 对左值表达式给出 T&\n");
    printf("      想拿到数组类型本身要写 std::remove_reference_t<decltype(\"abc\")>\n");
    printf("    它存在只读段，生命周期是整个程序，所以返回它的指针是安全的\n\n");

    printf("[5] 值类别决定走拷贝还是移动（这正是 move 语义的地基）\n");
    std::string s1 = "hello";
    std::string s2 = s1;                 // 左值 → 拷贝
    std::string s3 = std::move(s1);      // xvalue → 移动
    printf("    s1 = \"%s\"（被移动走，通常变空）\n", s1.c_str());
    printf("    s2 = \"%s\"（拷贝得到）\n", s2.c_str());
    printf("    s3 = \"%s\"（移动得到）\n", s3.c_str());
    printf("    sizeof(std::string) = %zu（SSO 小缓冲，短字符串不堆分配）\n\n",
           sizeof(std::string));

    printf("[6] 一句话总结\n");
    printf("    有名字、能取地址        -> lvalue\n");
    printf("    将亡、可移动            -> xvalue（std::move/T&& 返回/成员访问 .m 于右值）\n");
    printf("    纯结果、没身份          -> prvalue（字面量、x+1、返回非引用的调用）\n");
    return 0;
}

// ---------------------------------------------------------------------------
// 分类速查（C++11 起的五类，实际用得最多的是下面三个）：
//   glvalue（有身份）= lvalue + xvalue      ← 能取地址/能绑左值引用
//   rvalue（可移动） = xvalue + prvalue     ← 能绑 T&&，能触发移动
//   lvalue  : 变量名、返回 T& 的调用、字符串字面量、*p、前置 ++ 的结果
//   xvalue  : std::move(x)、返回 T&& 的调用、static_cast<T&&>(x)
//   prvalue : 字面量（除字符串）、x+1、返回 T 的调用、后置 ++ 的结果、&x 取地址
