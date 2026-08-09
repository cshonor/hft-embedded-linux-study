# 第 17 章 标准库特殊设施

日常开发不高频、但关键场景必备的小众标准库工具，不属于基础语法。

## 小节

- [17.1 tuple 元组](./17.1-tuple元组.md)
- [17.2 bitset 位集](./17.2-bitset位集.md)
- [17.3 正则表达式 regex](./17.3-正则表达式regex.md)
- [17.4 随机数](./17.4-随机数.md)
- [17.5 计时工具（chrono）](./17.5-计时工具（chrono）.md)
- [17.6 可选：特殊数值类型](./17.6-可选：特殊数值类型.md)
- [学习优先级](./17.7-学习优先级.md)


## 章节摘要

标准库特殊设施：`tuple`（元组）、`bitset`（位集）、`regex`（正则表达式）、随机数库（`random`）、计时工具（`chrono`）、特殊数值类型（`optional`/`variant`/`any`，C++17）。

### 和 C 的区别

| C | C++ |
|---|-----|
| 无 tuple | `std::tuple` |
| 位运算手写 | `std::bitset`（自带转字符串/翻转） |
| `regex.h` POSIX 正则 | `std::regex`（ECMAScript 语法） |
| `rand()` 质量差 | `<random>` 引擎+分布（高质量随机数） |
| `clock()`/`time()` | `std::chrono`（高精度+类型安全） |

## 章节自测

### Q1: tuple 解包

```cpp
#include <tuple>
auto t = std::make_tuple(42, "hello", 3.14);
// 方式 A
int i; const char *s; double d;
std::tie(i, s, d) = t;
// 方式 B (C++17)
auto [a, b, c] = t;
std::cout << a << " " << b << " " << c;
```

> 方式 B 的输出是什么？`tie` 和结构化绑定有什么区别？

<details>
<summary>答案与复习指引</summary>

**输出：** `42 hello 3.14`

**区别：**
- `std::tie` 需要先声明变量再绑定，语法冗长
- 结构化绑定 `auto [a,b,c] = t` 一步到位，更简洁
- 结构化绑定还可以用于 `struct`/`array`/`pair`，`tie` 只能用于 `tuple`

**复习：** → [17.1 tuple 元组](./17.1-tuple元组.md)
</details>

### Q2: bitset

```cpp
std::bitset<8> b(42);
std::cout << b.to_string() << " " << b.count() << " " << b[1];
```

> 输出是什么？`42` 的二进制是什么？

<details>
<summary>答案与复习指引</summary>

**输出：** `00101010 3 1`

**解析：**
- `42` = `0b00101010`（8 位二进制）
- `to_string()` = `"00101010"`
- `count()` = 3（有 3 个 1）
- `b[1]` = 1（从右往左第 2 位是 1）

**和 C 的区别：** C 要手写位运算 `(42 >> 1) & 1` 来检查某位，`bitset` 直接 `b[1]` 更直观，还有 `count()`/`flip()`/`to_string()` 等便利方法。

**复习：** → [17.2 bitset 位集](./17.2-bitset位集.md)
</details>

### Q3: chrono 计时

```cpp
#include <chrono>
auto start = std::chrono::high_resolution_clock::now();
// ... 做一些工作 ...
auto end = std::chrono::high_resolution_clock::now();
auto ms = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
std::cout << ms.count() << " us";
```

> `ms.count()` 返回什么？`chrono` 比 C 的 `clock()` 好在哪里？

<details>
<summary>答案与复习指引</summary>

**`ms.count()` 返回**微秒数（`long long` 类型），表示两端点之间的时间差。

**`chrono` 优势：**
1. **类型安全**：不同时间单位是不同类型，不会意外混淆微秒和毫秒
2. **高精度**：`high_resolution_clock` 通常是纳秒级，`clock()` 精度低（毫秒级）
3. **可移植**：跨平台一致 API

**HFT 用途：** 测量策略处理延迟、行情处理耗时。`chrono::steady_clock` 用于间隔测量（不受系统时间调整影响）。

**复习：** → [17.5 计时工具（chrono）](./17.5-计时工具（chrono）.md)
</details>

### Q4: random 引擎

```cpp
#include <random>
std::mt19937 gen(42);  // 固定种子
std::uniform_int_distribution<int> dist(1, 100);
std::cout << dist(gen) << " " << dist(gen);
```

> 输出固定吗？为什么不用 `rand()`？

<details>
<summary>答案与复习指引</summary>

**输出固定：** 种子固定为 42，每次运行输出相同序列（`7 49` 在大多数实现中，但具体值取决于实现）。

**不用 `rand()` 的原因：**
1. **质量差**：`rand()` 周期短、分布不均匀（低位熵低）
2. **不可控分布**：`rand() % 100` 有模偏差，不是均匀分布
3. **不可重现**：`rand()` 依赖全局状态（非线程安全），`mt19937` 是独立对象（线程安全）

**HFT 用途：** 回测模拟、压力测试数据生成——用固定种子保证可重现性。

**复习：** → [17.4 随机数](./17.4-随机数.md)
</details>
