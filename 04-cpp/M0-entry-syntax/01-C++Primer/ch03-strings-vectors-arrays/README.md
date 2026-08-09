# 第 3 章 字符串、向量和数组

在前一章内置类型的基础上，本章介绍 C++ 标准库中一组更高级的抽象数据类型：可变长字符串 `string`、可变长集合 `vector`、配套迭代器，以及底层内置数组。

## 小节

- [3.1 命名空间的 using 声明](./3.1-命名空间的using声明.md)
- [3.2 标准库类型 string](./3.2-library-type-string/3.2-标准库类型string.md)
  - [3.2.1 定义与初始化](./3.2-library-type-string/3.2.1-定义与初始化.md)
  - [3.2.2 读写操作](./3.2-library-type-string/3.2.2-读写操作.md)
  - [3.2.3 拼接与内存优化](./3.2-library-type-string/3.2.3-拼接与内存优化.md)
  - [3.2.4 string 与 C 风格字符串](./3.2-library-type-string/3.2.4-string与C风格字符串.md)
  - [3.2.5 cctype 字符校验](./3.2-library-type-string/3.2.5-cctype字符校验.md)
  - [3.2.6 HFT 场景拓展](./3.2-library-type-string/3.2.6-HFT场景拓展.md)
- [3.3 标准库类型 vector](./3.3-library-type-vector/3.3-标准库类型vector.md)
  - [3.3.1 动态数组基础](./3.3-library-type-vector/3.3.1-动态数组基础.md)
  - [3.3.2 圆括号与花括号初始化](./3.3-library-type-vector/3.3.2-圆括号与花括号初始化.md)
  - [3.3.3 下标与增删](./3.3-library-type-vector/3.3.3-下标与增删.md)
  - [3.3.4 元素类型与嵌套容器](./3.3-library-type-vector/3.3.4-元素类型与嵌套容器.md)
  - [3.3.5 拷贝移动与扩容](./3.3-library-type-vector/3.3.5-拷贝移动与扩容.md)
  - [3.3.6 HFT 实践与示例](./3.3-library-type-vector/3.3.6-HFT实践与示例.md)
- [3.4 迭代器介绍](./3.4-introducing-iterators/3.4-迭代器介绍.md)
  - [3.4.1 为何需要迭代器](./3.4-introducing-iterators/3.4.1-为何需要迭代器.md)
  - [3.4.2 begin、end 与常量迭代器](./3.4-introducing-iterators/3.4.2-begin-end与常量迭代器.md)
  - [3.4.3 迭代器运算符与遍历](./3.4-introducing-iterators/3.4.3-迭代器运算符与遍历.md)
  - [3.4.4 随机访问算术运算](./3.4-introducing-iterators/3.4.4-随机访问算术运算.md)
  - [3.4.5 易错点、失效与考点](./3.4-introducing-iterators/3.4.5-易错点失效与考点.md)
- [3.5 数组](./3.5-arrays/3.5-数组.md)
  - [3.5.1 定义与初始化](./3.5-arrays/3.5.1-定义与初始化.md)
  - [3.5.2 下标访问与越界](./3.5-arrays/3.5.2-下标访问与越界.md)
  - [3.5.3 数组退化与指针](./3.5-arrays/3.5.3-数组退化与指针.md)
  - [3.5.4 begin/end 与 vector 对比](./3.5-arrays/3.5.4-begin-end与vector对比.md)
  - [3.5.5 易错点与示例](./3.5-arrays/3.5.5-易错点与示例.md)
- [3.6 多维数组](./3.6-多维数组.md)
- [小结](./3.7-小结.md)


## 章节摘要

C++ 标准库核心类型：`string`（可变长字符串）、`vector`（可变长动态数组）、迭代器（统一遍历方式）和内置数组。这些替代了 C 的 `char[]`/`malloc`/手动指针管理。

### 和 C 的区别

| C | C++ |
|---|-----|
| `char str[100]` 固定长度 | `std::string` 自动管理长度 |
| `int *arr = malloc(n*sizeof(int))` | `std::vector<int>` 自动管理内存 |
| 指针遍历 `p++` | 迭代器 `it++`，类型安全 |
| `qsort` 需函数指针 | `std::sort` + lambda，类型安全+可内联 |
| 数组退化 `int[]`→`int*` | 同样退化，但 `begin()`/`end()` 更安全 |

## 章节自测

### Q1: string vs char[]

```cpp
#include <string>
std::string s1 = "hello";
std::string s2 = s1;      // 拷贝
s2[0] = 'H';
// s1 和 s2 分别是什么？
```

> s1 和 s2 分别是什么？如果是 `char s1[] = "hello"; char *s2 = s1;` 呢？

<details>
<summary>答案与复习指引</summary>

**string 版本：** `s1 = "hello"`, `s2 = "Hello"`。`string` 拷贝是**深拷贝**——各自独立内存，互不影响。

**char[] 版本：** `s2` 是指向 `s1` 的指针（浅拷贝），`s2[0] = 'H'` 也会修改 `s1`。这是 C 字符串的经典 bug 来源。

**复习：** → [3.2.1 定义与初始化](./3.2-library-type-string/3.2.1-定义与初始化.md)
</details>

### Q2: vector 初始化陷阱

```cpp
std::vector<int> v1(10, 20);    // A
std::vector<int> v2{10, 20};    // B
```

> v1 和 v2 分别是什么？圆括号和花括号的区别？

<details>
<summary>答案与复习指引</summary>

- `v1` = 10 个元素，每个值 20 → `{20, 20, 20, ..., 20}`（10 个）
- `v2` = 2 个元素 → `{10, 20}`

**区别：** `(10, 20)` 是"构造函数参数"——10 个 20；`{10, 20}` 是"初始化列表"——两个元素 10 和 20。花括号优先匹配 `initializer_list` 构造函数。

**复习：** → [3.3.2 圆括号与花括号初始化](./3.3-library-type-vector/3.3.2-圆括号与花括号初始化.md)
</details>

### Q3: 迭代器失效

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};
auto it = v.begin();
v.push_back(6);
// *it  // 安全吗？
```

> `push_back` 后 `it` 还有效吗？为什么？

<details>
<summary>答案与复习指引</summary>

**不安全。** `push_back` 可能触发扩容（重新分配内存），扩容后所有迭代器、指针、引用全部失效。即使没扩容，`push_back` 也可能使 `end()` 迭代器失效。

**规则：** 对 `vector`，`push_back` 后如果 `size() == capacity()`（发生了扩容），所有迭代器失效。安全的做法是 `push_back` 后重新获取迭代器。

**复习：** → [3.3.3 下标与增删](./3.3-library-type-vector/3.3.3-下标与增删.md) · [3.4.5 易错点、失效与考点](./3.4-introducing-iterators/3.4.5-易错点失效与考点.md)
</details>

### Q4: 数组退化

```cpp
void print_size(int arr[]) {
    std::cout << sizeof(arr);  // 输出什么？
}
int main() {
    int data[10];
    std::cout << sizeof(data);  // 输出什么？
    print_size(data);
}
```

> 两个 `sizeof` 分别输出什么（假设 int=4，指针=8）？为什么不同？

<details>
<summary>答案与复习指引</summary>

- `sizeof(data)` = 40（10 × 4 字节）
- `sizeof(arr)` = 8（指针大小）——数组退化为指针

**根因：** 数组作为函数参数传递时退化为指向首元素的指针，丢失长度信息。这是 C/C++ 共有的"数组退化"陷阱。C++ 的解决：传 `std::array` 或 `std::vector`，或用模板引用保持数组类型。

**复习：** → [3.5.3 数组退化与指针](./3.5-arrays/3.5.3-数组退化与指针.md)
</details>

### Q5: begin/end

```cpp
int arr[] = {10, 20, 30, 40, 50};
auto p = std::begin(arr);
auto q = std::end(arr);
std::cout << *p << " " << *(q - 1) << " " << q - p;
```

> 输出是什么？`std::end(arr)` 指向什么？

<details>
<summary>答案与复习指引</summary>

**输出：** `10 50 5`

- `*p` = 10（首元素）
- `*(q-1)` = 50（末元素，因为 `end` 指向最后一个元素的下一个位置）
- `q - p` = 5（元素个数，指针差值 = 元素数）

**`std::end(arr)` 指向"尾后位置"（past-the-end）**——不指向任何有效元素，是哨兵位置。

**复习：** → [3.5.4 begin/end 与 vector 对比](./3.5-arrays/3.5.4-begin-end与vector对比.md)
</details>
