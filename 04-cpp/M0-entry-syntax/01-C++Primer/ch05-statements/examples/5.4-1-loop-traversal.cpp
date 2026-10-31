// 5.4 迭代语句：遍历写法实测 / auto 拷贝陷阱 / size_t 回绕
//
//   clang++ -std=c++17 -O2 5.4-1-loop-traversal.cpp -o l && ./l
//
// 三个实测结论：
//   1) -O2 下五种遍历写法耗时几乎相同 → 瓶颈是内存带宽，不是循环写法
//   2) for (auto x : v) 是真的拷贝每个元素，改不到原值
//   3) for (size_t i = n-1; i >= 0; --i) 恒真，减到 0 后回绕成 2^64-1

#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

using clk = std::chrono::steady_clock;
static volatile long sink;

int main() {
    // ---------- 1) 五种遍历写法 ----------
    std::vector<int> v(1000000, 1);
    const int R = 20;
    auto bench = [&](const char* name, auto f) {
        auto t0 = clk::now();
        for (int r = 0; r < R; ++r) f();
        auto t1 = clk::now();
        printf("  %-32s %6.2f ms / %d 轮\n", name,
               std::chrono::duration<double, std::milli>(t1 - t0).count(), R);
    };

    printf("===== 1) 五种遍历写法（100 万元素 × 20 轮，-O2）=====\n");
    bench("下标 v[i]  (size_t)", [&] { long s = 0; for (size_t i = 0; i < v.size(); ++i) s += v[i]; sink = (long)s; });
    bench("下标 v[i]  (int 缓存 size)", [&] { long s = 0; int n = (int)v.size(); for (int i = 0; i < n; ++i) s += v[i]; sink = s; });
    bench("迭代器 begin/end", [&] { long s = 0; for (auto it = v.begin(); it != v.end(); ++it) s += *it; sink = s; });
    bench("范围 for (auto x 拷贝)", [&] { long s = 0; for (auto x : v) s += x; sink = s; });
    bench("范围 for (const auto& x)", [&] { long s = 0; for (const auto& x : v) s += x; sink = s; });
    printf("  → 差距在噪声内：别为性能在下标/迭代器之间纠结，选可读性好的\n");

    // ---------- 2) auto 拷贝陷阱 ----------
    printf("\n===== 2) for (auto x) vs for (auto& x) =====\n");
    struct Big { std::string s; int pad[8]; };
    std::vector<Big> b(3);
    printf("  sizeof(Big) = %zu\n", sizeof(Big));
    for (auto x : b) x.s = "changed";        // 拷贝，改不到
    printf("  for(auto  x : b) 改后 b[0].s = \"%s\"\n", b[0].s.c_str());
    for (auto& x : b) x.s = "changed";       // 引用，改得到
    printf("  for(auto& x : b) 改后 b[0].s = \"%s\"\n", b[0].s.c_str());

    // ---------- 3) size_t 回绕 ----------
    printf("\n===== 3) size_t 逆向循环回绕 =====\n");
    std::vector<int> t{1, 2, 3};
    int guard = 0;
    for (size_t i = t.size() - 1; i >= 0; --i) {      // ❌ i >= 0 恒真
        if (++guard > 5) {
            printf("  第 %d 次 i=%zu → 回绕了，i>=0 永远成立\n", guard, i);
            break;
        }
    }
    printf("  正确写法：for (size_t i = t.size(); i-- > 0; ) { ... }\n");
    return 0;
}
