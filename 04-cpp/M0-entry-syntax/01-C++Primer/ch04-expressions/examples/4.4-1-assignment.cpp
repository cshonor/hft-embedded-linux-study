// 4.4-1 赋值运算符：左值性、求值顺序、复合赋值
//
// 编译：
//   clang++ -std=c++17 -O2 examples/4.4-1-assignment.cpp -o assign && ./assign

#include <chrono>
#include <cstdio>
#include <string>

// 用来观察「左值被求值几次」的计数器容器
struct Probe {
    int v = 0;
    int& get() { ++reads; return v; }
    static int reads;
};
int Probe::reads = 0;

struct Widget {
    int id = 0;
    Widget& operator=(const Widget& o) { id = o.id; ++copies; return *this; }
    static int copies;
};
int Widget::copies = 0;

int main() {
    printf("=== 4.4-1 赋值运算符实测 ===\n\n");

    // ---------- [1] 复合赋值：左值只求值一次 ----------
    printf("[1] 复合赋值 vs 普通赋值：左值求值次数\n");
    Probe::reads = 0;
    Probe p;
    p.get() = p.get() + 1;      // 左值求值 2 次（一次定位、一次读取）
    printf("    p.get() = p.get() + 1  -> 左值被求值 %d 次\n", Probe::reads);

    Probe::reads = 0;
    p.get() += 1;               // 复合赋值：只求值 1 次
    printf("    p.get() += 1           -> 左值被求值 %d 次\n", Probe::reads);
    printf("    结论：a[i++] += x 与 a[i++] = a[i++] + x 完全不同，后者 i 加两次\n\n");

    // ---------- [2] 赋值表达式在 C++ 里是左值 ----------
    printf("[2] 赋值表达式的左值性（C++ 独有）\n");
    int a = 1, b = 2;
    (a = b) = 3;                // ✅ C++：a=b 返回 a 的左值引用，可继续赋值
    printf("    (a = b) = 3  ->  a=%d b=%d   // C++ 合法；同样代码在 C 里是 error\n", a, b);
    int& r = (a = 5);
    printf("    int& r = (a = 5); r 现在是 %d 的引用 -> %d\n", a, r);
    printf("    ⚠ 类类型的 operator= 如果不返回引用，链式赋值会多出拷贝\n\n");

    // ---------- [3] C++17 规定：右操作数先于左操作数求值 ----------
    printf("[3] 赋值的求值顺序\n");
    int step = 0;
    auto probe = [&step](const char* tag) -> int& {
        printf("        %s 在第 %d 步求值\n", tag, ++step);
        static int slot = 0;
        return slot;
    };
    probe("左") = probe("右");   // C++17：右先，再左
    printf("    C++17 起：右操作数先求值，然后才求左操作数（C++11/14 未指定）\n");
    printf("    ⚠ i = i++ 在 C++11/14 是 UB，C++17 才定义成「i 不变」\n\n");

    // ---------- [4] 类类型 operator= 必须返回引用 ----------
    printf("[4] 类类型的赋值返回值\n");
    Widget w1{1}, w2{2}, w3{3};
    Widget::copies = 0;
    w1 = w2 = w3;               // 右结合：w1 = (w2 = w3)
    printf("    w1 = w2 = w3  ->  w1.id=%d w2.id=%d w3.id=%d，operator= 调用 %d 次\n",
           w1.id, w2.id, w3.id, Widget::copies);
    printf("    返回 Widget& 才能链式；返回 Widget（值）会多出 2 次拷贝\n\n");

    // ---------- [5] 连续赋值的成本：以 string 为例 ----------
    printf("[5] string 连续赋值（深拷贝是真实开销）\n");
    const int N = 200000;
    std::string s1, s2;
    std::string payload(256, 'x');       // 256 字节，超过 SSO 阈值
    printf("    SSO 阈值：短字符串通常 <= 15 字节不分配堆内存\n");
    printf("    sizeof(std::string) = %zu，本例 payload.size() = %zu\n",
           sizeof(std::string), payload.size());

    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) {
        s1 = payload;                     // 每次深拷贝 256 字节
        s2 = s1;                          // 又一次深拷贝
    }
    auto t1 = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    printf("    %d 轮「两次赋值」= %.2f ms（%.1f ns/轮）\n", N, ms, ms * 1e6 / N);
    printf("    热路径上想省掉它：用 std::string_view 只读传递，或 move 赋值\n\n");

    // ---------- [6] 赋值 vs 初始化 ----------
    printf("[6] 别把赋值当初始化\n");
    printf("    int x = 5;   // 初始化（调用构造函数 / 直接置值）\n");
    printf("    int y; y = 5;// 先默认初始化，再赋值 —— 类类型会多一次构造+析构\n");
    printf("    成员初始化一律用初始化列表：Foo() : m(5) {} 而不是 Foo() { m = 5; }\n\n");

    // ---------- [7] 速查 ----------
    printf("[7] 速查\n");
    printf("    • 赋值是**右结合**：a = b = c 等价于 a = (b = c)\n");
    printf("    • 优先级很低，仅高于逗号：x = a + b 不用加括号，x = (a, b) 必须加\n");
    printf("    • 复合赋值（+= 等）左值只求值一次，且只做一次类型转换\n");
    printf("    • 内置赋值返回左值引用；自定义 operator= 也要返回 T&\n");
    printf("    • 自赋值检查：a = a 时 operator= 要能正确工作（先拷后删 / copy-swap）\n");

    return 0;
}
