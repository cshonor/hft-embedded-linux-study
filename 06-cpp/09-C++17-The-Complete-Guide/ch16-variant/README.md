# 第 16 章 std::variant

**std::variant<>**

## 本章讲什么

`std::variant<T1, T2, ...>` 是类型安全的联合体（tagged union）——持有某一时刻的多种可能类型之一，编译期知道所有可能类型。替代继承多态和裸 union。

## 要点

### 基本用法

```cpp
#include <variant>

using Msg = std::variant<Tick, Trade, OrderBook>;

Msg m = Tick{...};

// 1. 访问（需知道当前类型）
Tick& t = std::get<Tick>(m);   // 类型错抛 bad_variant_access

// 2. 检查类型
if (std::holds_alternative<Tick>(m)) { ... }

// 3. 访问器（推荐）
std::visit([](auto&& msg) {
    using T = std::decay_t<decltype(msg)>;
    if constexpr (std::is_same_v<T, Tick>) { handle_tick(msg); }
    else if constexpr (std::is_same_v<T, Trade>) { handle_trade(msg); }
    else { handle_book(msg); }
}, m);
```

### `std::visit` + overloaded 模式

```cpp
// C++17 overloaded 技巧（结构体 + 变长 using）
struct Visitor {
    void operator()(const Tick& t) { handle_tick(t); }
    void operator()(const Trade& t) { handle_trade(t); }
    void operator()(const OrderBook& b) { handle_book(b); }
};
std::visit(Visitor{}, m);

// 更简洁的 lambda 版（需 helper）
template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

std::visit(overloaded{
    [](const Tick& t) { handle_tick(t); },
    [](const Trade& t) { handle_trade(t); },
    [](const OrderBook& b) { handle_book(b); }
}, m);
```

### 存储与开销

- `sizeof(variant<T1,...,Tn>)` ≈ `max(sizeof(Ti)) + tag`（对齐填充）。
- **无堆分配**——值内联。
- tag 是一个整数（类型索引），访问时编译器生成跳表。
- 比虚函数快（无 vtable 间接、无虚析构），但有 tag 检查 + 跳表。

### variant vs 虚函数 vs union

| 方案 | 优点 | 缺点 |
|------|------|------|
| `variant` + visit | 值语义、无堆分配、编译期类型集 | 访问器写法稍繁、类型集固定 |
| 继承 + virtual | 类型集可扩展、运行期多态 | 堆分配（通常）、vtable 间接、虚析构 |
| 裸 union | 零开销 | 手动管 tag、不安全 |

### `std::get_if` 非抛异常访问

```cpp
if (auto* p = std::get_if<Tick>(&m)) {
    handle_tick(*p);   // 不抛异常，返回 nullptr 表示类型不符
}
```

## HFT 关联

- **消息体用 variant**：行情消息 `variant<Tick, Trade, OrderBook>` 替代继承基类 `Message*`——值语义、无堆分配、cache 友好。
- **`visit` + `if constexpr` 分派**：编译期展开，无 vtable，比虚函数 `process(msg)` 快。
- **无堆分配**：variant 内联存储，SPSC 队列传 variant 无堆分配，热路径可控。
- **`overloaded` lambda 模式**：消息处理用 overloaded 写每个类型的处理逻辑，集中清晰。
- **`get_if` 热路径**：确定类型时用 `get_if` 非抛异常访问，比 `get` 安全无开销。
- **替代 `any`**：类型集已知用 variant（编译期类型安全），类型集未知才用 any（运行期 type_info）。

## 自测题

1. `variant` 和继承多态相比有什么优势？为什么无堆分配？
2. `std::visit` 的作用是什么？`overloaded` 模式如何简化访问器？
3. `std::get<T>` 和 `std::get_if<T>` 的区别？热路径用哪个？
4. variant 的存储大小怎么算？tag 是什么？
5. HFT 消息体为什么用 `variant<Tick, Trade, OrderBook>` 而非 `Message*` 继承？
