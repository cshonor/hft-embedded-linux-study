# 15.1 std::optional 基础用法

> 第 15 章 std::optional · 下一节：[15.2 optional 高级技巧与陷阱](02-optional高级技巧与陷阱.md)

## 这节讲什么

`std::optional<T>` 表示"可能有值也可能没有"的类型。C++14 之前，表示"可能没有值"要么用指针（nullptr 表示没有），要么用特殊值（-1 表示无效），要么用 pair<T, bool>。optional 把这些模式统一为一个类型安全的容器。

## 为什么要学这个（先建立直觉）

C 程序员表示"可能失败"的方式：

```c
// C：用特殊值表示失败
int find_index(int* arr, int n, int target) {
    for (int i = 0; i < n; ++i)
        if (arr[i] == target) return i;
    return -1;  // -1 表示"没找到"
}
// 问题：-1 是合法的 int，调用者可能忘记检查
```

C++ 的老方法：

```cpp
// 方法 1：指针（nullptr 表示没找到）
T* find(...) {
    if (found) return &val;
    return nullptr;  // 但返回栈地址是悬垂的
}

// 方法 2：pair<T, bool>
std::pair<int, bool> find(...) {
    if (found) return {index, true};
    return {0, false};  // 需要构造一个假值
}

// 方法 3：输出参数
bool find(int& out) {
    if (found) { out = val; return true; }
    return false;
}
```

C++17 optional：

```cpp
std::optional<int> find_index(int* arr, int n, int target) {
    for (int i = 0; i < n; ++i)
        if (arr[i] == target) return i;
    return std::nullopt;  // 没找到
}

auto idx = find_index(arr, 10, 42);
if (idx) {
    use(*idx);  // 有值
} else {
    // 没找到
}
```

## 基本接口

### 创建

```cpp
std::optional<int> a;              // 空
std::optional<int> b = 42;         // 有值
std::optional<int> c = std::nullopt;  // 显式空
std::optional<int> d{};            // 空
std::optional<int> e = std::make_optional(42);  // 有值
```

### 检查是否有值

```cpp
std::optional<int> opt = 42;

if (opt) { ... }              // true：有值
if (opt.has_value()) { ... }  // 同上
bool empty = !opt;             // false
```

### 访问值

```cpp
std::optional<int> opt = 42;

*opt;          // 42（未检查就访问，空时是 UB）
opt.value();   // 42（空时抛 std::bad_optional_access）
opt.value_or(0); // 42（空时返回默认值 0）

// 空的 opt：
std::optional<int> empty;
// *empty;     // UB!
// empty.value(); // 抛异常
empty.value_or(0); // 0
```

### 修改

```cpp
std::optional<int> opt = 42;
opt = 100;              // 赋值
opt.emplace(200);       // 原地构造（避免临时对象）
opt.reset();            // 清空
opt = std::nullopt;     // 清空（同上）
```

## 实际用法

### 1. 查找函数

```cpp
std::optional<Config> load_config(const std::string& path) {
    if (!file_exists(path)) return std::nullopt;
    return Config{read_file(path)};
}

auto cfg = load_config("config.json");
if (!cfg) { /* 处理配置缺失 */ }
```

### 2. 配合 if-init

```cpp
if (auto cfg = load_config("config.json"); cfg) {
    use(*cfg);
}
// cfg 出作用域自动清理
```

### 3. 链式调用

```cpp
std::optional<int> find_id(const std::string& name);
std::optional<Price> get_price(int id);

// 链式查找
auto price = find_id("AAPL").and_then(get_price);
if (price) { use(*price); }
// 注意：and_then 是 C++23 的 monadic 操作，C++17 需要手写
```

### 4. 可选返回值

```cpp
// 配置参数：有就用来，没有就用默认值
std::optional<int> parse_port(const std::string& s);
int port = parse_port(str).value_or(8080);  // 解析失败用 8080
```

## optional 的内存布局

```cpp
std::optional<int> opt;
// sizeof(opt) ≈ sizeof(int) + 1（bool 标志位），对齐后可能是 8 字节

// optional 内部：
struct {
    bool has_value;  // 标志位
    T value;         // 值（如果 has_value）
};
// 即使 has_value=false，value 的空间仍然存在（不动态分配）
```

**注意**：`sizeof(optional<T>)` > `sizeof(T)`，至少多 1 字节标志位。

## HFT 关联

```cpp
// 行情查找
std::optional<Tick> last_tick(const std::string& symbol) {
    auto it = ticks_.find(symbol);
    if (it == ticks_.end()) return std::nullopt;
    return it->second;
}

// 使用
if (auto t = last_tick("AAPL"); t) {
    match_order(*t);
}

// 风控检查
std::optional<RiskReport> check_risk(const Order& ord);
auto report = check_risk(ord);
if (report) reject(*report);
```

## 小结

| 接口 | 说明 |
|------|------|
| `opt.has_value()` | 检查是否有值 |
| `*opt` | 直接访问（空时 UB） |
| `opt.value()` | 访问（空时抛异常） |
| `opt.value_or(default)` | 空时返回默认值 |
| `opt.reset()` | 清空 |
| `opt.emplace(args...)` | 原地构造 |

---

← [本章导读](./README.md) · [下一节 →](02-optional高级技巧与陷阱.md)
