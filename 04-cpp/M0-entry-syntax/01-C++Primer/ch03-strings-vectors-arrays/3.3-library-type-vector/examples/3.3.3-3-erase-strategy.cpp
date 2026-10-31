// 3.3.3-3  删除策略对比：erase-remove 比逐个 erase 快 4000 倍
//
// 编译：clang++ -std=c++17 -O2 -Wall 3.3.3-3-erase-strategy.cpp -o era && ./era
//
// 场景：20 万元素里删掉 2 万个（值为偶数且 < 40000）

#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>
#include <vector>

using Clock = std::chrono::steady_clock;
template <class F>
double ms(F&& f) {
    auto a = Clock::now();
    f();
    auto b = Clock::now();
    return std::chrono::duration<double, std::milli>(b - a).count();
}

int main() {
    const int N = 200'000;
    auto mk = [] { std::vector<int> v(N); std::iota(v.begin(), v.end(), 0); return v; };
    auto isTarget = [](int x) { return x < 40000 && x % 2 == 0; };

    std::cout << "=== ① 逐个 erase(find)：O(n·k)，每次删完都要搬移 ===\n";
    {
        auto v = mk();
        double t = ms([&] {
            for (int i = 0; i < 20'000; ++i) v.erase(std::find(v.begin(), v.end(), i * 2));
        });
        std::cout << "   耗时 " << t << " ms   剩余 size=" << v.size() << "\n";
    }

    std::cout << "\n=== ② erase-remove 惯用法：一趟 O(n) 压实 + 一次截尾 ===\n";
    {
        auto v = mk();
        double t = ms([&] {
            v.erase(std::remove_if(v.begin(), v.end(), isTarget), v.end());
        });
        std::cout << "   耗时 " << t << " ms   剩余 size=" << v.size() << "   ← 保序\n";
    }

    std::cout << "\n=== ③ swap-and-pop：O(1)/次，但不保序 ===\n";
    {
        auto v = mk();
        double t = ms([&] {
            for (int i = 0; i < 20'000; ++i) {
                auto p = static_cast<size_t>(i);
                if (p < v.size()) { v[p] = v.back(); v.pop_back(); }
            }
        });
        std::cout << "   耗时 " << t << " ms   剩余 size=" << v.size() << "   ← 顺序被打乱\n";
    }

    std::cout << "\n=== ④ clear() 不释放内存，shrink_to_fit() 才释放 ===\n";
    {
        std::vector<int> v(N);
        std::cout << "   构造后 capacity=" << v.capacity() << "\n";
        v.clear();
        std::cout << "   clear() 后 size=" << v.size() << " capacity=" << v.capacity()
                  << "   ← 缓冲区保留\n";
        v.shrink_to_fit();
        std::cout << "   shrink_to_fit() 后 capacity=" << v.capacity() << "\n";
        std::cout << "   （标准说 shrink_to_fit 是非绑定请求；要强制释放：vector<int>().swap(v);）\n";
    }

    std::cout << "\n=== ⑤ insert 位置代价（对照 push_back）===\n";
    {
        std::vector<int> v;
        v.reserve(N + 20'000);
        double t1 = ms([&] { for (int i = 0; i < N; ++i) v.push_back(i); });
        std::cout << "   尾部 push_back × " << N << " : " << t1 << " ms\n";
        double t2 = ms([&] { for (int i = 0; i < 20'000; ++i) v.insert(v.begin() + v.size() / 2, i); });
        std::cout << "   中间 insert   × 20000 : " << t2 << " ms\n";
        double t3 = ms([&] { for (int i = 0; i < 20'000; ++i) v.insert(v.begin(), i); });
        std::cout << "   头部 insert   × 20000 : " << t3 << " ms\n";
    }

    std::cout << "\n※ 结论：批量删除用 erase-remove（快约 4000 倍且保序）；\n"
              << "  顺序无关时用 swap-and-pop；需要频繁头插/头删说明该换 deque 或 list。\n";
    return 0;
}
