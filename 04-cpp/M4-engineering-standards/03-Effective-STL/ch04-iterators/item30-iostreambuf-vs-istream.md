# Item 30：`istreambuf_iterator` vs `istream_iterator`

> 第 4 章 迭代器 · Item 30 · 上一节：[Item 29 流迭代器](item29-stream-iterators.md) · 下一节：[Item 31 迭代器分类](item31-iterator-categories.md)

## 为什么要学这个（先建立直觉）

C 程序员读文件有两种方式：

```c
// 方式 1：fscanf——格式化读取（跳空白、解析类型）
int x;
fscanf(fp, "%d", &x);

// 方式 2：fread/fgetc——原始字节读取（不跳空白、不解析）
char buf[4096];
int n = fread(buf, 1, sizeof(buf), fp);  // 原始字节
```

C++ 也有对应的两种迭代器：

```cpp
// istream_iterator：格式化读取（跳空白、类型解析）—— 慢
std::istream_iterator<char>(f);

// istreambuf_iterator：原始字节读取（不跳空白、不解析）—— 快
std::istreambuf_iterator<char>(f);
```

---

## 这节讲什么

`istreambuf_iterator` 直接读字节流（不跳空白、不格式化），比 `istream_iterator`（跳空白、格式化）快。读原始二进制用前者。

---

## 区别详解

```cpp
// 文件内容：'a', ' ', 'b', '\n', 'c'

// istream_iterator<char>：跳过空白
std::ifstream f1("data.txt");
std::string s1{std::istream_iterator<char>(f1), std::istream_iterator<char>()};
// s1 = "abc"（空格和换行被跳过）

// istreambuf_iterator<char>：不跳空白
std::ifstream f2("data.txt");
std::string s2{std::istreambuf_iterator<char>(f2), std::istreambuf_iterator<char>()};
// s2 = "a b\nc"（保留所有字符）
```

### 性能差异

```cpp
// 慢：istream_iterator 每次调用 operator>> → 格式化 + 跳空白
std::string s1{std::istream_iterator<char>(f), {}};

// 快：istreambuf_iterator 直接从流缓冲区取一个字节
std::string s2{std::istreambuf_iterator<char>(f), {}};
// 快 5-10 倍
```

---

## 常见错误（新手踩坑）

### 错误 1：用 istream_iterator 读原始文本

```cpp
// 想读入完整文件内容（包括空白）
std::ifstream f("file.txt");
std::string s{std::istream_iterator<char>(f), std::istream_iterator<char>()};
// s 丢失了所有空白！
```

**修正：** 用 `istreambuf_iterator<char>` 读取完整内容。

### 错误 2：用 istream_iterator 读二进制

```cpp
std::ifstream f("data.bin", std::ios::binary);
// istream_iterator<char> 会跳空白、做格式化 → 破坏二进制数据
```

**修正：** 用 `istreambuf_iterator<char>` 或 `f.read(buf, n)`。

---

## 新手要点（和 C 的区别）

| 维度 | C `fscanf` vs `fread` | C++ `istream_iterator` vs `istreambuf_iterator` | 为什么 |
|------|----------------------|------------------------------------------------|--------|
| 格式化 | fscanf vs fread | istream_iterator vs istreambuf_iterator | 对应关系 |
| 跳空白 | fscanf 跳 vs fread 不跳 | istream_iterator 跳 vs istreambuf 不跳 | 相同 |
| 性能 | fread 快 | istreambuf_iterator 快 | 无格式化开销 |

**一句话：** C 的 `fscanf`（格式化）vs `fread`（原始）对应 C++ 的 `istream_iterator`（格式化）vs `istreambuf_iterator`（原始）。读原始文本/二进制用 `istreambuf_iterator`。

---

## HFT 关联

- **读配置文件**：非热路径读配置文件可以用 `istreambuf_iterator<char>` 一次读取全部内容到 `string`，比 `istream_iterator` 快且保留空白。
- **热路径不用**：HFT 行情解析用 `read()` + 手写解析器，不用任何流迭代器。

---

## 代码自测

### Q1: 跳空白 vs 不跳
```cpp
// 文件内容：'a', ' ', 'b'
std::ifstream f("test.txt");

// A
std::string s1{std::istream_iterator<char>(f), std::istream_iterator<char>()};
// B（重新打开文件）
std::ifstream f2("test.txt");
std::string s2{std::istreambuf_iterator<char>(f2), std::istreambuf_iterator<char>()};
```
> s1 和 s2 分别是什么？

<details>
<summary>答案</summary>

- **s1** = "ab"（`istream_iterator` 跳过空格）
- **s2** = "a b"（`istreambuf_iterator` 保留空格）
</details>

### Q2: 读整个文件
```cpp
// 最简洁的"读整个文件到 string"写法
std::ifstream f("file.txt");
std::string content{std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
```
> 为什么用 istreambuf_iterator 而不是 istream_iterator？

<details>
<summary>答案</summary>

1. **保留空白**：`istreambuf_iterator` 不跳过空格/制表/换行，保留文件原始内容。
2. **更快**：直接从流缓冲区取字节，不做格式化，快 5-10 倍。
3. **二进制安全**：不会因格式化破坏二进制数据。
</details>

---

## 参考与延伸

- 上一节：[Item 29 流迭代器](item29-stream-iterators.md)
- 下一节：[Item 31 迭代器分类](item31-iterator-categories.md)
- 回到：[第 4 章 迭代器](README.md)
