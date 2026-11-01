// 4.11-2 类型转换的高频陷阱（隐式转换 + 窄化 + 整型提升）
//
// 编译：
//   clang++ -std=c++17 -O2 -Wall examples/4.11-2-conversion-traps.cpp -o traps && ./traps
//   （会保留 2 处有意演示的 warning：-Wsign-compare、-Wliteral-conversion）

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

int main() {
    printf("=== 4.11-2 类型转换陷阱实测 ===\n\n");

    // ---------- [1] 有符号 / 无符号混用：负数变大正数 ----------
    printf("[1] 有符号 vs 无符号\n");
    int si = -1;
    unsigned ui = 1;
    printf("    si = -1, ui = 1\n");
    printf("    si < ui            = %d   // ❗si 被转成 unsigned = %u\n",
           (si < ui) ? 1 : 0, (unsigned)si);
    printf("    si < (int)ui       = %d   // 显式转换后符合直觉\n", (si < (int)ui) ? 1 : 0);
    printf("    ⚠ -Wsign-compare 就是为这类代码准备的，别关掉它\n\n");

    // ---------- [2] size_t 下溢：倒序循环的经典越界 ----------
    printf("[2] size_t 下溢\n");
    printf("    std::string::npos = %zu\n", std::string::npos);
    std::string s = "abc";
    printf("    正向查找 \"z\"：pos = %zu，判空必须写 pos == npos\n", s.find('z'));
    printf("    for (size_t i = v.size()-1; i >= 0; --i) 在 v 为空时：\n");
    std::vector<int> empty;
    printf("        empty.size()-1 = %zu  —— 变成巨大值，i >= 0 恒真，直接越界\n",
           empty.size() - 1);
    printf("    倒序正确写法：for (size_t i = v.size(); i-- > 0;) 或用反向迭代器\n\n");

    // ---------- [3] 整型提升：char/short 先变 int 再算 ----------
    printf("[3] 整型提升\n");
    char c1 = 100, c2 = 100;
    auto sum_c = c1 + c2;                 // char + char -> int
    printf("    char(100) + char(100) = %d, 结果类型是 int？%s\n",
           sum_c, (sizeof(sum_c) == sizeof(int)) ? "是" : "否");
    printf("    ❗如果把 200 存回 char：%d  (char 在本机%s有符号)\n",
           (char)(c1 + c2), (char)-1 < 0 ? "" : "没");
    unsigned char uc = 200;
    printf("    unsigned char uc = 200;  uc 参与运算 -> %d（提升成 int，不会变负）\n", uc + uc);
    short sh = 30000;
    printf("    short(30000) * 2 = %d  —— 提升后不会溢出；不提升才会\n", sh * 2);
    printf("    ⚠ printf(\"%%c\", c1+c2) 之类必须自己转成 int，varargs 不做类型检查\n\n");

    // ---------- [4] 浮点转整型：截断 + 越界是 UB ----------
    printf("[4] 浮点 -> 整型\n");
    printf("    (int)3.99   = %d   (向零截断)\n", (int)3.99);
    printf("    (int)-3.99  = %d   (不是 -4)\n", (int)-3.99);
    printf("    (int)(3.99+0.5) = %d  —— 想四舍五入要自己加 0.5（正数）\n", (int)(3.99 + 0.5));
    printf("    ⚠ 超出目标范围的浮点转整型是**未定义行为**（不是实现定义）：\n");
    printf("      (int)1e30 / (int)NAN / (int)INFINITY 的结果都不保证\n");
    printf("      实测 (int)1e30 = %d  （ARM64 上通常饱和到 INT_MIN 或别的，别依赖）\n", (int)1e30);
    printf("      正确做法：先比较范围再转，或用 std::clamp / 饱和转换函数\n\n");

    // ---------- [5] 窄化转换：{} 报错，() 静默 ----------
    printf("[5] 窄化转换（narrowing）\n");
    int big = 300;
    char c_from_int(big);                 // 圆括号：静默截断
    printf("    char c(big=300)      = %d   // 静默截断，编译无警告\n", c_from_int);
    // char c_brace{big};                 // ❌ 花括号：error: constant expression evaluates to 300
    printf("    char c{big=300}      -> 编译错误（花括号禁止窄化）\n");
    char c_ok{static_cast<char>(big)};    // ✅ 写明意图后可以用花括号
    printf("    char c{static_cast<char>(big)} = %d   // 显式转换后合法\n", c_ok);
    printf("    规则：能用 {} 就用 {}，它能挡住窄化、最令人头疼的解析等一堆坑\n\n");

    // ---------- [6] 标量 -> bool：只有 0 / null 是 false ----------
    printf("[6] 标量转 bool\n");
    printf("    (bool)0      = %d\n", (bool)0);
    printf("    (bool)-1     = %d   // 负数也是 true！\n", (bool)-1);
    printf("    (bool)0.0    = %d\n", (bool)0.0);
    printf("    (bool)0.1    = %d\n", (bool)0.1);
    int* np = nullptr;
    printf("    (bool)nullptr= %d\n", (bool)np);
    printf("    (bool)\"\"     = %d   // 空字符串字面量是**非空指针**，转 bool 是 true\n", (bool)"");
    printf("    判空指针写 if (p) 而不是 if (p != NULL) 或 if (p == true)\n\n");

    // ---------- [7] 数组退化为指针（sizeof 失去长度信息） ----------
    printf("[7] 数组退化\n");
    int arr[10] = {};
    printf("    sizeof(arr)  = %zu   (10 个 int)\n", sizeof(arr));
    int* ap = arr;
    printf("    sizeof(ap)   = %zu   (退化后只剩指针大小)\n", sizeof(ap));
    printf("    用 sizeof(arr)/sizeof(arr[0]) 求长度只在**同一作用域**内有效；\n");
    printf("    一旦传进函数（形参是指针）就失效 —— 用 std::array / std::span 替代\n\n");

    // ---------- [8] 隐式转换链：赋值时的静默截断 ----------
    printf("[8] 赋值时的隐式截断\n");
    long long ll = 1234567890123LL;
    int truncated = ll;                   // 静默截断
    printf("    int i = 1234567890123LL -> %d   // 只保留低 32 位\n", truncated);
    printf("    int 范围 [%d, %d]\n", std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    printf("    -Wall 下 clang 对常量会警告 -Wconstant-conversion；变量则完全静默\n\n");

    // ---------- [9] 速查：什么时候必须显式转换 ----------
    printf("[9] 什么时候必须写 cast\n");
    printf("    • 无符号 -> 有符号（或反过来）：先确认范围，再 static_cast\n");
    printf("    • 浮点 -> 整型：先 clamp 到目标范围，再 static_cast（越界是 UB）\n");
    printf("    • void* -> T*：只能 static_cast\n");
    printf("    • 多态向下转型：dynamic_cast（并检查 nullptr / 捕 bad_cast）\n");
    printf("    • 指针 <-> 整数（MMIO）：reinterpret_cast + uintptr_t\n");
    printf("    • 其余「能隐式转就别写 cast」：写了反而掩盖类型错误\n");

    return 0;
}
