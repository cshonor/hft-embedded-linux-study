# 19.1 std::string_view 基础

> 第 19 章 字符串视图 · 下一节：[19.2 string_view 陷阱与最佳实践](02-string-view陷阱与最佳实践.md)

## 这节讲什么

`std::string_view` 是 C++17 最重要的库特性之一——一个不拥有字符串所有权的轻量视图。它只有指针+长度，零拷贝地引用现有字符串数据。HFT 中解析 FIX/CSV 协议的核心工具。

## 为什么要学这个（先建立直觉）

C 函数接收字符串的方式：

```c
// C：const char* + 长度（或靠 '\0' 终止）
int parse(const char* data, size_t len);

// 问题：不传长度 → 靠 \0，二进制数据可能包含 \0
// 问题：传 const char* 和 size_t → 两个参数，容易不匹配
```

C++14 的问题：

```cpp
// C++14：函数参数用 const string&
void process(const std::string& s);

// 问题：如果实参是 const char*，会隐式构造临时 string → 拷贝！
process("hello");  // 构造临时 string → 拷贝 5 字节

// 问题：从 string 取子串 → 又拷贝
std::string s = "hello world";
process(s.substr(0, 5));  // substr 拷贝 5 字节
```

C++17 string_view：

```cpp
void process(std::string_view sv);

process("hello");           // 零拷贝：string_view 指向字面量
process(s);                 // 零拷贝：string_view 指向 s 的数据
process(std::string_view(s.data(), 5));  // 零拷贝：子串视图

// string_view 只是指针+长度，不拷贝数据
```

## string_view 的内存布局

```cpp
// string_view 内部就是指针 + 长度
struct string_view {
    const char* data_;  // 8 字节
    size_t size_;       // 8 字节
};
// sizeof(string_view) = 16 字节
```

对比 `std::string`（通常 32 字节 + 堆分配），`string_view` 极轻量。

## 基本接口

### 构造

```cpp
std::string_view sv1("hello");             // 从 C 字符串（靠 strlen 算长度）
std::string_view sv2("hello", 5);          // 从指针+长度（不需要 \0）
std::string s = "hello world";
std::string_view sv3(s);                   // 从 string（零拷贝）
std::string_view sv4(s.data(), 5);         // 子串视图（零拷贝）
std::string_view sv5 = "hello";            // 隐式构造
```

### 访问

```cpp
std::string_view sv = "hello world";

sv[0];              // 'h'
sv.at(0);           // 'h'（边界检查，越界抛异常）
sv.front();         // 'h'
sv.back();          // 'd'
sv.data();          // const char* 指针
sv.size();          // 11
sv.length();        // 11
sv.empty();         // false
```

### 子串与搜索

```cpp
std::string_view sv = "hello world";

sv.substr(0, 5);       // "hello"（零拷贝：返回新的 string_view）
sv.substr(6);          // "world"
sv.find("world");      // 6
sv.find("xyz");        // npos
sv.find('o');          // 4
sv.rfind('o');         // 7
sv.starts_with("hello"); // C++20
sv.ends_with("world");   // C++20
```

### 比较

```cpp
std::string_view a = "hello";
std::string_view b = "world";

a == b;      // false
a < b;       // true（字典序）
a.compare(b); // <0
```

### 转换回 string

```cpp
std::string_view sv = "hello";
std::string s(sv);      // 拷贝：string_view → string
std::string s2 = std::string(sv);  // 同上
auto s3 = sv.data();     // const char*（需要 \0 终止才能当 C 字符串用）
```

## HFT 关联

### FIX 协议解析

```cpp
// FIX 协议字段格式：tag=value<SOH>
// 例如：35=D<SOH>55=AAPL<SOH>38=100<SOH>

void parse_fix(std::string_view msg) {
    while (!msg.empty()) {
        auto sep = msg.find('=');
        auto tag = msg.substr(0, sep);
        auto rest = msg.substr(sep + 1);
        auto soh = rest.find('\x01');  // FIX 分隔符
        auto value = rest.substr(0, soh);

        // 零拷贝提取 tag 和 value
        if (tag == "35") handle_msg_type(value);
        else if (tag == "55") handle_symbol(value);

        msg = rest.substr(soh + 1);  // 移动到下一个字段
    }
}
```

### CSV 解析

```cpp
std::vector<std::string_view> split_csv(std::string_view line) {
    std::vector<std::string_view> fields;
    size_t start = 0;
    while (start <= line.size()) {
        auto comma = line.find(',', start);
        if (comma == std::string_view::npos) comma = line.size();
        fields.push_back(line.substr(start, comma - start));
        start = comma + 1;
    }
    return fields;
    // 零拷贝：所有 string_view 指向原始 line 的数据
}
```

## 小结

| 特性 | `const std::string&` | `std::string_view` |
|------|---------------------|--------------------|
| 大小 | 32 字节 | 16 字节 |
| 从 `const char*` 构造 | 隐式拷贝 | 零拷贝 |
| 子串 | 拷贝 | 零拷贝 |
| 拥有数据 | 是 | 否 |
| 可修改数据 | 否 | 否 |

---

← [本章导读](./README.md) · [下一节 →](02-string-view陷阱与最佳实践.md)
