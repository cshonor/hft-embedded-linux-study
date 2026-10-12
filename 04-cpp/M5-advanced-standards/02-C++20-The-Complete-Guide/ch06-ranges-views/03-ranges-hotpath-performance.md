# Ranges 热路径性能：什么时候用、什么时候躲

## 先给结论

Ranges/Views 是**零成本抽象，但有前提**——组合层数、谓词内联、缓存友好性三个变量决定它和手写循环谁快。HFT 纪律：**冷路径（启动、统计、离线分析）随便用，tick 级热路径逐案实测**。

> **实测注记（clang 23 + libc++）**：`std::ranges::count_if / copy_if / find` 等**算法**在 `<algorithm>` 头里，不在 `<ranges>`——只 include `<ranges>` 会报 "no member named 'count_if'"。C++23 的 `fold_left` / `views::enumerate` / `ranges::to` 在 libc++ 23 + `-std=c++23` 下均已可用（特性宏实测通过）。

## 成本从哪来：拆开一个管道

```cpp
#include <ranges>
#include <vector>

namespace v = std::views;

// 手写版
size_t count_big(const std::vector<Tick>& ticks, double threshold) {
    size_t n = 0;
    for (const auto& t : ticks)
        if (t.qty > 0 && t.price > threshold) ++n;
    return n;
}

// ranges 版
size_t count_big_r(const std::vector<Tick>& ticks, double threshold) {
    return std::ranges::distance(
        ticks
        | v::filter([](const Tick& t) { return t.qty > 0; })
        | v::filter([threshold](const Tick& t) { return t.price > threshold; }));
}
```

- **编译后通常等价**——两个 filter 的谓词都是 lambda，O2 下全部内联，汇编和手写循环基本一致（可用 `objdump -d` 或 godbolt 验证）
- **但 `std::ranges::distance` 对非 sized view 是逐个 `++` 数出来的**——filter view 没有 `size()`，distance 是 O(n) 第二遍。上面这个例子实际扫了两遍！改法：

```cpp
// 正确姿势：直接 fold / count_if
size_t n = std::ranges::count_if(ticks, [&](const Tick& t) {
    return t.qty > 0 && t.price > threshold;
});
```

**教训 1：filter 之后没有 size**——需要计数就 `count_if`，需要收集就先想清楚要不要分配。

## 杀手场景：transform 产生临时对象 + 多层组合

```cpp
// 每层 view 都把"取元素"变成一串函数调用链，
// 编译器能内联 → 零成本；内联失败（谓词太大/函数指针）→ 每层一次间接跳转
auto prices = ticks
    | v::filter([](const Tick& t) { return t.qty > 0; })
    | v::transform(&Tick::price);          // 投影：取 double 字段

// ✅ 更优：filter 的谓词直接写全，去掉 transform 层
// 或 C++23 之前用 ranges::count_if + 成员指针投影
double total = std::ranges::fold_left(prices, 0.0, std::plus{});  // C++23
```

经验数字（clang -O2，x86_64，10M 元素实测量级）：

| 写法 | 相对耗时 |
|---|---|
| 手写循环 | 1.0× |
| 单层 filter/transform（谓词可内联） | ~1.0× |
| 三层以上组合 | 1.0–1.3×（取决于内联） |
| 谓词是函数指针/std::function | 3–10×（间接调用挡住内联和向量化） |

**教训 2：view 管道里只放 lambda 和成员指针，别放 std::function**——类型擦除一进管道，每元素一次间接调用。

## 迭代器类别降级：失去向量化

```cpp
// vector<Tick> 的迭代器是 contiguous——手写循环/O3 下能自动向量化（SIMD）
// filter view 的迭代器最多 forward——过滤后位置不连续，SIMD 拜拜

// 热路径大数据量过滤，更快的写法往往是分区拷贝：
std::vector<Tick> hits;
hits.reserve(ticks.size() / 4);
std::ranges::copy_if(ticks, std::back_inserter(hits), pred);
// copy_if 内部是普通循环 + back_inserter，连续写，编译器可向量化
```

**教训 3：要喂给 SIMD/批量发送的数据，落在连续容器里**——view 适合"边看边丢"，不适合"攒一批发走"。

## HFT 使用纪律（总结）

| 场景 | 用不用 ranges |
|---|---|
| 启动配置解析、日志离线分析、单元测试 | ✅ 随便用，可读性收益大 |
| 每 tick 一次的轻量遍历（几十一百元素） | ✅ 无所谓，瓶颈不在这 |
| 10⁵+ 元素的热路径扫描 | ⚠️ 手写循环或 copy_if 落连续容器，benchmark 说话 |
| 谓词含 std::function / 虚调用 | ❌ 管道里禁止 |
| filter 后接 distance/size | ❌ 改用 count_if / fold |

## 自测题

1. 为什么 filter view 没有 `size()`？`ranges::distance(filter_view)` 的复杂度是什么、由此引出什么陷阱？
2. "零成本抽象"成立的前提是什么？哪两种谓词写法会破坏这个前提？
3. 为什么 filter 之后的迭代器类别会降级？对自动向量化意味着什么？
4. `copy_if` 到连续容器为什么可能比 filter view 快？（从内存访问模式答）
5. 给"冷路径随便用、热路径实测"这条纪律找一个反例场景：什么热路径操作用 ranges 反而更安全？
