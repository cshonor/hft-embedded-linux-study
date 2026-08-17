# Item 16：将 string/vector 数据传给旧 C API

> 第 2 章 vector 和 string · Item 16 · 上一节：[Item 15 string 实现多样性](item15-string-implementation.md) · 下一节：[Item 17 交错使用 vector 和 string](item17-vector-string-interop.md)

## 为什么要学这个（先建立直觉）

C 程序员的字符串/缓冲区就是裸内存：

```c
char buf[256];
read(fd, buf, 256);          // 直接传指针
write(fd, buf, n);           // 直接传指针
legacy_parse(buf, n);        // C API 接受 char* + length
```

C++ 的 `string`/`vector` 封装了内存管理，但很多 C API（POSIX syscall、DPDK、第三方 C 库）只接受裸指针。你需要用 `c_str()`/`data()` 取出连续内存传给 C API。

```cpp
std::string s = "hello";
legacy_c_func(s.c_str());  // 只读 C 字符串

std::vector<char> buf(256);
legacy_read(buf.data(), buf.size());  // 可写缓冲
```

---

## 这节讲什么

C++11 起 `string` 与 `vector` 都保证连续存储。`c_str()`/`data()` 取连续内存传给 C API。注意：返回的指针在容器扩容/析构后失效。

---

## C API 互操作

```cpp
// 只读：传 string 给接受 const char* 的 C API
std::string filename = "data.txt";
FILE* fp = fopen(filename.c_str(), "r");  // c_str() 返回 const char*

// 可写：传 vector 给接受 char*/void* 的 C API
std::vector<char> buf(4096);
ssize_t n = read(fd, buf.data(), buf.size());  // data() 返回 char*
buf.resize(n);  // 缩到实际读取的大小

// 可写 string：
std::string s(256, '\0');
int len = legacy_read(s.data(), s.size());  // C++17 起 data() 返回 char*
s.resize(len);
```

### 指针失效陷阱

```cpp
std::vector<char> buf;
buf.resize(100);
char* p = buf.data();
legacy_func(p, 100);  // ✅

buf.push_back('x');    // 可能扩容 → p 失效！
legacy_func(p, 100);   // UB！p 指向已释放的旧内存
```

---

## 常见错误（新手踩坑）

### 错误 1：扩容后使用 data()/c_str() 返回的指针

```cpp
std::string s = "hello";
const char* p = s.c_str();
s += " world";  // 可能扩容 → p 失效
printf("%s", p);  // UB！
```

**修正：** 在不扩容的范围内使用指针。先完成所有修改，再取 `c_str()`。

### 错误 2：用 &v[0] 代替 data()（C++03 遗留）

```cpp
std::vector<int> v(10);
int* p = &v[0];    // C++03 写法（v 为空时 UB）
int* p2 = v.data(); // C++11 写法（v 为空时返回 nullptr，安全）
```

**修正：** 用 `data()`，空容器时安全返回 `nullptr`。

### 错误 3：传 string 给需要可写缓冲的 C API 时忘了预留空间

```cpp
std::string s;  // size = 0
// s.data() 指向空缓冲
read(fd, s.data(), 256);  // UB！size 是 0，没有可写空间
```

**修正：** `std::string s(256, '\0');` 先分配足够空间，再传 `s.data()`。

---

## 新手要点（和 C 的区别）

| 维度 | C | C++ STL | 为什么 |
|------|---|---------|--------|
| 缓冲区 | `char buf[N]` | `vector<char>` / `string` | RAII 管理 |
| 传给 C API | 直接传 `buf` | `data()` / `c_str()` | 取连续内存 |
| 指针失效 | `realloc` 后 | 扩容后 | 相同问题 |
| 可写访问 | 直接 | `data()`（C++17 char*） | C++11 前 const |

**一句话：** C 的数组就是裸指针，直接传给 API。C++ 的 `vector`/`string` 封装了内存，用 `data()`/`c_str()` 取出裸指针传给 C API——但注意扩容会让指针失效。

---

## HFT 关联

- **C API 互操作**：DPDK / syscall 接口要 `data()`/`c_str()` 取裸指针，注意返回前容器不能扩容（否则指针失效）。
- **`string_view` 零拷贝桥接**：C++17 `string_view` 可以从 `const char*` + length 构造，在 C API 和 C++ 代码间零拷贝传递只读数据。
- **预分配缓冲**：`vector<char> buf(4096)` 预分配固定大小缓冲，传 `data()` 给 `read()`，避免热路径动态分配。

---

## 代码自测

### Q1: c_str vs data
```cpp
std::string s = "hello";
const char* p1 = s.c_str();   // A
const char* p2 = s.data();    // B
// p1 和 p2 有什么区别？
```

<details>
<summary>答案</summary>

C++11 起**没有区别**——两者都返回 `const char*`，指向连续内存，且以 `'\0'` 结尾。

历史区别：C++03 中 `c_str()` 保证 `'\0'` 终止，`data()` 不保证。C++11 统一了——`data()` 也保证 `'\0'` 终止。
</details>

### Q2: 可写缓冲
```cpp
std::string s;           // A
std::string s2(256, 0);  // B

// read(fd, s.data(), 256);    // A: 安全吗？
read(fd, s2.data(), 256);      // B: 安全吗？
```
> A 和 B 哪个安全？

<details>
<summary>答案</summary>

- **A 不安全**：`s` 的 size 是 0，`s.data()` 指向空缓冲（或 nullptr）。`read` 写入 256 字节 → 缓冲区溢出/UB。
- **B 安全**：`s2` 的 size 是 256，`s2.data()` 指向 256 字节的可写缓冲。

C++17 起 `string::data()` 返回 `char*`（可写）。C++11/14 返回 `const char*`（只读），需要 `&s[0]`。
</details>

### Q3: 指针失效
```cpp
std::vector<int> v = {1, 2, 3};
int* p = v.data();
v.reserve(100);  // A
v.push_back(4);  // B
*p = 99;         // C
```
> C 行安全吗？

<details>
<summary>答案</summary>

**视情况**：
- 如果 `reserve(100)` 扩容了（原 capacity < 100）→ A 行后 `p` 失效 → C 行 UB。
- 如果 `reserve(100)` 没扩容 → A 行后 `p` 有效 → B 行不扩容（capacity≥100）→ C 行安全。

**最佳实践**：扩容/修改后重新获取 `data()`。
</details>

### Q4: vector 传给 C API
```cpp
// C API: void process_data(const int* data, size_t count);
std::vector<int> v = {10, 20, 30, 40, 50};
process_data(v.data(), v.size());  // A

// 如果在调用前 v.push_back(60); 呢？
v.push_back(60);
process_data(v.data(), v.size());  // B
```
> B 安全吗？和 A 有什么区别？

<detailf>
<summary>答案</summary>

**B 安全**——但前提是 `push_back` 后重新获取了 `data()`。B 行在 `push_back` 之后调用 `v.data()`，获取的是新的指针（可能已扩容搬迁）。`size()` 也更新为 6。

**关键规则**：每次修改容器后重新获取 `data()`/`c_str()`，不要缓存指针。
</details>

---

## 参考与延伸

- 上一节：[Item 15 string 实现多样性](item15-string-implementation.md)
- 下一节：[Item 17 交错使用 vector 和 string](item17-vector-string-interop.md)
- 回到：[第 2 章 vector 和 string](README.md)
