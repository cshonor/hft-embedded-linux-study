# 第 15 章 std::optional

**std::optional<>**

## 本章讲什么

`std::optional<T>` 表示"可能有值也可能没有"的可空值，替代裸指针（`nullptr` 表示无）、哨兵值（`-1` 表示无）、`bool + T` 双字段等 hack。

## 要点

### 基本用法

```cpp
#include <optional>

std::optional<int> lookup(const std::string& key) {
    auto it = map.find(key);
    if (it != map.end()) return it->second;
    return std::nullopt;   // 空
}

auto v = lookup("foo");
if (v) {
    use(*v);        // 解引用
} else {
    // 无值
}

int x = v.value_or(0);   // 有值取值，无值取 0
int y = v.value();       // 有值取值，无值抛 bad_optional_access
```

### 核心接口

| 操作 | 说明 |
|------|------|
| `opt` / `opt.has_value()` | 是否有值 |
| `*opt` | 解引用（不检查，未定义行为风险） |
| `opt.value()` | 解引用（无值抛异常） |
| `opt.value_or(def)` | 有值取值，无值取 def |
| `opt = nullopt` | 清空 |
| `opt.emplace(args...)` | 原地构造值 |
| `opt.reset()` | 销毁值 |

### 存储布局

`optional<T>` 内部是一个 `T` 的存储（用 `aligned_storage` 或 union）+ 一个 bool 标志。**不额外分配堆内存**——值内联在 optional 对象中。

```cpp
// sizeof(optional<int>) 通常是 8（int + 4 padding + 1 bool + padding）
// sizeof(optional<BigStruct>) = sizeof(BigStruct) + 对齐填充 + bool
```

### 与其他可空方案的对比

| 方案 | 优点 | 缺点 |
|------|------|------|
| `optional<T>` | 类型安全、无歧义、无堆分配 | 占用额外 bool 空间 |
| 裸指针 `T*` + nullptr | 零开销 | 所有权不明、悬垂风险 |
| 哨兵值（-1、NaN） | 无额外空间 | 值域受限、易错 |
| `pair<bool, T>` | 显式 | 语义不如 optional 清晰 |

### `optional` 的引用问题

`optional<T&>` 在 C++17 不支持（C++26 可能加）。要可空引用用 `optional<std::reference_wrapper<T>>` 或指针。

## HFT 关联

- **可空查找结果**：合约查找 `optional<Contract> find(symbol)`，比返回指针或哨兵值清晰。
- **可选配置项**：`optional<int> timeout = config.get("timeout")`，未配置时 nullopt，`value_or(default)` 取默认。
- **无堆分配**：optional 内联存储，热路径用 `optional<Tick>` 不分配堆，比 `unique_ptr<Tick>` 轻。
- **`value_or` 提供默认值**：`auto px = quote.value_or(last_known_px)` 行情缺失时用上次值。
- **慎用 `value()` 抛异常**：热路径用 `if (opt) *opt` 而非 `opt.value()`——后者无值抛异常有开销。
- **cache 友好**：`optional<T>` 大小 = `T` + 对齐，小类型放栈上 cache 友好；大类型注意 false sharing。

## 自测题

1. `optional<T>` 相比裸指针 `T*` + nullptr 有什么优势？
2. `*opt` 和 `opt.value()` 的区别是什么？热路径用哪个？
3. `optional<T>` 的存储布局是什么？会分配堆吗？
4. `value_or(def)` 的语义是什么？
5. HFT 为什么用 `optional<Tick>` 而非 `unique_ptr<Tick>`？

## 代码自测

### Q1: optional 基本用法
```cpp
std::optional<int> find(int key) {
    if (key == 42) return 42;
    return std::nullopt;
}

auto r = find(42);
if (r) std::cout << *r;    // 42
auto r2 = find(0);
std::cout << r2.value_or(-1);  // -1
```
> optional 相比返回指针或特殊值（如 -1）有什么优势？

<details>
<summary>答案与复习指引</summary>

**优势**：
1. **语义明确**：`optional<int>` 明确表示"可能没有值"，不像 `int` 返回 -1 有歧义（-1 可能是合法值）
2. **无指针开销**：optional 在栈上（值+bool标志），不涉及堆分配
3. **类型安全**：`*r` 解引用前检查 `r.has_value()`，空 optional 解引用是 UB 但比裸指针更易检查
4. **value_or**：提供默认值的便捷接口

**HFT 场景**：查找订单返回 `optional<OrderRef>` 比 `OrderRef*` 更安全（不涉及所有权/生命周期），比返回空 OrderRef 更清晰。

**注意**：`sizeof(optional<T>)` > `sizeof(T)`（多一个 bool + 对齐填充）。

**复习：** → [optional](./README.md)
</details>
