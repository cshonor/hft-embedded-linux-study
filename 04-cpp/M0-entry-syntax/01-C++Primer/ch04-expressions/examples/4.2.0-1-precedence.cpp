// 4.2.0-1 优先级 / 结合律的实测坑位
//
// 编译：
//   clang++ -std=c++17 -O0 -Wall examples/4.2.0-1-precedence.cpp -o prec && ./prec
//   clang++ -std=c++11 -O0 -Wall examples/4.2.0-1-precedence.cpp -o prec11   # 看 [5] 的 UB 警告差异
//
// 要点：优先级只决定「怎么加括号」，不决定「谁先算」。

#include <cstdio>
#include <cstdint>

// 注意：下面的 [1][2][8] 是「故意不加括号」来演示优先级，clang 会提醒加括号，
// 那些 -Wparentheses 警告本身就是本节的知识点，不用压掉。
int main() {
    printf("=== 4.2.0-1 优先级与结合律实测 ===\n\n");

    // ---------- [1] 算术高于移位：1 << 2 + 3 是 32 不是 7 ----------
    printf("[1] 算术高于移位\n");
    printf("    1 << 2 + 3        = %d   (加括号后 1 << (2+3) = 32)\n", 1 << 2 + 3);
    printf("    (1 << 2) + 3      = %d\n", (1 << 2) + 3);
    printf("    想要 7 必须写：(1 << 2) + 3\n\n");

    // ---------- [2] 位与低于相等：测位最经典的错写 ----------
    printf("[2] 位与(&) 低于 相等(==)\n");
    int flags = 0b1010;
    // 想写的是「第 1 位是否置位」
    printf("    flags & 0b0010 == 0b0010  -> %d   // 实际是 flags & (0b0010==0b0010) = flags & 1\n",
           flags & 0b0010 == 0b0010 ? 1 : 0);
    printf("    (flags & 0b0010) == 0b0010 -> %d   // 加括号才对\n\n",
           (flags & 0b0010) == 0b0010 ? 1 : 0);

    // ---------- [3] & 高于 |：加括号能改语义 ----------
    printf("[3] & 高于 |\n");
    int a = 0b1100, b = 0b1010, c = 0b0101;
    printf("    a & b | c  = %d   等价于 (a&b)|c = %d\n", a & b | c, (a & b) | c);
    printf("    a & (b|c)  = %d   —— 语义不同，别省括号\n\n", a & (b | c));

    // ---------- [4] 后缀 ++ 高于一元 * ：*p++ 是 *(p++) ----------
    printf("[4] 后缀 ++ 高于 一元 *\n");
    int arr[4] = {10, 20, 30, 40};
    int* p = arr;
    int v1 = *p++;                 // 取 arr[0]，然后 p 前进
    printf("    int v1 = *p++;  v1=%d, 现在 *p=%d   (等价于 *(p++))\n", v1, *p);
    int v2 = (*p)++;               // 改的是 arr[1]
    printf("    int v2 = (*p)++; v2=%d, arr[1]=%d   (括号改变一切)\n\n", v2, arr[1]);

    // ---------- [5] i = i++ ：C++11/14 是 UB，C++17 定义良好 ----------
    printf("[5] i = i++ 的标准差异\n");
#if __cplusplus >= 201703L
    volatile int i = 0;
    i = i++;                       // C++17：右边先求值(0)，再赋回 -> i 必为 0
    printf("    C++17：i = %d   （标准保证：赋值的右操作数先于左操作数求值）\n\n", i);
#else
    int i = 0;
    i = i++;                       // C++11/14：未定义行为，clang 会给 -Wunsequenced
    printf("    C++11/14：i = %d （未定义行为，换个编译器可能不是这个数）\n\n", i);
#endif

    // ---------- [6] 函数实参的求值顺序：C++17 依然未指定 ----------
    printf("[6] 函数实参求值顺序仍未指定\n");
    int step = 0;
    auto probe = [&step](const char* tag) -> int {
        printf("        %s 在第 %d 步被求值\n", tag, ++step);
        return 0;
    };
    auto sink = [](int, int) {};
    sink(probe("左实参"), probe("右实参"));
    printf("    实测 clang 从左到右，但标准没保证 —— 别依赖任何一边\n\n");

    // ---------- [7] 赋值右结合：a = b = c ----------
    printf("[7] 赋值是右结合\n");
    int x = 0, y = 0;
    x = y = 5;                     // 等价于 x = (y = 5)
    printf("    x = y = 5  ->  x=%d y=%d\n", x, y);
    printf("    (x = y) = 5 在 C++ 里合法吗？见下：\n");
    int z = 0;
    (z = y) = 7;                   // C++：赋值表达式是左值，合法；C 里是 error
    printf("    (z = y) = 7 -> z=%d y=%d  (C++ 合法；同样代码在 C 里报错)\n\n", z, y);

    // ---------- [8] ?: 优先级极低：cout << cond ? a : b 是 bug ----------
    printf("[8] 条件运算符 ?: 优先级倒数第二（只高于逗号和赋值）\n");
    int cond = 1, A = 111, B = 222;
    printf("    cond ? A : B        = %d\n", cond ? A : B);
    printf("    0 << (cond ? A : B) = %d   // 想写 (0 << cond) ? A : B 就必须加括号\n", 0 << (cond ? A : B));
    printf("    坑位原型：std::cout << cond ? A : B; 会先算 (cout << cond) 再 ?:\n\n");

    // ---------- [9] 一元运算符右结合 ----------
    printf("[9] 一元运算符右结合\n");
    int neg = -5;
    printf("    -*p      等价于 -(*p)：对 arr[1]=%d 取负 = %d\n", arr[1], -*p);
    printf("    !!(x)    = %d  (!! 是把任意标量压成 0/1 bool 的惯用法)\n", !!neg);
    printf("    注意 -!x 是 -( !x )，不是 (-x)! —— 后者语法错误\n");

    return 0;
}
