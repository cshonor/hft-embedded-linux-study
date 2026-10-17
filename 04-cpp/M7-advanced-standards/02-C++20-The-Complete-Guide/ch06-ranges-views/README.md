# 第 6 章 范围与视图

**Ranges and Views**

## 本章讲什么

C++20 的 **Ranges** 是对 STL 的革命性重构——让算法接受"范围"（begin/end 配对）而非裸迭代器，支持**惰性组合**（`v | filter | transform`），替代手写循环和嵌套算法调用。

## 要点

### 从迭代器到范围

```cpp
// C++17：算法要传 begin/end
std::sort(v.begin(), v.end());
auto it = std::find(v.begin(), v.end(), 42);

// C++20：算法接受范围
std::ranges::sort(v);
auto it = std::ranges::find(v, 42);
```

范围 = 有 `begin()` 和 `end()` 的东西（容器、数组、string、自定义）。

### 视图：惰性组合

```cpp
// C++17：要中间容器
std::vector<int> tmp;
std::copy_if(v.begin(), v.end(), std::back_inserter(tmp), is_positive);
std::vector<int> result;
std::transform(tmp.begin(), tmp.end(), std::back_inserter(result), square);

// C++20 Ranges：管道组合，无中间容器
auto result = v
    | std::views::filter(is_positive)
    | std::views::transform(square);

for (int x : result) { /* 惰性求值 */ }
```

视图是**惰性**的——不立即计算，遍历时才逐元素处理。无中间容器分配，零额外内存。

### 视图的特点

| 特性 | 说明 |
|------|------|
| 惰性求值 | 遍历时才计算，不预生成 |
| 零拷贝 | 不拥有数据，引用底层范围 |
| 可组合 | `|` 管道连接 |
| 轻量 | 视图对象通常只存迭代器/指针 |

### 常用视图

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};

// filter：过滤
auto pos = v | std::views::filter([](int x){ return x > 0; });

// transform：变换
auto sq = v | std::views::transform([](int x){ return x*x; });

// take：取前 N 个
auto first3 = v | std::views::take(3);   // {1,2,3}

// drop：跳过前 N 个
auto rest = v | std::views::drop(2);      // {3,4,5}

// reverse：反转
auto rev = v | std::views::reverse;       // {5,4,3,2,1}

// iota：生成序列
auto seq = std::views::iota(1, 10);       // 1..9

// keys/values：map 的键/值
for (auto& k : map | std::views::keys) { ... }
```

### 与 Concepts 的结合

```cpp
// Ranges 大量用 Concept 约束
template <std::ranges::input_range R>
void process(R&& r);

template <std::ranges::random_access_range R>
void sort(R&& r);   // 要求随机访问
```

算法通过 Concept 自动选择最优实现。

### Ranges 的迭代器分类

C++20 把迭代器 Concept 标准化：
- `input_iterator` / `output_iterator`
- `forward_iterator`
- `bidirectional_iterator`
- `random_access_iterator`
- `contiguous_iterator`

算法用 Concept 约束，编译期选最优实现。

## HFT 关联

- **行情批处理管道**：`ticks | filter(valid) | transform(parse) | take(top_n)` 一行组合，无中间容器。
- **零分配热路径**：视图惰性求值不分配中间 vector，热路径可控。
- **map 遍历优化**：`handlers | views::values` 只遍历值，不写 `for (auto& [k,v] : handlers)` 丢弃 k。
- **滑窗计算**：`views::slide(N)` 做滑动窗口指标（移动平均），无手写循环。
- **慎用复杂管道在纳秒热路径**：视图组合有迭代器层层包装，极高频热路径可能不如手写循环——需实测。
- **回测/批处理适合**：离线数据分析大量用 Ranges，代码简洁、无中间分配。

## 自测题

1. Ranges 相比传统 STL 算法的两个核心改进是什么？
2. 视图（View）的"惰性"是什么意思？为什么零拷贝？
3. `v | views::filter(f) | views::transform(g)` 的求值时机？
4. Ranges 的迭代器 Concept 有哪些？算法如何利用？
5. HFT 热路径为什么慎用复杂 Ranges 管道？什么场景适合？

## 代码自测

### Q1: 管道式 views
```cpp
std::vector<int> v = {1, 2, 3, 4, 5, 6};

auto result = v
    | std::views::filter([](int x) { return x % 2 == 0; })  // {2, 4, 6}
    | std::views::transform([](int x) { return x * x; })     // {4, 16, 36}
    | std::views::take(2);                                     // {4, 16}

for (int x : result) std::cout << x << ' ';  // 4 16
```
> views 是惰性的吗？filter/transform/take 各做什么？

<details>
<summary>答案与复习指引</summary>

**是的**，views 是惰性的（lazy evaluation）：
- 管道只是创建"视图适配器"，不立即计算
- 迭代时才按需计算每个元素
- 无中间容器分配（零拷贝）

| View | 作用 |
|------|------|
| `filter(pred)` | 只保留满足谓词的元素 |
| `transform(f)` | 对每个元素应用 f |
| `take(n)` | 取前 n 个 |
| `drop(n)` | 跳过前 n 个 |
| `reverse()` | 反转 |
| `take_while(pred)` | 取到第一个不满足的为止 |

**vs 传统算法**：
```cpp
// 传统：需要临时容器
std::vector<int> evens;
std::copy_if(v.begin(), v.end(), std::back_inserter(evens), ...);
std::vector<int> squares;
std::transform(evens.begin(), evens.end(), std::back_inserter(squares), ...);

// Ranges：无临时容器，惰性链式
auto result = v | views::filter(...) | views::transform(...);
```

**复习：** → [Ranges Views](./README.md)
</details>
