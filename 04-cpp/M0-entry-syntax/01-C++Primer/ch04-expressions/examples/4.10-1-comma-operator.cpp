// 4.10-1 逗号运算符：优先级最低、依次求值取最右
//
// 编译：
//   clang++ -std=c++17 -O2 -Wall examples/4.10-1-comma-operator.cpp -o comma && ./comma

#include <cstdio>
#include <vector>

struct Probe {
    int v;
    // 重载逗号运算符：会毁掉「左先于右」的顺序保证
    Probe operator,(const Probe&) const { printf("        [重载的逗号被调用]\n"); return *this; }
};

int main() {
    printf("=== 4.10-1 逗号运算符实测 ===\n\n");

    // ---------- [1] 依次求值，取最右 ----------
    printf("[1] 从左到右依次求值，结果是最后一个\n");
    int i = 0;
    int r = (i = 1, i = 2, i = 3);
    printf("    (i=1, i=2, i=3) 的结果 = %d，i = %d\n", r, i);
    printf("    逗号表达式的类型和值 = 最右操作数；其余操作数的值被丢弃\n\n");

    // ---------- [2] 逗号保证「左先于右」—— 这是它最有用的性质 ----------
    printf("[2] 逗号是少数有**顺序保证**的运算符\n");
    int step = 0;
    auto probe = [&step](const char* tag) -> int {
        printf("        %s 在第 %d 步\n", tag, ++step);
        return 0;
    };
    int r2 = (probe("第一"), probe("第二"), probe("第三"));
    printf("    结果 = %d（取最右），三步顺序固定\n", r2);
    printf("    ✅ 标准保证：逗号左侧的每个求值和副作用，都先于右侧发生\n\n");

    // ---------- [3] 函数实参里的逗号不是逗号运算符 ----------
    printf("[3] 括号里的逗号 vs 函数调用的逗号\n");
    auto f = [](int a, int b) { printf("        f(a=%d, b=%d)\n", a, b); };
    f(1, 2);                       // 这是参数分隔符，不是逗号运算符
    printf("    f(1, 2) 是两个实参；f((1, 2)) 才是一个实参（值为 2）\n");
    auto g = [](int a) { printf("        g(a=%d)\n", a); };
    g((1, 2));                     // 先算 1（丢弃），再算 2，只传一个实参
    printf("    ⚠ 写 f(a, b) 想表达逗号运算符是常见误解，必须再加一层括号\n\n");

    // ---------- [4] arr[1, 3] 静默取后一个下标 ----------
    printf("[4] 下标里的逗号\n");
    int arr[5] = {10, 20, 30, 40, 50};
    printf("    arr[5] = {10,20,30,40,50}\n");
    printf("    arr[1, 3]  = %d   // ❗等价于 arr[3]，前面的 1 被丢弃\n", arr[1, 3]);
    printf("    arr[3]     = %d\n", arr[3]);
    printf("    这种写法几乎永远是笔误；二维请用 arr[i][j]\n\n");

    // ---------- [5] 优先级最低：赋值里必须加括号 ----------
    printf("[5] 优先级最低\n");
    int a = 0, b = 0;
    a = 1, b = 2;                  // 等价于 (a = 1), (b = 2) —— 逗号低于赋值
    printf("    a = 1, b = 2   ->  a=%d b=%d   // 逗号比赋值还低\n", a, b);
    int c = (1, 2);                // 想让 c = 2，括号不可省
    printf("    int c = (1, 2) ->  c=%d   // 不加括号会变成 (int c = 1), 2\n", c);
    printf("    同理：for (i=0, j=0; ...) 里的逗号是声明分隔符，不是运算符\n\n");

    // ---------- [6] 重载逗号后，顺序保证消失 ----------
    printf("[6] 重载逗号运算符会毁掉顺序保证\n");
    Probe p1{1}, p2{2};
    Probe p3 = (p1, p2);           // 调用重载的 operator,
    printf("    结果 p3.v = %d（取最右 p2）\n", p3.v);
    printf("    ⚠ 一旦重载，逗号变成普通函数调用：求值顺序退回「未指定」\n");
    printf("    这就是为什么不应该重载 operator,（&& 和 || 同理）\n\n");

    // ---------- [7] 逗号的正当用途 ----------
    printf("[7] 什么时候该用逗号\n");
    std::vector<int> v{1, 2, 3};
    int sum = 0;
    for (size_t k = 0, n = v.size(); k < n; ++k) {
        sum += v[k];               // 初始化语句里的逗号：声明两个变量（这是分隔符）
    }
    printf("    for 的初始化/自增子句里推进多个变量：for (++i, --j; ...)\n");
    printf("    宏里塞多条语句；或者 (void)cast 之后取值这种小技巧\n");
    printf("    其余场景一律拆成多行语句 —— 可读性优先\n\n");

    // ---------- [8] 速查 ----------
    printf("[8] 速查\n");
    printf("    • 优先级**最低**，比赋值还低；想当运算符用必须加括号\n");
    printf("    • 从左到右依次求值，结果是最右操作数的值和类型\n");
    printf("    • 有确定顺序保证（左先于右）—— 这点 && || ?: 也有\n");
    printf("    • 函数实参、初始化列表里的逗号是**分隔符**，不是运算符\n");
    printf("    • 不要重载 operator,（同样不要重载 && 和 ||）\n");

    return 0;
}
