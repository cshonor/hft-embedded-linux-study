# Item 49：学会解读 STL 错误信息

> 第 7 章 使用 STL 编程 · Item 49 · 上一节：[Item 48 include 大小写](item48-include-case-sensitivity.md) · 下一节：[Item 50 参考资源](item50-reference-resources.md)

## 为什么要学这个（先建立直觉）

在 C 里，编译器错误信息通常很短——`printf` 参数不对就是一行警告。C++ 模板错误信息动辄数百行，因为模板实例化会展开整个类型链。

```c
/* C: 错误信息简短 */
int main() {
    int x = "hello";  // warning: initialization of 'int' from 'char *'
    return 0;
}
```

```cpp
// C++: 模板错误信息巨大
#include <vector>
#include <algorithm>

int main() {
    std::vector<int> v = {3, 1, 4};
    std::sort(v.begin(), v.end(),
        [](int a, int b) { return a < b; });  // 少了一个参数？
    // GCC 错误信息可能超过 50 行
    // 包含 std::sort 的模板展开、迭代器类型、lambda 闭包类型...
    return 0;
}
```

**直觉**：STL 模板错误信息看起来吓人，但有固定模式。学会从最内层找你自己的代码，忽略标准库内部的模板展开。

## 这节讲什么

### 读错误信息的策略

**从内到外**：错误信息通常是一个嵌套的类型链，最内层是你代码出问题的地方。

```
// 典型 STL 错误结构（简化）：
error: no matching function for call to 'sort(
    std::vector<int>::iterator,
    std::vector<int>::iterator,
    <lambda>)'
note: candidate template ignored: substitution failure
    [with _Iter = std::vector<int>::iterator,
          _Compare = main()::<lambda(int, int)>]
```

读法：
1. 找 `error:` 行 → 知道是 "no matching function for sort"
2. 看参数列表 → sort 收到了两个迭代器和一个 lambda
3. 看 `note:` → 模板替换失败，原因在 substitution failure 中
4. 检查 lambda 签名和 sort 期望的比较器签名是否匹配

### 常见错误模式

#### 模式 1：类型不匹配

```cpp
std::vector<int> v = {1, 2, 3};
std::string s = "hello";
std::find(v.begin(), v.end(), s);  // int 容器里找 string？
// error: no matching function for call to 'find(...)'
//   note: couldn't deduce template parameter '_ValueType'
//   （vector<int> 的元素是 int，但你在找 string）
```

#### 模式 2：缺少比较器

```cpp
struct Order { int price; };
std::vector<Order> orders = {{100}, {50}, {200}};
std::sort(orders.begin(), orders.end());  // Order 没有 operator<
// error: no match for 'operator<'
//   note: operand types are 'Order' and 'Order'
```

**修复**：提供比较器。

```cpp
std::sort(orders.begin(), orders.end(),
    [](const Order& a, const Order& b) { return a.price < b.price; });
```

#### 模式 3：const 违规

```cpp
const std::vector<int> v = {1, 2, 3};
auto it = v.begin();  // it 是 const_iterator
*it = 10;  // error: assignment of read-only location
```

### static_assert 植入自定义诊断

```cpp
#include <type_traits>

template<typename T>
void process(const T& value) {
    static_assert(std::is_integral_v<T>,
        "process() requires an integral type");
    // 如果 T 不是整数类型，错误信息直接显示你的消息
    // 而不是几十行模板展开
}

// process("hello");  // error: static_assert failed:
                    //   "process() requires an integral type"
```

### C++20 Concepts：更清晰的错误

```cpp
// C++20: concept 约束模板参数
#include <concepts>

template<std::integral T>
void process(T value) { /* ... */ }

// process("hello");  // error: constraints not satisfied
//   'const char*' does not satisfy 'integral'
```

## 常见错误（新手踩坑）

### 错误 1：被错误信息长度吓到

```cpp
// 50+ 行错误信息 → 新手直接放弃
// 实际上只需要看第一行 error: 和你的代码行号
```

**策略**：看第一个 `error:` 行和 `note:` 中的实际类型。

### 错误 2：看不懂迭代器类型名

```cpp
// 错误信息中出现：
// std::_Vector_base<int, std::allocator<int>>::_Vector_impl
// 这其实就是 std::vector<int> 的内部实现
```

**策略**：识别常见缩写——`_Vector_base` = vector 内部，`__normal_iterator` = vector::iterator，`basic_string` = string。

### 错误 3：忽略 note 中的行号

```cpp
// error 信息指向 <algorithm> 头文件中的 sort 声明
// 但真正的问题在你的调用点（note 中标注）
```

**策略**：看 `note:` 中的 "in instantiation of..." 行，找你代码的文件名和行号。

## 新手要点（和 C 的区别）

| 方面 | C | C++ |
|------|---|-----|
| 错误信息长度 | 短（1-3 行） | 长（10-100+ 行） |
| 原因 | 无模板 | 模板实例化展开 |
| 定位 | 直接看行号 | 从内层找你的代码 |
| 自定义诊断 | assert 宏 | static_assert / concepts |

## HFT 关联

- **static_assert 植入诊断**：策略接口用 `static_assert` 约束类型，错误信息直接指向误用
- **C++20 Concepts**：模板参数约束用 concept，错误信息比 SFINAE 清晰百倍
- **编译时间**：模板错误不只难读，也增加编译时间——合理包含头文件减少实例化

## 代码自测

### Q1: 类型不匹配

```cpp
std::vector<int> v = {1, 2, 3};
std::string target = "hello";
auto it = std::find(v.begin(), v.end(), target);
```
> 编译错误信息的核心问题是什么？

<details>
<summary>答案</summary>

**核心问题**：`std::find` 的第三参数类型（`std::string`）与容器元素类型（`int`）不匹配。

`find` 期望第三参数能和 `*v.begin()`（int）比较，但 `string` 和 `int` 之间没有 `operator==`。

错误信息可能显示：
```
error: no matching function for call to 'find(...)'
note: couldn't infer template parameter
```

**修复**：确保查找值类型匹配容器元素类型。
```cpp
auto it = std::find(v.begin(), v.end(), 2);  // int
```
</details>

### Q2: 缺少 operator<

```cpp
struct Order { int price; };
std::vector<Order> orders = {{100}, {50}};
std::sort(orders.begin(), orders.end());
```
> 错误信息的核心是什么？怎么修？

<details>
<summary>答案</summary>

**核心**：`std::sort` 需要比较 `Order` 对象，但 `Order` 没有 `operator<`。

错误信息类似：
```
error: no match for 'operator<' (operand types are 'Order' and 'Order')
```

**修复方式 1**：提供比较器
```cpp
std::sort(orders.begin(), orders.end(),
    [](const Order& a, const Order& b) { return a.price < b.price; });
```

**修复方式 2**：给 Order 定义 operator<
```cpp
struct Order {
    int price;
    bool operator<(const Order& o) const { return price < o.price; }
};
```
</details>

### Q3: static_assert

```cpp
template<typename T>
T square(T x) {
    static_assert(std::is_arithmetic_v<T>, "square requires arithmetic type");
    return x * x;
}

// 调用
auto r1 = square(5);       // A
auto r2 = square(3.14);    // B
auto r3 = square("hi");    // C
```
> A、B、C 哪个会触发 static_assert？

<details>
<summary>答案</summary>

- **A `square(5)`**：✅ `int` 是 arithmetic 类型，正常编译
- **B `square(3.14)`**：✅ `double` 是 arithmetic 类型，正常编译
- **C `square("hi")`**：❌ `const char*` 不是 arithmetic 类型，触发 static_assert

C 的错误信息：
```
error: static_assert failed: "square requires arithmetic type"
```

**static_assert 的价值**：错误信息直接显示你的自定义消息，而不是几十行模板展开。比 SFINAE 更直观。
</details>

### Q4: 读嵌套错误

```
error: no matching function for call to 'transform(
    std::vector<int>::iterator,
    std::vector<int>::iterator,
    std::back_insert_iterator<std::vector<std::string>>,
    <lambda(int)>)'
note: couldn't deduce template parameter '_OutputType'
```
> 这段错误信息的核心问题是什么？

<details>
<summary>答案</summary>

**核心问题**：lambda 返回类型和目标容器类型不匹配。

`transform` 的签名：把输入范围的元素变换后输出到目标。这里：
- 输入：`vector<int>`（元素是 int）
- lambda：`lambda(int)` — 输入 int
- 输出：`back_insert_iterator<vector<string>>` — 目标容器是 `vector<string>`

但 lambda 返回 int（或推导类型），插入 `vector<string>` 时类型不匹配 → 无法推导 `_OutputType`。

**修复**：让 lambda 返回 `std::string`，或改目标容器为 `vector<int>`。

```cpp
// 修复 1：lambda 返回 string
std::transform(v.begin(), v.end(), std::back_inserter(strs),
    [](int x) { return std::to_string(x); });

// 修复 2：目标容器改为 vector<int>
std::transform(v.begin(), v.end(), std::back_inserter(result),
    [](int x) { return x * 2; });
```
</details>

## 参考与延伸

- 上一节：[Item 48 include 大小写](item48-include-case-sensitivity.md)
- 下一节：[Item 50 参考资源](item50-reference-resources.md)
- Effective Modern C++ ch05 type traits
