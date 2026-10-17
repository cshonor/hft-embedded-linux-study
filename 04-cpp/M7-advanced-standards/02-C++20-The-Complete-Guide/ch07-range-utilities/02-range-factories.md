# 范围工厂：iota / counted / all / owning_view

## views::iota——不分配内存的"虚拟容器"

```cpp
#include <ranges>

namespace v = std::views;

// iota(begin)：无限序列 0,1,2,...
// iota(begin, end)：有限序列 [begin, end)
for (int lane : v::iota(0, 64)) {
    pin_to_cpu(lane);   // 绑核循环：没有 vector{0..63}，一个数都不存
}

// 配合 take：无限序列截前 N 个
auto first_100 = v::iota(1) | v::take(100);

// 边界值是类型级的：iota<uint8_t>(0, 300) 编译期报错（300 超出 uint8_t）
```

**本质**：`iota_view` 的迭代器就是一个整数本身，`sizeof` = 一个值。替代"先造 vector<int> 再循环"的写法，零分配零内存流量。

```cpp
// 实战：生成测试行情序列，不落盘
for (auto [i, t] : v::iota(0, 1000)
                  | v::transform([](int i) { return make_tick(i, 100.0 + i * 0.01); })
                  | v::enumerate)          // C++23：带下标
{
    feed.push(t);
    assert(t.seq == i);
}
```

## views::counted——指针 + 长度的轻量包装

```cpp
Tick buf[4096];
size_t n = receive_into(buf);   // 运行期才知道收了多少

// counted(指针迭代器, 个数)：把 (ptr, n) 变成一个 range
auto batch = v::counted(buf, n);

std::ranges::for_each(batch, &FeedHandler::on_tick);
// 等价于 span{buf, n}，但起点可以是任何迭代器而不只是连续内存
```

`counted` vs `span`：counted 接受**任意迭代器**（list 的也行，当然那就失去连续性）；span 仅限连续内存但带静态 extent 和边界信息。连续内存场景优先 span（见 ch09），花式迭代器场景用 counted。

## views::all——统一"把容器变成 view"的入口

```cpp
std::vector<Tick> ticks = load();
std::span<Tick>   window = ticks;

auto a = v::all(ticks);    // ref_view<vector<Tick>>：借一个引用，不拷贝
auto b = v::all(window);   // span 本身已是 view → 原样返回（decay-copy）

// 管道里写容器名时编译器隐式干的就是 v::all：
ticks | v::filter(pred)    // ≡ v::all(ticks) | v::filter(pred)
```

**左值 → ref_view（借引用）；右值 → owning_view（接管所有权）**：

```cpp
auto make() { return std::vector<Tick>{/*...*/}; }

// 右值容器进管道：owning_view 把它 move 进来持有，管道结束才析构
auto big = make() | v::filter([](const Tick& t) { return t.qty > 100; });
// ✅ 安全——没有 owning_view 的话右值 vector 在分号处析构，view 全悬垂
```

这就是"右值容器可以直接进管道"的原理——不是 ranges 宽容，是 `all` 悄悄把所有权接走了。

## owning_view vs 拷贝：所有权语义对照

| 写法 | 发生什么 | 成本 |
|---|---|---|
| `v::all(lvalue)` | ref_view，借引用 | 0 |
| `v::all(view)` | 原样返回 | 0 |
| `v::all(rvalue容器)` | owning_view，move 接管 | 一次 move（容器本身 O(1)） |
| `r::to<vector>(view)`（C++23） | 物化成新 vector | O(n) 分配+拷贝 |

```cpp
// C++23 ranges::to：把 view 的结果"落"成容器——显式的物化点
auto hits = ticks
          | v::filter([](const Tick& t) { return t.qty > 100; })
          | std::ranges::to<std::vector>();   // 在这里才分配

// C++20 没有 ranges::to 时：
std::vector<Tick> hits;
std::ranges::copy(ticks | v::filter(pred), std::back_inserter(hits));
```

纪律：**物化（to/copy）只在需要"攒一批"时做**——能流式消费（for_each/fold）就别落成容器，省一次分配一次拷贝。

## 自测题

1. `iota_view` 的 `sizeof` 大约是多少？为什么它生成百万个"元素"没有内存流量？
2. `views::counted` 和 `span` 的使用场景怎么分？各自对迭代器类别有什么要求？
3. 左值 vector、右值 vector、已有 view 分别被 `views::all` 变成什么？为什么右值容器进管道是安全的？
4. `owning_view` 的存在让哪种 C++17 里必然悬垂的写法变合法了？
5. "物化点"指什么？为什么 `ranges::to` 应该尽量少用？
