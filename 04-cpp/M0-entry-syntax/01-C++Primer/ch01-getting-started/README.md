# 第 1 章 开始

本章介绍 C++ 的大部分基础内容，包括类型、变量、表达式、语句及函数，帮助读者具备编写、编译及运行简单程序的能力。

## 小节

- [1.1 编写一个简单的 C++ 程序](./1.1-编写一个简单的C++程序.md)
- [1.2 初识输入输出](./1.2-初识输入输出.md)
- [1.3 注释简介](./1.3-注释简介.md)
- [1.4 控制流](./1.4-控制流.md)
- [1.5 类简介](./1.5-a-brief-introduction-to-classes/1.5-类简介.md)
  - [1.5.1 类的使用与头文件分离](./1.5-a-brief-introduction-to-classes/1.5.1-类的使用与头文件分离.md)
  - [1.5.2 链接、编译产物与示例](./1.5-a-brief-introduction-to-classes/1.5.2-链接编译产物与示例.md)
- [1.6 书店程序](./1.6-书店程序.md)
- [小结与术语表](./1.7-小结与术语表.md)


## 章节摘要

本章是 C++ 入门：编写、编译、运行一个简单程序，理解 `main` 函数、基本 I/O（`cin`/`cout`）、注释、控制流（`while`/`for`/`if`）以及类简介。

### 和 C 的区别

| C | C++ |
|---|-----|
| `#include <stdio.h>` | `#include <iostream>` |
| `printf`/`scanf` | `std::cout`/`std::cin`（类型安全） |
| `/* */` 注释 | `//` 单行注释（C99 起也有） |
| 手写 struct + 函数 | class 封装数据 + 操作 |
| `return 0` 表示成功 | 同 C，但 `main` 可省略 return（隐式返回 0） |

## 章节自测

### Q1: cin/cout vs scanf/printf

```cpp
#include <iostream>
int main() {
    int a, b;
    std::cin >> a >> b;
    std::cout << "sum = " << (a + b) << std::endl;
}
// 输入: 3 5
```

> 输出是什么？`std::endl` 和 `\n` 有什么区别？`cin >> a >> b` 如何分隔输入？

<details>
<summary>答案与复习指引</summary>

**输出：** `sum = 8`

**解析：**
- `cin >> a >> b` 以空白（空格/Tab/换行）分隔输入，连续读取两个 int
- `std::endl` = 输出换行 + **刷新缓冲区**（flush）；`\n` 只输出换行不刷新。频繁 `endl` 有性能开销
- `cout` 是类型安全的——编译器根据 `a+b` 的类型自动选择输出格式，不像 `printf` 要手写 `%d`

**复习：** → [1.2 初识输入输出](./1.2-初识输入输出.md)
</details>

### Q2: main 返回值

```cpp
int main() {
    // 没有 return 语句
}
```

> 这段代码合法吗？返回值是什么？C 里也这样吗？

<details>
<summary>答案与复习指引</summary>

**合法。** C++ 标准规定 `main` 函数如果没写 `return`，隐式返回 0（表示成功）。

**和 C 的区别：** C89 也隐式返回 0（但标准说"行为未定义"到 C99 才明确返回 0）。实践中都建议显写 `return 0;`。

**复习：** → [1.1 编写一个简单的 C++ 程序](./1.1-编写一个简单的C++程序.md)
</details>

### Q3: while 循环与累加

```cpp
#include <iostream>
int main() {
    int sum = 0, val = 1;
    while (val <= 10) {
        sum += val;
        ++val;
    }
    std::cout << "Sum = " << sum << std::endl;
}
```

> 输出是什么？`++val` 和 `val++` 在这里有什么区别？

<details>
<summary>答案与复习指引</summary>

**输出：** `Sum = 55`（1+2+...+10=55）

**解析：**
- `++val`（前置递增）返回递增后的值；`val++`（后置递增）返回递增前的值
- 对内置类型 `int`，两者性能无差别；对迭代器/复杂类型，前置 `++it` 更快（后置需要保存旧值）
- 习惯上 C++ 优先用前置 `++`，尤其对迭代器

**复习：** → [1.4 控制流](./1.4-控制流.md)
</details>

### Q4: for 循环

```cpp
#include <iostream>
int main() {
    int sum = 0;
    for (int i = -100; i <= 100; ++i)
        sum += i;
    std::cout << sum << std::endl;
}
```

> 输出是什么？为什么？

<details>
<summary>答案与复习指引</summary>

**输出：** `0`

**解析：** -100 到 100 的整数求和，正负抵消（-100+100=0, -99+99=0, ...），只剩 0。

**复习：** → [1.4 控制流](./1.4-控制流.md)
</details>

### Q5: Sales_data 类简介

```cpp
struct Sales_data {
    std::string bookNo;
    unsigned units_sold = 0;
    double revenue = 0.0;
};
```

> 这个 struct 和 C 的 struct 有什么区别？`= 0` 是什么？

<details>
<summary>答案与复习指引</summary>

**和 C 的区别：**
1. C++ struct 可以有**类内初始值**（`= 0`），C 不行
2. C++ struct 成员可以是 `std::string` 等有构造/析构的类型，C 不行
3. C++ struct 默认访问权限是 public（class 是 private），C 没有访问控制

**类内初始值** `= 0`：创建对象时如果不显式初始化，成员自动用这个值。C 没有此特性。

**复习：** → [1.5 类简介](./1.5-a-brief-introduction-to-classes/1.5-类简介.md)
</details>
