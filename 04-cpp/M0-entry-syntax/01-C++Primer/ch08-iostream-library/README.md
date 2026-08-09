# 第 8 章 IO 库

C++ 通过标准库中的一族类处理输入和输出。本章介绍如何读写设备（控制台、命名文件）以及内存中的 `string` 对象。

## 小节

- [IO 类体系](./8.1-IO类体系.md)
- [管理条件状态](./8.2-管理条件状态.md)
- [文件输入输出](./8.3-文件输入输出.md)
- [string 流](./8.4-string流.md)


## 章节摘要

C++ IO 库体系：IO 类继承层次（`istream`/`ostream`/`iostream`/`ifstream`/`ofstream`/`istringstream`/`ostringstream`）、条件状态管理（`good`/`fail`/`bad`/`eof`）、文件流（打开/关闭/模式）、字符串流。

### 和 C 的区别

| C | C++ |
|---|-----|
| `FILE*` + `fprintf`/`fscanf` | `ostream`/`istream` + `<<`/`>>` |
| `sprintf`/`sscanf` | `ostringstream`/`istringstream` |
| 手动检查返回值 | 流状态 `if (cin >> x)` |
| 无 RAII | `fstream` 析构自动关闭文件 |

## 章节自测

### Q1: 流状态

```cpp
int x;
std::cin >> x;   // 用户输入 "abc"
if (std::cin.fail()) std::cout << "fail ";
if (std::cin.eof())  std::cout << "eof ";
if (std::cin.bad())  std::cout << "bad ";
std::cout << std::cin.good();
```

> 用户输入 "abc" 后输出什么？如何恢复？

<details>
<summary>答案与复习指引</summary>

**输出：** `fail 0`

**解析：**
- `fail()` = true：类型不匹配（期望 int 但输入了字母）
- `eof()` = false：没到文件尾
- `bad()` = false：没有 I/O 级错误
- `good()` = 0（false）：因为 failbit 被设置

**恢复：** `std::cin.clear();` 清除错误状态，然后 `std::cin.ignore(...)` 丢弃坏输入。

**复习：** → [管理条件状态](./8.2-管理条件状态.md)
</details>

### Q2: fstream RAII

```cpp
void process_file(const std::string &path) {
    std::ifstream in(path);
    if (!in) { /* 错误处理 */ return; }
    std::string line;
    while (std::getline(in, line)) {
        // 处理 line
    }
}   // in 在这里析构，自动关闭文件
```

> 和 C 的 `FILE*` 相比有什么优势？

<details>
<summary>答案与复习指引</summary>

**优势：**
1. **RAII 自动关闭**：`ifstream` 析构自动调用 `close()`，即使异常也不泄漏文件描述符
2. **类型安全**：`>>` 根据变量类型自动解析，不像 `fscanf` 要手写格式符
3. **错误检查更直观**：`if (!in)` 或 `if (in >> x)` 检查流状态

**C 的写法：** 需要 `FILE *fp = fopen(...); ... fclose(fp);`，忘记 `fclose` 或异常路径不 `fclose` 会泄漏 fd。

**复习：** → [文件输入输出](./8.3-文件输入输出.md)
</details>

### Q3: stringstream

```cpp
std::ostringstream oss;
oss << "Price: " << 3.14 << " Qty: " << 100;
std::string result = oss.str();
// result 是什么？

std::istringstream iss("2024-08-06");
int year, month, day;
char dash1, dash2;
iss >> year >> dash1 >> month >> dash2 >> day;
// year, month, day 分别是多少？
```

> `result` 是什么？`year`/`month`/`day` 分别是多少？

<details>
<summary>答案与复习指引</summary>

**result = `"Price: 3.14 Qty: 100"`** —— `ostringstream` 把各种类型拼接成字符串

**year=2024, month=8, day=6** —— `istringstream` 用 `>>` 按类型解析，`char` 读取分隔符 `-`

**和 C 的区别：** C 用 `sprintf`/`sscanf`，需要手动管理缓冲区大小和格式字符串。`stringstream` 类型安全且自动管理内存。

**复习：** → [string 流](./8.4-string流.md)
</details>
