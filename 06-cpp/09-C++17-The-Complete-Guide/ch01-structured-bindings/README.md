# 第 1 章 结构化绑定

**Structured Bindings**

## 本章讲什么

C++17 让你一条语句把结构体/数组/pair 的多个成员"解包"到独立变量，替代 `std::tie` 或手写 `.first/.second`。

## 要点

### 三种绑定形式

```cpp
// 1. 结构体/类的公有成员
struct Point { int x, y; };
Point p{1, 2};
auto [x, y] = p;          // x=1, y=2，拷贝

// 2. 数组 / tuple-like
int arr[3] = {1, 2, 3};
auto [a, b, c] = arr;

std::pair pr{1, "hi"};
auto [k, v] = pr;

// 3. map 迭代
for (const auto& [key, val] : mymap) { ... }
```

### 值语义细节

```cpp
auto [x, y] = p;     // x,y 是 p 成员的拷贝
auto& [x, y] = p;    // x,y 是 p 成员的引用（可改 p）
const auto& [x, y] = p;  // 只读引用
```

`auto [x,y] = p` 中 `x,y` 的类型由编译器从 `p` 推导——`auto` 的对象其实是**匿名变量**（编译器内部叫 `e`），`x,y` 是它的成员引用/别名。这就是为什么 `auto [x,y] = p` 后改 `x` 不会改 `p`（除非用 `auto&`）。

### 不能完全适用的场景

- 结构体成员**非公有**（有私有成员的类不能绑定全部成员）。
- 绑定的成员数必须和声明变量数一致。
- C++17 不能用 `[[maybe_unused]]` 标记未用绑定变量（C++26 才行），用 `std::ignore` 替代 `std::tie` 仍有效。

## HFT 关联

- **解包 tick/order 字段**：`auto [price, qty, side] = tick;` 比 `tick.price / tick.qty` 简洁，少写临时变量。
- **map 迭代合约表**：`for (auto& [sym, cb] : handlers)` 订阅表遍历更直观。
- **配合 `optional`/`variant`**：`if (auto [ok, val] = lookup(); ok)` 风格（C++17 也有 `if (auto it = m.find(k); it != m.end())`）。
- **零开销**：编译期完成，运行时和手写 `.x/.y` 等价。

## 自测题

1. `auto [x,y] = p` 和 `auto& [x,y] = p` 的区别？为什么前者改 `x` 不影响 `p`？
2. 结构化绑定的三种形式分别适用于什么类型？
3. 哪些情况下不能用结构化绑定？
4. `for (const auto& [k,v] : map)` 比传统迭代器写法好在哪里？
5. HFT 解包 tick 字段为什么用结构化绑定而不是 `std::tie`？
