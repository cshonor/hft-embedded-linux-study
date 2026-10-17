# Item 29：`istream_iterator` / `ostream_iterator`

> 第 4 章 迭代器 · Item 29 · 上一节：[Item 27-28 反向迭代器](item27-28-reverse-iterator-base.md) · 下一节：[Item 30 iostreambuf_iterator](item30-iostreambuf-vs-istream.md)

## 为什么要学这个（先建立直觉）

C 程序员从文件读数据到数组：

```c
FILE* fp = fopen("data.txt", "r");
int arr[100], n = 0;
while (fscanf(fp, "%d", &arr[n]) == 1) n++;
fclose(fp);
```

C++ 的流迭代器让流当容器用——一行代码完成读取+构造：

```cpp
std::ifstream f("data.txt");
std::vector<int> v{std::istream_iterator<int>(f), std::istream_iterator<int>()};
// 从文件读取所有 int，直接构造 vector
```

简洁但每次 `++` 都解析一次流，性能不如批量 `read`。

---

## 这节讲什么

流迭代器让流当容器用：`istream_iterator` 从输入流读取，`ostream_iterator` 向输出流写入。简洁但解析开销大，HFT 不用。

---

## 流迭代器用法

```cpp
// 从 cin 读入 vector
std::vector<int> v(std::istream_iterator<int>(std::cin),
                   std::istream_iterator<int>());

// 从文件读入
std::ifstream f("data.txt");
std::vector<int> v{std::istream_iterator<int>(f), std::istream_iterator<int>()};

// 写出到 cout
std::copy(v.begin(), v.end(),
          std::ostream_iterator<int>(std::cout, " "));
// 输出：1 2 3 4 5（每个后面加空格）
```

### 用法示例

```cpp
// 读入 set（自动去重+排序）
std::set<int> s{std::istream_iterator<int>(f), std::istream_iterator<int>()};

// vector → cout（逗号分隔）
std::copy(v.begin(), v.end(),
          std::ostream_iterator<int>(std::cout, ", "));
```

---

## 常见错误（新手踩坑）

### 错误 1：最烦人解析

```cpp
std::vector<int> v(std::istream_iterator<int>(cin),
                   std::istream_iterator<int>());
// v 是函数声明！不是 vector！
```

**修正：** 用 `{}` 初始化：`std::vector<int> v{...};`（见 Item 6）

### 错误 2：性能陷阱

```cpp
// 每次读取都解析流——比 fread + 批量处理慢 10-100 倍
std::vector<int> v{std::istream_iterator<int>(f), std::istream_iterator<int>()};
```

**修正：** HFT 场景用 `fread`/`read` + 手写解析器。

### 错误 3：istream_iterator 跳过空白

```cpp
// istream_iterator<int> 跳过空白字符（空格/制表/换行）
// 如果要读取原始字节（包括空白），用 istreambuf_iterator
```

**修正：** 读二进制/原始字符用 `istreambuf_iterator`（见 Item 30）。

---

## 新手要点（和 C 的区别）

| 维度 | C 读取 | C++ 流迭代器 | 为什么 |
|------|--------|-------------|--------|
| 读取 | `fscanf` 循环 | `istream_iterator` | 声明式 |
| 写入 | `fprintf` 循环 | `ostream_iterator` | 声明式 |
| 性能 | 快（直接 I/O） | 慢（流解析开销） | 格式化代价 |
| 类型安全 | ❌（格式串） | ✅（模板推导） | 编译期检查 |

**一句话：** C 的 `fscanf` 循环繁琐但快。C++ 的流迭代器简洁但慢——适合原型和工具，不适合 HFT 热路径。

---

## HFT 关联

- **流迭代器慎用**：`istream_iterator` 解析开销大，HFT 行情解析用 `string_view` + 手写解析器，不用流迭代器。
- **ostream_iterator 调试输出**：非热路径的调试/日志输出可以用 `ostream_iterator`，简洁。

---

## 代码自测

### Q1: 读取文件
```cpp
std::ifstream f("data.txt");  // 内容：1 2 3 4 5
std::vector<int> v{std::istream_iterator<int>(f), std::istream_iterator<int>()};
std::cout << v.size();
```
> 输出多少？

<details>
<summary>答案</summary>

输出 `5`。`istream_iterator<int>` 从文件读取 5 个整数，构造 vector。`istream_iterator<int>()` 是默认构造的"结束"迭代器（表示流结束）。
</details>

### Q2: ostream_iterator
```cpp
std::vector<int> v = {1, 2, 3};
std::copy(v.begin(), v.end(),
          std::ostream_iterator<int>(std::cout, ", "));
```
> 输出什么？

<details>
<summary>答案</summary>

输出 `1, 2, 3, `。`ostream_iterator<int>(cout, ", ")` 把每个 int 写到 cout，后面跟 ", "。

注意末尾也有 ", "——`ostream_iterator` 不自动处理分隔符。
</details>

### Q3: 最烦人解析
```cpp
std::vector<int> v(std::istream_iterator<int>(std::cin),
                   std::istream_iterator<int>());
v.push_back(42);
```
> 能编译吗？

<detailf>
<summary>答案</summary>

**不能编译**（或报奇怪的函数相关错误）。这是最烦人解析——`v` 被解析为函数声明而非 vector 对象。`v.push_back(42)` 对函数调用报错。

**修正：** 用 `{}`：`std::vector<int> v{...};`
</details>

### Q4: 性能对比
```cpp
// A: istream_iterator
std::ifstream f("data.txt");
std::vector<int> v{std::istream_iterator<int>(f), std::istream_iterator<int>()};

// B: fread + 手写解析
FILE* fp = fopen("data.txt", "r");
char buf[65536];
int n = fread(buf, 1, sizeof(buf), fp);
// 手写解析 buf 中的整数
```
> A 和 B 的性能差异？

<detailf>
<summary>答案</summary>

**B 比 A 快 10-100 倍**。
- **A**：每次 `++istream_iterator` 都调用 `operator>>`，涉及流状态检查、格式化、locale 等。大量函数调用开销。
- **B**：一次 `fread` 批量读取，手写解析直接操作字符数组，无流开销。

**HFT 教训**：热路径用批量 I/O + 手写解析，不用流迭代器。
</details>

---

## 参考与延伸

- 上一节：[Item 27-28 反向迭代器](item27-28-reverse-iterator-base.md)
- 下一节：[Item 30 iostreambuf_iterator](item30-iostreambuf-vs-istream.md)
- 回到：[第 4 章 迭代器](README.md)
