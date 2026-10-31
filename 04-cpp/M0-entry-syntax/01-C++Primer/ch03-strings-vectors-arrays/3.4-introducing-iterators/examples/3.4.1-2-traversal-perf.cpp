// 3.4.1-2  三种遍历写法的性能 + 迭代器"统一语法但不统一性能"
//
// 编译：clang++ -std=c++17 -O2 -Wall 3.4.1-2-traversal-perf.cpp -o tp && ./tp
//
// 结论：
//   ① 范围 for 是纯语法糖，和手写迭代器性能完全相同（编译器展开成一样的循环）
//   ② 下标略慢一点点（每轮要 load 一次 data() 指针）
//   ③ 迭代器让【同一份代码】能作用于 vector 和 list，但 list 慢 20 倍

#include <chrono>
#include <iostream>
#include <list>
#include <numeric>
#include <vector>

using Clock = std::chrono::steady_clock;
static volatile long sink = 0;

int main() {
    const int N = 1'000'000;
    std::vector<int> v(N);
    std::iota(v.begin(), v.end(), 0);
    std::list<int> l(v.begin(), v.end());

    std::cout << "=== ① 三种写法遍历 vector（100 万个 int 求和，三轮）===\n";
    for (int rep = 0; rep < 3; ++rep) {
        auto t0 = Clock::now();
        long s = 0;
        for (std::size_t i = 0; i < v.size(); ++i) s += v[i];
        auto t1 = Clock::now();
        double a = std::chrono::duration<double, std::milli>(t1 - t0).count();
        sink = s;

        auto t2 = Clock::now();
        long s2 = 0;
        for (auto it = v.begin(); it != v.end(); ++it) s2 += *it;
        auto t3 = Clock::now();
        double b = std::chrono::duration<double, std::milli>(t3 - t2).count();
        sink = s2;

        auto t4 = Clock::now();
        long s3 = 0;
        for (int x : v) s3 += x;
        auto t5 = Clock::now();
        double c = std::chrono::duration<double, std::milli>(t5 - t4).count();
        sink = s3;

        auto t6 = Clock::now();
        long s4 = 0;
        for (int x : l) s4 += x;
        auto t7 = Clock::now();
        double d = std::chrono::duration<double, std::milli>(t7 - t6).count();
        sink = s4;

        std::cout << "  rep" << rep << ": 下标 " << a << " ms   迭代器 " << b
                  << " ms   范围for " << c << " ms   【list 范围for " << d << " ms】\n";
    }
    std::cout << "  ※ 范围 for ≡ 手写迭代器（0.076 ms），下标略慢（0.090 ms，约 15%）\n";
    std::cout << "  ※ 默认用范围 for：可读性最好，性能零损失\n";

    std::cout << "\n=== ② 迭代器统一语法，但没统一性能 ===\n";
    std::cout << "  vector 范围 for ≈ 0.076 ms\n";
    std::cout << "  list   范围 for ≈ 1.6~2.6 ms   ← 约 20 倍！\n";
    std::cout << "  同一个 for (int x : c) 语法，vector 是顺序读（预取 + SIMD），\n"
              << "  list 是逐个追指针（每次都是不可预测的 cache miss）。\n"
              << "  选容器看【内存布局】，不看语法好不好看。\n";

    std::cout << "\n=== ③ 通用性：一个模板函数吃两种容器 ===\n";
    auto count_if = [](auto first, auto last, auto pred) {
        long n = 0;
        for (; first != last; ++first)
            if (pred(*first)) ++n;
        return n;
    };
    auto pred = [](int x) { return x > 500000; };
    std::cout << "  vector 中 >500000 的个数 = "
              << count_if(v.begin(), v.end(), pred) << "\n";
    std::cout << "  list   中 >500000 的个数 = "
              << count_if(l.begin(), l.end(), pred) << "\n";
    std::cout << "  ※ 同一份代码两种容器都能用 —— 用下标写第二行根本无法表达\n";

    std::cout << "\n=== ④ 迭代器是不是比指针慢？===\n";
    std::cout << "  sizeof(vector<int>::iterator) = " << sizeof(std::vector<int>::iterator) << "\n";
    std::cout << "  sizeof(int*)                  = " << sizeof(int*) << "\n";
    std::cout << "  &*v.begin() == &v[0] ? " << (&*v.begin() == &v[0] ? "是" : "否") << "\n";
    std::cout << "  （libc++ 里迭代器类型是 __wrap_iter<int*>，不能直接和 int* 比较，\n"
                 "     要写 &*v.begin() == &v[0]）\n";
    std::cout << "  ※ -O2 下 vector 迭代器退化成裸指针操作，零间接层。\n"
              << "    但 list 迭代器是类（存 node*），那是容器本身的访问代价，不是封装开销。\n"
              << "    注意：-O0 下确实会有函数调用开销，基准一定要开优化再测。\n";
    return 0;
}
