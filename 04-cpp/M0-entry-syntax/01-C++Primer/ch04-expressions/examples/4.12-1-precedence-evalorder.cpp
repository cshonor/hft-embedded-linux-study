// 4.12-1-precedence-evalorder.cpp —— 优先级（编译期固定）vs 求值顺序（标准说了才算）
//
// 编译运行：
//   clang++ -std=c++17 -Wall -O2 4.12-1-precedence-evalorder.cpp -o a && ./a
// 对比 C++14（同一份代码，求值顺序规则不同）：
//   clang++ -std=c++14 -Wall -O2 4.12-1-precedence-evalorder.cpp -o b && ./b
// 对比优化级别对 UB 的影响：
//   clang++ -std=c++17 -O0 4.12-1-precedence-evalorder.cpp -o c && ./c
//
// ⚠ 本文件有 1 处**故意保留**的 warning（-Wunsequenced），它就是 C++14 下 UB 的证据。

#include <cstdio>

// 下面几处是"故意不加括号"来演示优先级，clang 会提醒加括号——这里压掉噪音，
// 真正要保留的警告是 C++14 下的 -Wunsequenced（见 [5]）。
#pragma clang diagnostic ignored "-Wshift-op-parentheses"

// ---------- 探针：打印自己被求值的时刻 ----------
static int step = 0;
int side(const char* tag, int v) {
    printf("      step %d: %s 被求值\n", ++step, tag);
    return v;
}
void reset_step() { step = 0; }
int sum2(int p, int q);            // 前向声明（定义在文件末尾）

int main() {
    printf("=== 4.12-1 优先级 vs 求值顺序（clang %d.%d, c++%ld）===\n\n",
           __clang_major__, __clang_minor__, __cplusplus);

    // ---------------------------------------------------------------
    printf("[1] 优先级：编译期就焊死，跟求值顺序是两回事\n");
    int a = 1, b = 2, c = 4;
    printf("    1 + 2 << 3   = %d   ← + 高于 <<，即 (1+2)<<3\n", 1 + 2 << 3);
    printf("    1 << 2 + 3   = %d   ← 同理 (2+3) 先算\n", 1 << 2 + 3);
    printf("    a & b == c   = %d   ← == 高于 &，即 a & (b==c) = %d & %d\n",
           a & b == c, a, (b == c));
    printf("    ⚠ 想要 (a&b)==c 必须自己加括号 —— clang 会给 -Wparentheses 警告\n");
    int x, y, z;
    x = y = z = 7;                       // 赋值右结合
    printf("    x = y = z = 7 → x=%d y=%d z=%d（右结合）\n\n", x, y, z);

    // ---------------------------------------------------------------
    printf("[2] 哪些运算符**有**求值顺序保证（实测打印顺序）\n\n");

    printf("    (a) && || 短路 + 从左到右\n");
    reset_step();
    if (side("左操作数", 0) && side("右操作数", 1)) { }
    printf("      → 左操作数为 0，右操作数**根本没被求值**\n\n");

    printf("    (b) ?: 只求值被选中的分支\n");
    reset_step();
    int t = 1 ? side("then 分支", 10) : side("else 分支", 20);
    printf("      → t = %d，else 分支没被求值\n\n", t);

    printf("    (c) 逗号运算符：从左到右\n");
    reset_step();
    int cm = (side("逗号左", 1), side("逗号右", 2));
    printf("      → cm = %d\n\n", cm);

    // ---------------------------------------------------------------
    printf("[3] C++17 新增的保证：<< >> = [] 都是「左/右操作数先于运算」\n");
#if __cplusplus >= 201703L
    reset_step();
    int sh = (side("<< 左侧", 1) << side("<< 右侧", 2));
    printf("      → sh = %d，C++17 起顺序确定（左 → 右）\n", sh);
    reset_step();
    int i2 = 0;
    i2 = side("= 右侧", 5);
    printf("      → i2 = %d，C++17 起右操作数先求值，再写入左操作数\n\n", i2);
#else
    printf("      ⚠ 当前是 C++%ld：<< 的求值顺序**未指定**，本段不演示\n\n", __cplusplus);
#endif

    // ---------------------------------------------------------------
    printf("[4] 函数实参的求值顺序：**至今仍未指定**（最容易被误以为有保证）\n");
    reset_step();
    printf("    调用 probe(side(A), side(B)) ：\n");
    {
        int r = sum2(side("实参 A", 1), side("实参 B", 2));
        printf("      → r = %d，两个实参的求值顺序由编译器决定\n", r);
    }
    printf("    实测本机 clang 是从左到右（A 先于 B），但标准没保证 ——\n");
    printf("    换个编译器/换个优化级别都可能反过来，别依赖任何一边\n\n");

    // ---------------------------------------------------------------
    printf("[5] UB：同一表达式里改两次同一个变量\n");
    int i = 0;
#if __cplusplus >= 201703L
    i = 0;
    i = i++;                     // C++17：右操作数先求值并写回，再赋值 → 定义良好
    printf("    C++17: i = i++ 之后 i = %d（定义良好：右操作数先 sequenced-before 赋值）\n", i);
#else
    i = 0;
    i = i++;                     // C++14：UB（编译期 -Wunsequenced 警告）
    printf("    C++14: i = i++ 之后 i = %d ⚠ 未定义行为，换个编译器/优化级别可能不同\n", i);
#endif
    int j = 0;
    int ub = j++ + ++j;          // 任何标准下都是 UB
    printf("    j++ + ++j  = %d ⚠ UB（用 -O0 和 -O2 分别编译，结果可能不一样）\n\n", ub);

    printf("[6] 记住这三句话\n");
    printf("    ① 优先级只决定「怎么加括号」，不决定「谁先算」\n");
    printf("    ② 只有 && || ?: , 以及 C++17 起的 << >> = [] ->* 有顺序保证\n");
    printf("    ③ 函数实参顺序至今未指定 —— 别在一个实参里改、另一个实参里读同一个变量\n");
    return 0;
}

int sum2(int p, int q) { return p + q; }
