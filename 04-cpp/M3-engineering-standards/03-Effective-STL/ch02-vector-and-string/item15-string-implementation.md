# Item 15：注意 string 实现的多样性

> 第 2 章 vector 和 string · Item 15 · 上一节：[Item 14 reserve 避免重新分配](item14-reserve-avoid-realloc.md) · 下一节：[Item 16 传给 C API](item16-pass-to-c-api.md)

## 为什么要学这个（先建立直觉）

C 程序员的字符串只有一个布局：

```c
char s[100];  // 固定大小数组
// 或
char* s = malloc(strlen(src) + 1);  // 堆分配 + '\0'
// sizeof(char*) = 4/8 字节，固定
```

C++ 的 `std::string` 在不同标准库实现中布局不同：

```cpp
// GCC libstdc++: sizeof(string) = 32
// libc++: sizeof(string) = 24
// MSVC: sizeof(string) = 32
// 不同实现用不同的 SSO 阈值和内部布局
```

跨平台二进制共享时不能假设 `string` 的内存布局。

---

## 这节讲什么

不同标准库实现的 `string` 布局不同。**SSO（Small String Optimization）**让短字符串零堆分配。`sizeof(string)` 从 8 到 32 字节不等。跨平台二进制共享时用 `const char*` 或定长缓冲。

---

## SSO 详解

```cpp
// SSO 的核心思想：短字符串存在对象内部，长字符串在堆上
// 两种模式共用同一块内存（union）

// GCC libstdc++ string 内部布局（简化）：
struct string {
    char local_buf[16];  // SSO：短字符串存这里（≤15 字符 + '\0'）
    size_t length;
    union {
        char* heap_ptr;  // 长字符串：堆指针
        // local_buf 复用
    };
    size_t capacity;
};
// sizeof = 32

// SSO 阈值：
// libstdc++: 15 字符
// libc++:    22 字符
// MSVC:      15 字符
```

```cpp
std::string s1 = "AAPL";      // 4 字符 → SSO，零堆分配
std::string s2 = "BTC-PERP-20240927";  // 19 字符 → 可能超 SSO 阈值，堆分配
// sizeof(s1) == sizeof(s2)  // true，布局相同
```

---

## 常见错误（新手踩坑）

### 错误 1：假设 sizeof(string) 跨平台一致

```cpp
// 在 GCC 上 sizeof(string) = 32
// 在 libc++ 上 sizeof(string) = 24
// 跨平台二进制接口不能用 string 传参
void send_data(const std::string& s);  // ⚠️ ABI 依赖
```

**修正：** 跨 ABI 边界用 `const char*` + length。

### 错误 2：假设 SSO 阈值

```cpp
// 在 libstdc++ 上 SSO 阈值是 15
// 在 libc++ 上 SSO 阈值是 22
std::string s = "1234567890123456";  // 16 字符
// libstdc++: 堆分配（超 15）
// libc++: SSO（≤ 22）
// 性能行为不同！
```

**修正：** 不要依赖具体 SSO 阈值。如果需要确保零分配，用 `array<char, N>` 或 `string_view`。

### 错误 3：假设 string 内存连续且以 '\0' 结尾（C++03 没保证）

```cpp
std::string s = "hello";
char* p = &s[0];
p[3] = 'X';  // C++11 起安全（连续存储）
// C++03 不保证连续存储
```

**修正：** C++11 起 `string` 保证连续存储且 `data()`/`c_str()` 等价于 `&operator[](0)`。

---

## 新手要点（和 C 的区别）

| 维度 | C `char*` | C++ `string` | 为什么 |
|------|-----------|-------------|--------|
| 布局 | 固定（指针+数据） | 实现多样（SSO/堆） | 标准不规定布局 |
| sizeof | 4/8（指针） | 24/32（含 SSO） | 实现相关 |
| 短字符串 | 无优化 | SSO 零分配 | 性能优化 |
| '\0' 终止 | 必须 | C++11 起保证 | 兼容 C API |
| ABI | 统一 | 不统一 | 跨平台要注意 |

**一句话：** C 的 `char*` 布局固定（一个指针），C++ 的 `string` 布局因实现而异（SSO 阈值、sizeof 不同）。跨平台二进制接口用 `const char*`，不用 `string`。

---

## HFT 关联

- **SSO 与 symbol**：交易对 symbol 短（"AAPL"、"ESU5"），SSO 让 `string` 零堆分配。但长 symbol（"BTC-PERP-20240927"）超 SSO 阈值会堆分配——可考虑 `string_view` + 外部缓冲。
- **跨 ABI 用 `const char*`**：DPDK / syscall 接口要 `const char*`，不用 `string`（ABI 不兼容）。
- **`string_view`（C++17）零拷贝**：用 `string_view` 指向外部缓冲，零拷贝、零分配、跨 ABI 安全。

---

## 代码自测

### Q1: SSO
```cpp
std::string s1 = "hi";       // 2 字符
std::string s2 = "hello world this is a long string";  // 38 字符
std::cout << sizeof(s1) << ' ' << sizeof(s2);
```
> sizeof 相同吗？为什么？

<details>
<summary>答案</summary>

**相同**（如 GCC libstdc++ 中都是 32）。SSO 用 union 共用内存——短字符串存在内部缓冲，长字符串用堆指针。两种模式共用同一块内存，所以 sizeof 相同。

区别在内部：s1 没有堆分配（SSO），s2 有堆分配（超阈值）。
</details>

### Q2: 跨平台 sizeof
```cpp
std::cout << sizeof(std::string);
// GCC libstdc++: ?
// libc++: ?
// MSVC: ?
```
> 三个实现的 sizeof 分别是多少（大概）？

<details>
<summary>答案</summary>

- GCC libstdc++: 32
- libc++: 24
- MSVC: 32

不同实现的 `string` 内部布局不同（SSO 缓冲大小、指针/长度字段排列不同）。跨平台不能假设 sizeof。
</details>

### Q3: SSO 阈值
```cpp
std::string s = "1234567890123456";  // 16 字符
// 在 libstdc++（SSO ≤ 15）上会发生什么？
// 在 libc++（SSO ≤ 22）上呢？
```

<details>
<summary>答案</summary>

- **libstdc++（阈值 15）**：16 > 15 → 堆分配。`malloc` + 拷贝。
- **libc++（阈值 22）**：16 ≤ 22 → SSO。零堆分配。

同一段代码在不同实现上的性能行为不同——这就是"string 实现多样性"的实际影响。
</details>

### Q4: string_view 替代
```cpp
// 旧代码
void process(const std::string& s);  // 依赖 string 布局

// C++17
void process(std::string_view sv);  // 零拷贝、跨 ABI
std::string s = "hello";
process(s);            // string → string_view 隐式转换
process("world");      // const char* → string_view
process(some_vector);  // vector<char> → string_view
```
> string_view 相比 const string& 有什么优势？

<details>
<summary>答案</summary>

1. **零拷贝**：`string_view` 只存指针+长度，不拷贝数据。
2. **跨类型**：接受 `string`、`const char*`、`vector<char>`、`array<char,N>` 等。
3. **跨 ABI**：`string_view` 布局固定（指针+大小），不依赖 `string` 实现。
4. **零分配**：不触发 SSO/堆分配。

**注意**：`string_view` 不拥有数据——原数据销毁后 `string_view` 变悬空（类似 C 的指针）。
</details>

---

## 参考与延伸

- 上一节：[Item 14 reserve 避免重新分配](item14-reserve-avoid-realloc.md)
- 下一节：[Item 16 传给 C API](item16-pass-to-c-api.md)
- 回到：[第 2 章 vector 和 string](README.md)
