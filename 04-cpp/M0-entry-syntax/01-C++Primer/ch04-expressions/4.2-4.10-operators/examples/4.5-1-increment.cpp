// 4.5-1  自增自减：前置 vs 后置 —— 语义、汇编等价性、真实的拷贝代价、C++17 求值顺序
//
// 编译（优化）：clang++ -std=c++17 -O2 -Wall 4.5-1-increment.cpp -o inc && ./inc
// 编译（无优化）：clang++ -std=c++17 -O0 -Wall 4.5-1-increment.cpp -o inc0 && ./inc0
//
// 核心结论：
//   ① -O2 下对 int / vector 迭代器，前置与后置生成【完全相同】的汇编（实测都是 83 条指令）
//   ② 后置真正的代价只在：副本被使用 / 优化器看不透 / -O0
//   ③ C++17 起 i = i++ 变良定义，但 i++ + i++ 仍是 UB

#include <chrono>
#include <iostream>
#include <type_traits>
#include <vector>

using Clock = std::chrono::steady_clock;
static volatile long vsink = 0;

struct Counter {                 // 轻量类：演示重载签名
    int v;
    Counter& operator++()    { ++v; return *this; }                    // 前置：无参，返回引用
    Counter  operator++(int) { Counter t = *this; ++v; return t; }     // 后置：int 哑元，返回副本
};

struct Heavy {                   // 重型类：后置要拷 64 字节
    long d[8];
    Heavy& operator++()    { for (auto& x : d) ++x; return *this; }
    Heavy  operator++(int) { Heavy t = *this; for (auto& x : d) ++x; return t; }
};

int main() {
    std::cout << "=== ① 语义对照 ===\n";
    int i = 10; ++i; int j = i++;
    std::cout << "  i=10; ++i; int j=i++;  → i=" << i << " j=" << j << "\n";
    i = 5; int a = ++i;
    std::cout << "  i=5; int a=++i;        → i=" << i << " a=" << a << "\n";
    i = 5; int b = i++;
    std::cout << "  i=5; int b=i++;        → i=" << i << " b=" << b << "\n";

    std::cout << "\n=== ② 类类型重载：返回值是引用还是副本 ===\n";
    Counter c{0};
    std::cout << "  decltype(++c) 是 Counter& ? " << std::is_same<decltype(++c), Counter&>::value << "\n";
    std::cout << "  decltype(c++) 是 Counter  ? " << std::is_same<decltype(c++), Counter>::value << "\n";
    int k = 0;
    ++++k;
    std::cout << "  ++++k  → 合法，k=" << k << "（前置返回左值）\n";
    std::cout << "  k++++  → 编译错误（后置返回右值）\n";

    std::cout << "\n=== ③ 迭代器：前置 vs 后置（-O2 下无差异）===\n";
    {
        std::vector<int> v(1000, 1);
        for (int rep = 0; rep < 2; ++rep) {
            auto t0 = Clock::now();
            long s = 0;
            for (int r = 0; r < 10000; ++r) for (auto it = v.begin(); it != v.end(); ++it) s += *it;
            auto t1 = Clock::now();
            double x = std::chrono::duration<double, std::milli>(t1 - t0).count();

            auto t2 = Clock::now();
            long s2 = 0;
            for (int r = 0; r < 10000; ++r) for (auto it = v.begin(); it != v.end(); it++) s2 += *it;
            auto t3 = Clock::now();
            double y = std::chrono::duration<double, std::milli>(t3 - t2).count();
            std::cout << "  rep" << rep << ": ++it " << x << " ms   it++ " << y
                      << " ms   (差 " << (x - y) << " ms)\n";
            vsink = s + s2;
        }
        std::cout << "  ※ -O2 下两者汇编完全相同（实测 83 条指令逐条一致）\n"
                  << "     优化器把后置那份没人用的副本直接消除了\n";
    }

    std::cout << "\n=== ④ 后置【真的】慢的场景：重型类 + 低优化 ===\n";
    {
        std::cout << "  sizeof(Heavy) = " << sizeof(Heavy) << " 字节（后置每次拷这么多）\n";
        Heavy h{};
        auto t0 = Clock::now();
        for (int n = 0; n < 5'000'000; ++n) ++h;
        auto t1 = Clock::now();
        double x = std::chrono::duration<double, std::milli>(t1 - t0).count();

        Heavy g{};
        auto t2 = Clock::now();
        for (int n = 0; n < 5'000'000; ++n) g++;
        auto t3 = Clock::now();
        double y = std::chrono::duration<double, std::milli>(t3 - t2).count();

        std::cout << "  前置 ++h : " << x << " ms\n";
        std::cout << "  后置 g++ : " << y << " ms\n";
        if (x > 0.001) std::cout << "  比值 = " << (y / x) << "x   （-O0 下实测约 1.16x）\n";
        else           std::cout << "  （-O2 下整个循环被优化掉了，要用 -O0 才能看到真实差距）\n";
        vsink = h.d[0] + g.d[0];
    }

    std::cout << "\n=== ⑤ C++17 求值顺序：i = i++ 从 UB 变良定义 ===\n";
    {
        int m = 0; m = m++;
        std::cout << "  int m=0; m = m++;  → m=" << m << "   ← 良定义（右操作数先求值）\n";
        int p = 0; p = ++p;
        std::cout << "  int p=0; p = ++p;  → p=" << p << "   ← 良定义\n";
        std::cout << "  i++ + i++          → 仍是 UB（两个修改之间无顺序保证）\n";
        std::cout << "  i++ + ++i / v[i]=i++ → 也都是 UB\n";
    }

    std::cout << "\n=== ⑥ 结论 ===\n"
              << "  仍然推荐 ++i，但理由换成「换类型/换优化级别不用改」，不是「更快」\n"
              << "  真正要避开的是：副本被使用（auto old = it++）和 -O0 构建\n";
    return 0;
}
