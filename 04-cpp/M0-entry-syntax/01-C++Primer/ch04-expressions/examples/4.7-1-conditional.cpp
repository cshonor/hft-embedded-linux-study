// 4.7-1 条件运算符 ?: 的实测
//
// 编译：
//   clang++ -std=c++17 -O2 examples/4.7-1-conditional.cpp -o cond && ./cond

#include <cstdio>
#include <string>
#include <type_traits>

__attribute__((noinline)) static int left() { printf("        左分支被求值\n"); return 1; }
__attribute__((noinline)) static int right() { printf("        右分支被求值\n"); return 2; }

int main() {
    printf("=== 4.7-1 条件运算符实测 ===\n\n");

    // ---------- [1] 只求值被选中的那个分支 ----------
    printf("[1] 只求值选中的分支（等价于 if-else 的表达式形式）\n");
    printf("    cond ? left() : right()，cond = true：\n");
    int r1 = true ? left() : right();
    printf("        结果 = %d\n", r1);
    printf("    cond ? left() : right()，cond = false：\n");
    int r2 = false ? left() : right();
    printf("        结果 = %d\n", r2);
    printf("    ⚠ 这是 ?: 与函数调用的本质区别：f(left(), right()) 两个实参都求值\n\n");

    // ---------- [2] 两个分支必须能转成同一个「公共类型」 ----------
    printf("[2] 公共类型（composite type）\n");
    int i = 1;
    double d = 2.5;
    auto r3 = true ? i : d;               // int 转成 double
    printf("    true ? int : double      -> 结果 %g，类型大小 %zu 字节（= double）\n",
           r3, sizeof(r3));
    auto r4 = true ? i : (short)3;        // short 提升成 int
    printf("    true ? int : short       -> 类型大小 %zu 字节（= int，整型提升）\n", sizeof(r4));
    auto r5 = true ? "abc" : nullptr;     // const char* 与 nullptr_t -> const char*
    printf("    true ? \"abc\" : nullptr  -> 类型大小 %zu 字节（= 指针）\n", sizeof(r5));
    printf("    ⚠ 两个分支类型不兼容时编译报错：true ? 1 : std::string(\"a\")\n\n");

    // ---------- [3] C++ 独有：?: 的结果可以是左值 ----------
    printf("[3] ?: 结果的左值性（C++ 有，C 没有）\n");
    int x = 10, y = 20;
    (true ? x : y) = 99;                  // ✅ C++：两个分支都是同类型左值时结果是左值
    printf("    (true ? x : y) = 99  ->  x=%d y=%d   // C++ 合法；同样代码在 C 里报错\n", x, y);
    printf("    规则：两个操作数都是同类型**左值**时，?: 的结果是左值，可以赋值/取地址\n\n");

    // ---------- [4] 右结合：嵌套 ?: 从右往左分组 ----------
    printf("[4] 右结合\n");
    int score = 85;
    // 等价于 score >= 90 ? 'A' : (score >= 60 ? 'B' : 'C')
    char grade = score >= 90 ? 'A' : score >= 60 ? 'B' : 'C';
    printf("    score=85 -> %c   （分组：>=90 ? 'A' : (>=60 ? 'B' : 'C')）\n", grade);
    printf("    ⚠ 嵌套超过两层就该换成 if-else 或查表，可读性优先\n\n");

    // ---------- [5] 优先级倒数第三：cout << cond ? a : b 是经典 bug ----------
    printf("[5] 优先级陷阱\n");
    int cond = 0, A = 111, B = 222;
    printf("    cond ? A : B          = %d\n", cond ? A : B);
    printf("    坑位原型：std::cout << cond ? A : B;\n");
    printf("        实际分组是 (std::cout << cond) ? A : B —— 输出的是 cond，不是 A/B\n");
    printf("    正确写法：std::cout << (cond ? A : B);\n");
    printf("    同理：a = cond ? A : B 没问题（?: 高于赋值）；但 a + cond ? A : B 会变成 (a+cond) ? A : B\n\n");

    // ---------- [6] ?: vs if-else：能生成更好的代码 ----------
    printf("[6] ?: 与 if-else 的等价性\n");
    printf("    ?: 是**表达式**，可以出现在任何需要值的地方（初始化、实参、返回）：\n");
    printf("        int v = cond ? A : B;         // if-else 得写成 4 行 + 先声明 v\n");
    printf("        return cond ? A : B;\n");
    printf("        constexpr int N = cond ? 1 : 2;   // 还能用于编译期常量\n");
    constexpr int N = true ? 1 : 2;
    printf("        constexpr 实测：N = %d\n", N);
    printf("    ⚠ 但两个分支有副作用时，?: 的可读性不如 if-else，别硬塞\n\n");

    // ---------- [7] 速查 ----------
    printf("[7] 速查\n");
    printf("    • 优先级仅高于赋值和逗号 —— 涉及 << + 等一律加括号\n");
    printf("    • 右结合：a ? b : c ? d : e 等价于 a ? b : (c ? d : e)\n");
    printf("    • 只求值被选中的分支（唯一「短路」的三目结构）\n");
    printf("    • 两个分支会先转成公共类型，类型不同时留意隐式转换\n");
    printf("    • C++ 中同类型左值分支的结果是左值（C 里不是）\n");

    return 0;
}
