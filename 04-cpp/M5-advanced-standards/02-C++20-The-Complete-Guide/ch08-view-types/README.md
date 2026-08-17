# 第 8 章 深入理解视图类型

**View Types in Detail**

## 本章讲什么

详述 C++20 内置视图的类型：filter、transform、take/drop、reverse、iota、join、split、elements、keys/values、adjacent/slide 等。理解每个视图的语义、复杂度和适用场景。

## 要点

### 视图分类

| 类别 | 视图 |
|------|------|
| 过滤/变换 | `filter`、`transform` |
| 切片 | `take`、`take_while`、`drop`、`drop_while` |
| 反转/生成 | `reverse`、`iota`、`repeat`、`empty`、`single` |
| 拆分/合并 | `join`、`split`、`join_with`（C++23） |
| 元素访问 | `elements<N>`、`keys`、`values` |
| 窗口 | `adjacent<N>`、`slide<N>`、`chunk(N)`（C++23） |
| 去重/唯一 | `unique`（C++20 算法非视图） |

### 各视图详解

```cpp
std::vector<int> v = {1, 2, 3, 4, 5, 6};

// filter：保留满足谓词的
auto evens = v | views::filter([](int x){ return x % 2 == 0; });
// {2, 4, 6}

// transform：逐元素变换
auto sq = v | views::transform([](int x){ return x * x; });
// {1, 4, 9, 16, 25, 36}

// take / take_while
auto first3 = v | views::take(3);              // {1, 2, 3}
auto until4 = v | views::take_while([](int x){ return x < 4; });  // {1, 2, 3}

// drop / drop_while
auto rest = v | views::drop(2);                 // {3, 4, 5, 6}

// reverse
auto rev = v | views::reverse;                  // {6, 5, 4, 3, 2, 1}

// iota：生成序列
auto seq = views::iota(0, 10);                  // 0..9
auto inf = views::iota(0);                       // 无限 0,1,2,...（配合 take 用）

// join：展平嵌套范围
std::vector<std::vector<int>> nested = {{1,2},{3,4}};
auto flat = nested | views::join;               // {1, 2, 3, 4}

// split：按分隔符拆分
std::string s = "a,b,c";
auto parts = s | views::split(',') | views::transform([](auto r){ return std::string(r.begin(), r.end()); });
// {"a", "b", "c"}

// elements<N>：取 tuple/pair 的第 N 元素
std::vector<std::pair<int,std::string>> pv = {{1,"a"},{2,"b"}};
auto firsts = pv | views::elements<0>;          // {1, 2}
auto keys = pv | views::keys;                    // 同上
auto values = pv | views::values;                // {"a", "b"}

// slide（C++23）：滑动窗口
auto windows = v | views::slide(3);
// {1,2,3}, {2,3,4}, {3,4,5}, {4,5,6}
```

### 复杂度与缓存

| 视图 | 复杂度 | 缓存 |
|------|--------|------|
| `transform` | O(1) 每元素 | 不缓存（每次遍历都算） |
| `filter` | O(n) 遍历找下一个 | 不缓存 |
| `take`/`drop` | O(1) | - |
| `reverse` | 双向迭代器 | 不缓存 |
| `iota` | O(1) 每元素 | - |
| `join` | O(1) 每元素 | - |

视图**不缓存**——每次遍历都重新计算。多次遍历同一视图会重复计算。

### 视图的可复制性

视图是轻量的（迭代器+哨兵），可随意拷贝。但**引用底层范围**——底层范围销毁后视图悬垂。

### 无限视图

```cpp
// iota 无上限
auto naturals = views::iota(1);   // 1, 2, 3, ...
auto first10 = naturals | views::take(10);   // 安全：取前 10
// 不能直接 for (int x : naturals) {} —— 无限循环
```

无限视图配合 `take`/`take_while` 使用，做惰性生成。

## HFT 关联

- **`filter` + `transform` 行情管道**：`ticks | filter(valid) | transform(normalize)` 零分配处理。
- **`slide` 做移动平均**：`prices | views::slide(N) | transform(avg)` 滑动窗口算指标（C++23，C++20 要手写）。
- **`split` 解析 FIX 字段**：`fix_msg | views::split('|')` 拆分字段，零拷贝。
- **`elements<N>` 取 pair 字段**：`orders | views::elements<0>` 取所有订单 ID。
- **`iota` 生成序号**：`views::iota(0) | take(N)` 给元素编号，替代手写循环计数。
- **视图不缓存的注意**：热路径多次遍历同一视图会重复计算——要缓存结果用 `ranges::to<vector>`（C++23）或手写循环。

## 自测题

1. `filter` 和 `transform` 视图的语义区别？
2. `take_while` 和 `take` 的区别？`drop_while` 呢？
3. `join` 和 `split` 是什么关系？
4. 视图为什么不缓存？多次遍历有什么后果？
5. HFT 如何用 `split` 解析 FIX 消息？用 `slide` 做移动平均（C++23）？

## 代码自测

### Q1: 常用 view 类型
```cpp
std::vector<int> v = {1, 2, 3, 4, 5};
std::map<int, std::string> m = {{1, "a"}, {2, "b"}};

// keys/values 视图
for (int k : m | std::views::keys) std::cout << k;  // 12
for (auto& val : m | std::views::values) std::cout << val;  // ab

// elements<N>：取 tuple/pair 的第 N 个
for (int k : m | std::views::elements<0>) std::cout << k;  // 12

// split/join
std::string s = "a,b,c";
for (auto part : s | std::views::split(',')) {
    // "a", "b", "c"
}

// adjacent<N>：滑动窗口
for (auto [a, b] : v | std::views::adjacent<2>) {
    std::cout << a << b << ' ';  // 12 23 34 45
}
```
> split 和 adjacent 分别做什么？views 是拥有数据还是引用数据？

<details>
<summary>答案与复习指引</summary>

- **`split(delim)`**：按分隔符切分范围为子范围
- **`adjacent<N>`**（C++23，C++20 用 `slide`）：滑动窗口，每窗口 N 个相邻元素

**views 引用数据**：views 不拥有底层数据，只是"视图"——引用原容器的元素。如果原容器销毁/修改，view 失效（类似 `string_view`）。

**安全规则**：
- view 的生命周期不能超过底层数据
- 通过 view 修改元素会改原数据（如果 view 不是 const）
- `filter`/`transform` 创建新迭代器但仍引用原数据

**复习：** → [View 类型](./README.md)
</details>
