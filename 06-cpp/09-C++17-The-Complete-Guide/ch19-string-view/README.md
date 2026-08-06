# 第 19 章 字符串视图

**String Views**

## 本章讲什么

`std::string_view` 是 C++17 最重要的库特性之一——一个**非拥有**的字符串视图，只存指针 + 长度，零拷贝传递。替代 `const std::string&` 和 `const char*` 的很多场景。

## 要点

### 基本用法

```cpp
#include <string_view>

void log(std::string_view msg) {   // 不拷贝，零分配
    std::cout << msg;
}

log("hello");                    // const char* → string_view
log(std::string("hello"));       // std::string → string_view
log(string_view{"hello", 5});    // 显式构造
```

### 存储与开销

`string_view` 内部就是 `const char*` + `size_t`：
```
sizeof(string_view) = 16（64 位）
```

- **不拥有数据**——不分配、不释放。
- 构造/拷贝/赋值都是 O(1)（拷贝指针 + 长度）。
- **必须保证底层字符串的生命周期**——view 比底层 string 活得长就悬垂。

### 接口（与 string 的子集）

`string_view` 有 `string` 的大部分**只读**接口：
- `size()`/`length()`、`empty()`
- `operator[]`、`at()`、`front()`/`back()`
- `substr(pos, len)` —— 返回 string_view，零拷贝
- `find`/`rfind`/`find_first_of` 等
- `remove_prefix(n)`/`remove_suffix(n)` —— 原地调整
- `data()` —— 底层指针（可能不 null-terminated！）

**没有**：`c_str()`（不保证 null-terminated）、修改操作、`operator+`。

### 零拷贝解析

```cpp
// FIX 协议字段解析：不拷贝
std::string_view fix = "8=FIX.4.2|55=AAPL|44=150.25";

auto sep = fix.find('|');
std::string_view first = fix.substr(0, sep);   // "8=FIX.4.2"，零拷贝
fix.remove_prefix(sep + 1);                     // 剩余部分
```

每次 substr/remove_prefix 只改指针和长度，无内存分配。解析 100 个字段零拷贝。

### 生命周期陷阱

```cpp
std::string_view dangerous() {
    std::string s = "hello";
    return s;   // 返回 string_view 指向 s，s 析构后悬垂！
}

// 安全：底层是字符串字面量（static 存储期）
std::string_view safe() { return "hello"; }
```

**规则**：string_view 不能比底层 string 活得长。字符串字面量（`"..."`）是 static 存储期，安全；临时 string 不安全。

### `string_view` vs `const string&` vs `const char*`

| 方案 | 拷贝 | 分配 | 长度 | null-terminated |
|------|------|------|------|-----------------|
| `const string&` | 否 | 构造 string 时分配 | 有 | 是 |
| `const char*` | 否 | 否 | 要 strlen | 是 |
| `string_view` | 否 | 否 | 有（O(1)） | 不保证 |

## HFT 关联

- **FIX 协议零拷贝解析**：行情字段解析用 `string_view` substr，无 string 构造/分配，纳秒级热路径关键优化。
- **函数参数**：日志、监控、回调名用 `string_view` 参数，接受 `const char*`、`string`、`string_view` 三种实参无拷贝。
- **CSV 字段切分**：批量行情 CSV 解析用 `string_view` + `remove_prefix`，零拷贝遍历。
- **生命周期警惕**：热路径的 string_view 必须确保底层缓冲（mmap、环形缓冲）在 view 使用期间不被释放/覆盖。
- **`data()` 与 C API**：`string_view::data()` 传给 `memcpy`/系统调用，但不保证 null-terminated——要 null-terminated 用 `string::c_str()`。
- **非 null-terminated 的优势**：解析二进制协议中的"长度前缀字符串"时，`string_view{ptr, len}` 直接用，无需补 `\0`。

## 自测题

1. `string_view` 相比 `const string&` 有什么优势？相比 `const char*` 呢？
2. `string_view` 的生命周期陷阱是什么？什么时候安全，什么时候不安全？
3. `string_view::substr` 和 `string::substr` 的区别？为什么前者零拷贝？
4. `string_view` 为什么没有 `c_str()`？`data()` 的注意事项？
5. HFT FIX 协议解析为什么用 `string_view` + `remove_prefix`？零拷贝体现在哪？
