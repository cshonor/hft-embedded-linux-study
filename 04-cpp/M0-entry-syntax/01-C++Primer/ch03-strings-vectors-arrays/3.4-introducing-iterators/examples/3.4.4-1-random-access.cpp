// 3.4.4-1  随机访问迭代器：类别、difference_type、distance 复杂度、二分
//
// 编译：clang++ -std=c++17 -O2 -Wall 3.4.4-1-random-access.cpp -o ra && ./ra

#include <algorithm>
#include <array>
#include <chrono>
#include <deque>
#include <forward_list>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

using Clock = std::chrono::steady_clock;
static volatile long vsink = 0;

template <class T>
const char* cat() {
    using C = typename std::iterator_traits<T>::iterator_category;
    if (std::is_same<C, std::random_access_iterator_tag>::value) return "随机访问";
    if (std::is_same<C, std::bidirectional_iterator_tag>::value) return "双向";
    if (std::is_same<C, std::forward_iterator_tag>::value)       return "前向";
    return "其他";
}

int main() {
    std::cout << "=== ① 各容器的 iterator_category（实测）===\n";
    std::cout << "  vector<int>       : " << cat<std::vector<int>::iterator>() << "\n";
    std::cout << "  string            : " << cat<std::string::iterator>() << "\n";
    std::cout << "  array<int,5>      : " << cat<std::array<int, 5>::iterator>() << "\n";
    std::cout << "  deque<int>        : " << cat<std::deque<int>::iterator>() << "\n";
    std::cout << "  list<int>         : " << cat<std::list<int>::iterator>() << "\n";
    std::cout << "  map<int,int>      : " << cat<std::map<int, int>::iterator>() << "\n";
    std::cout << "  set<int>          : " << cat<std::set<int>::iterator>() << "\n";
    std::cout << "  forward_list<int> : " << cat<std::forward_list<int>::iterator>() << "\n";
    std::cout << "  ※ deque 也是随机访问（分段连续，跳跃可算术算）\n";

    std::cout << "\n=== ② difference_type ===\n";
    using D = std::vector<int>::difference_type;
    std::cout << "  是 ptrdiff_t? " << std::is_same<D, std::ptrdiff_t>::value
              << "   有符号? " << std::is_signed<D>::value
              << "   sizeof = " << sizeof(D) << "\n";

    std::cout << "\n=== ③ 反方向相减 → 转 size_t 的灾难 ===\n";
    std::vector<int> v{1, 2, 3, 4, 5};
    auto d1 = v.end() - v.begin();
    auto d2 = v.begin() - v.end();
    std::cout << "  v.end() - v.begin() = " << d1 << "\n";
    std::cout << "  v.begin() - v.end() = " << d2 << "\n";
    std::size_t bad = static_cast<std::size_t>(d2);
    std::cout << "  size_t(bad)         = " << bad << "   ← 2^64 - 5\n";
    std::cout << "  bad < 100 ?         = " << (bad < 100) << "   ← 比较也全错！\n";

    std::cout << "\n=== ④ std::distance 的复杂度随类别变 ===\n";
    {
        std::list<int> l(1'000'000);
        std::vector<int> big(1'000'000);

        auto t0 = Clock::now();
        auto ld = std::distance(l.begin(), l.end());
        auto t1 = Clock::now();
        double a = std::chrono::duration<double, std::milli>(t1 - t0).count();

        auto t2 = Clock::now();
        auto bd = std::distance(big.begin(), big.end());
        auto t3 = Clock::now();
        double b = std::chrono::duration<double, std::milli>(t3 - t2).count();

        std::cout << "  distance(list 100万)   = " << ld << "   耗时 " << a << " ms   ← O(n) 逐个 ++\n";
        std::cout << "  distance(vector 100万) = " << bd << "   耗时 " << b << " ms   ← O(1) 一次减法\n";
        if (b > 0) std::cout << "  比值 ≈ " << (a / b) << "x\n";
        std::cout << "  ※ 在循环里反复调 distance 会把 O(n) 变成 O(n²)\n";
    }

    std::cout << "\n=== ⑤ 二分：手写迭代器算术 vs std::lower_bound ===\n";
    {
        std::vector<int> sorted(1'000'000);
        for (std::size_t i = 0; i < sorted.size(); ++i) sorted[i] = static_cast<int>(i);

        auto bs = [&](int target) {
            auto beg = sorted.begin(), end = sorted.end();
            while (beg < end) {
                auto mid = beg + (end - beg) / 2;      // 正确：差值 ÷ 2 再加
                if (*mid == target)      return static_cast<int>(mid - sorted.begin());
                else if (*mid < target)  beg = mid + 1;
                else                     end = mid;
            }
            return -1;
        };

        auto t4 = Clock::now();
        long s = 0;
        for (int i = 0; i < 200'000; ++i) s += bs(i * 5 % 1'000'000);
        auto t5 = Clock::now();
        double c = std::chrono::duration<double, std::milli>(t5 - t4).count();
        vsink = s;

        auto t6 = Clock::now();
        long s2 = 0;
        for (int i = 0; i < 200'000; ++i) {
            auto it = std::lower_bound(sorted.begin(), sorted.end(), i * 5 % 1'000'000);
            s2 += (it - sorted.begin());
        }
        auto t7 = Clock::now();
        double d = std::chrono::duration<double, std::milli>(t7 - t6).count();
        vsink = s2;

        std::cout << "  手写二分   20 万次 : " << c << " ms (sum=" << s << ")\n";
        std::cout << "  lower_bound 20 万次 : " << d << " ms (sum=" << s2 << ")\n";
        if (d > 0) std::cout << "  ※ lower_bound 快 " << (c / d) << " 倍 → 别手搓二分\n";
    }

    std::cout << "\n=== ⑥ 两个迭代器不能相加 ===\n";
    std::cout << "  (v.begin() + v.end()) / 2                → 编译错误\n";
    std::cout << "  v.begin() + (v.end() - v.begin()) / 2    → 正确\n";
    {
        auto mid = v.begin() + (v.end() - v.begin()) / 2;
        std::cout << "  实测：*mid = " << *mid << "（5 个元素 {1,2,3,4,5} 的中间）\n";
        std::cout << "  mid - v.begin() = " << (mid - v.begin()) << "  ← 距离即下标\n";
    }

    std::cout << "\n=== ⑦ 越界算术同样 UB ===\n";
    std::cout << "  v.begin() - 1 / v.end() + 1 / v.begin() + 100 都是 UB，不抛异常\n";
    return 0;
}
