# Item 17：交错使用 vector 和 string 数据

> 第 2 章 vector 和 string · Item 17 · 上一节：[Item 16 传给 C API](item16-pass-to-c-api.md)

## 为什么要学这个（先建立直觉）

C 程序员的字符数据只有一种形式：

```c
char buf[100];           // 缓冲区
char* s = "hello";       // 字符串
// 两者本质相同：连续 char 数组
// buf 和 s 可以互传
```

C++ 的 `vector<char>` 和 `string` 都是一段连续字符，但语义不同——`string` 有 `\0` 终止语义，`vector<char>` 没有。C++17 的 `string_view` 让二者可以零拷贝互操作。

```cpp
std::vector<char> v = {'h', 'e', 'l', 'l', 'o'};
std::string s(v.begin(), v.end());      // vector → string
std::string_view sv(v.data(), v.size()); // vector → string_view（零拷贝）
```

---

## 这节讲什么

`vector<char>` 和 `string` 都是一段连续字符，可互相借用。但 `string` 有 `\0` 终止语义，`vector<char>` 没有——二进制数据用 `vector<char>` 或 `string_view`，别用 `string`。

---

## 互操作方式

```cpp
// vector<char> → string
std::vector<char> v = {'h', 'e', 'l', 'l', 'o'};
std::string s(v.begin(), v.end());  // 区间构造

// string → vector<char>
std::string s2 = "world";
std::vector<char> v2(s2.begin(), s2.end());

// C++17 string_view：零拷贝指向二者
std::string_view sv1(v.data(), v.size());   // 指向 vector
std::string_view sv2(s2);                    // 指向 string
// string_view 不拥有数据，原数据销毁后 sv 变悬空
```

### string vs vector<char> 语义差异

```cpp
std::string s = "hello";
s += '\0';        // string 可以包含 '\0'，但 c_str() 在第一个 '\0' 截断
s.c_str();        // 返回 "hello"（在 '\0' 处截断）
s.size();         // 6（包含 '\0'）

std::vector<char> v = {'h', 'e', 'l', 'l', 'o', '\0', 'X'};
// vector 不关心 '\0'，完整存储所有字节
v.size();         // 7
```

---

## 常见错误（新手踩坑）

### 错误 1：用 string 存二进制数据

```cpp
std::string binary_data = read_raw_socket();  // 可能含 '\0'
// 后续用 c_str() / strlen 会截断！
strlen(binary_data.c_str());  // 只到第一个 '\0'
```

**修正：** 二进制数据用 `vector<char>` 或 `string_view`，不用 `string`。

### 错误 2：string_view 悬空

```cpp
std::string_view get_data() {
    std::string s = "hello";
    return s;  // 返回指向 s 的 string_view
}  // s 析构 → string_view 悬空！
```

**修正：** `string_view` 不拥有数据。返回 `string`（拥有数据），不要返回指向局部变量的 `string_view`。

### 错误 3：用 string 的 += 拼接大量数据

```cpp
std::string result;
for (int i = 0; i < 1000; ++i) {
    result += generate_chunk(i);  // 可能多次扩容
}
```

**修正：** `result.reserve(estimated_size);` 预分配。

---

## 新手要点（和 C 的区别）

| 维度 | C `char*` | C++ `string` | C++ `vector<char>` | C++17 `string_view` |
|------|-----------|-------------|---------------------|---------------------|
| `\0` 语义 | 终止符 | 有（c_str 截断） | 无 | 无 |
| 二进制安全 | ❌ | ❌（c_str 截断） | ✅ | ✅ |
| 拥有数据 | 手动 | ✅ RAII | ✅ RAII | ❌ 不拥有 |
| 零拷贝 | N/A | ❌ | ❌ | ✅ |

**一句话：** C 的 `char*` 不区分文本和二进制。C++ 用 `string` 存文本（`\0` 语义），`vector<char>` 存二进制，`string_view` 零拷贝指向二者。

---

## HFT 关联

- **`vector<char>` vs `string` 解析**：FIX/二进制协议用 `vector<char>` 或 `string_view`，避免 `string` 的 `\0` 截断语义。
- **`string_view` 零拷贝**：行情消息解析用 `string_view` 指向接收缓冲，零拷贝切片/查找，不分配内存。
- **避免 `string` 拼接**：热路径上不用 `string += ` 拼接，预 `reserve` 或直接写入固定缓冲。

---

## 代码自测

### Q1: string vs vector<char>
```cpp
std::string s = "hello";
s.push_back('\0');
s.push_back('X');
std::cout << s.size() << ' ' << strlen(s.c_str());
```
> 输出什么？

<details>
<summary>答案</summary>

输出 `7 5`。
- `s.size()` = 7（string 完整存储所有字符，包括 `\0` 和 `X`）。
- `strlen(s.c_str())` = 5（C 函数 `strlen` 在第一个 `\0` 处停止计数）。

这就是 `string` 存二进制数据的问题——`c_str()` 和 C API 在 `\0` 处截断。
</details>

### Q2: string_view 零拷贝
```cpp
std::vector<char> v = {'h', 'e', 'l', 'l', 'o'};
std::string_view sv(v.data(), v.size());

// A: 从 sv 构造 string
std::string s(sv);

// B: 从 sv 切片
std::string_view sub = sv.substr(1, 3);  // "ell"
```
> A 和 B 各有没有拷贝？

<details>
<summary>答案</summary>

- **A**：有拷贝。`string(sv)` 从 `string_view` 拷贝数据构造新 `string`。
- **B**：零拷贝。`substr` 返回新的 `string_view`，只调整指针和长度，不拷贝数据。

`string_view` 的切片/查找操作都是零拷贝的——这是它在 HFT 行情解析中的核心价值。
</details>

### Q3: string_view 生命周期
```cpp
std::string_view get_prefix(const std::string& s) {
    return s.substr(0, 3);  // 返回指向 s 的 string_view
}
// 调用者
std::string_view sv = get_prefix("hello");  // A
std::cout << sv;  // B
```
> B 行安全吗？

<details>
<summary>答案</summary>

**不安全**。`"hello"` 是临时 `string`，`get_prefix` 返回后临时对象析构，`sv` 变悬空。

**修正：**
1. 返回 `std::string`（拷贝，安全）。
2. 或让调用者持有原 string：
```cpp
std::string s = "hello";
std::string_view sv = get_prefix(s);  // s 仍然存活
std::cout << sv;  // ✅ 安全
```
</details>

### Q4: 二进制数据处理
```cpp
// 从 socket 读取二进制数据（可能含 '\0'）
// A
std::string data = read_socket();
// B
std::vector<char> data = read_socket();
// C
std::array<char, 4096> buf;
int n = read(fd, buf.data(), buf.size());
std::string_view data(buf.data(), n);
```
> 哪种方式最适合处理二进制数据？

<details>
<summary>答案</summary>

**C 最适合**（固定缓冲 + string_view）。

- **A**：❌ `string` 的 `c_str()` 在 `\0` 截断，不适合二进制数据。
- **B**：✅ 可以，但每次读取都动态分配。
- **C**：✅ 最佳——固定缓冲零分配，`string_view` 零拷贝切片/查找，不关心 `\0`。

HFT 行情解析的标准模式：预分配固定缓冲 + `string_view` 零拷贝操作。
</details>

---

## 参考与延伸

- 上一节：[Item 16 传给 C API](item16-pass-to-c-api.md)
- 回到：[第 2 章 vector 和 string](README.md)
