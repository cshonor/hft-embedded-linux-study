# 第 4 章 表达式

本章介绍 C++ 丰富的运算符，以及它们作用于内置类型时的操作与隐式类型转换规则。对于含多个运算符的复杂表达式，需理解**优先级（precedence）、结合律（associativity）和求值顺序（order of evaluation）**。

## 小节

- [4.1 基础](./4.1-基础.md)
- [4.2–4.10 各种运算符详解](./4.2-4.10-operators/4.2-4.10各种运算符详解.md)
  - [4.2 算术运算符](./4.2-4.10-operators/4.2-算术运算符.md)
  - [4.3 关系与逻辑运算符](./4.2-4.10-operators/4.3-关系与逻辑运算符.md)
  - [4.4 赋值运算符](./4.2-4.10-operators/4.4-赋值运算符.md)
  - [4.5 自增自减](./4.2-4.10-operators/4.5-自增自减.md)
  - [4.6 成员访问运算符](./4.2-4.10-operators/4.6-成员访问运算符.md)
  - [4.7 条件运算符](./4.2-4.10-operators/4.7-条件运算符.md)
  - [4.8 位运算符](./4.2-4.10-operators/4.8-位运算符.md)
  - [4.9 sizeof 运算符](./4.2-4.10-operators/4.9-sizeof运算符.md)
  - [4.10 逗号运算符](./4.2-4.10-operators/4.10-逗号运算符.md)
- [4.11 类型转换](./4.11-type-conversions/4.11-类型转换.md)
  - [4.11.1 隐式类型转换](./4.11-type-conversions/4.11.1-隐式类型转换.md)
  - [4.11.2 显式转换与四种 cast](./4.11-type-conversions/4.11.2-显式转换与四种cast.md)
  - [4.11.3 规范、易错点与示例](./4.11-type-conversions/4.11.3-规范易错点与示例.md)
- [4.12 运算符优先级表](./4.12-operator-precedence/4.12-运算符优先级表.md)
  - [4.12.1 优先级与结合律表](./4.12-operator-precedence/4.12.1-优先级与结合律表.md)
  - [4.12.2 求值顺序与括号规范](./4.12-operator-precedence/4.12.2-求值顺序与括号规范.md)


## 章节摘要

C++ 运算符体系、优先级与结合律、求值顺序（C++17 前未指定多个 `<<` 的求值顺序）、隐式类型转换（整型提升、算术转换）与四种显式转换（`static_cast`/`dynamic_cast`/`const_cast`/`reinterpret_cast`）。

### 和 C 的区别

| C | C++ |
|---|-----|
| `(int)x` C 风格强转 | `static_cast<int>(x)` 四种 cast |
| 求值顺序大多未指定 | C++17 起 `<<`/`>>` 求值顺序确定 |
| 无 `dynamic_cast` | 有运行时类型检查的转换 |
| `&&`/`\|\|` 短路 | 相同 |

## 章节自测

### Q1: 求值顺序坑

```cpp
int i = 0;
std::cout << i << " " << ++i;  // C++14 之前：未定义行为？
```

> C++14 和 C++17 分别输出什么？为什么？

<details>
<summary>答案与复习指引</summary>

**C++14：** 未定义行为（UB）。`<<` 的左右操作数求值顺序未指定，可能先算 `++i` 再算 `i`，输出 `1 1`。

**C++17：** 输出 `0 1`。C++17 规定 `<<` 的求值顺序从左到右。

**教训：** 不要在同一表达式中修改并读取同一变量。即使 C++17 修了这个，其他运算符的求值顺序仍可能未指定。

**复习：** → [4.12.2 求值顺序与括号规范](./4.12-operator-precedence/4.12.2-求值顺序与括号规范.md)
</details>

### Q2: 四种 cast

```cpp
const char *pc = "hello";
// char *p = static_cast<char*>(pc);      // A
char *p = const_cast<char*>(pc);           // B
p[0] = 'H';                                // C: UB?
```

> A 行为什么编译失败？C 行是 UB 吗？

<details>
<summary>答案与复习指引</summary>

**A 行失败：** `static_cast` 不能去掉 `const`。去掉 `const` 必须用 `const_cast`。

**C 行是 UB：** 字符串字面量 `"hello"` 存储在只读区，通过 `const_cast` 去掉 const 后修改只读内存是未定义行为（可能段错误）。`const_cast` 只用于"原本不是 const 但被 const 接口传出来"的场景。

**四种 cast 用途：**
- `static_cast`：常规类型转换（int→double, void*→int*）
- `dynamic_cast`：多态向下转型（运行时检查）
- `const_cast`：增删 const
- `reinterpret_cast`：位模式重新解释（最危险）

**复习：** → [4.11.2 显式转换与四种 cast](./4.11-type-conversions/4.11.2-显式转换与四种cast.md)
</details>

### Q3: 隐式转换陷阱

```cpp
unsigned int u = 10;
int i = -42;
std::cout << i + i << std::endl;   // A
std::cout << u + i << std::endl;   // B
```

> A 和 B 分别输出什么？为什么？

<details>
<summary>答案与复习指引</summary>

**A: `-84`**（两个 int 相加，正常）
**B: `4294967254`**（`i` 被隐式转换为 `unsigned int`，-42 变成 4294967254，再加 10 = 4294967264... 实际取决于 unsigned 的位宽）

**根因：** 当 `int` 和 `unsigned` 混合运算时，`int` 被隐式转换为 `unsigned`。负数转为 unsigned 会变成很大的正数。这是 C/C++ 共有的经典 bug 来源。

**复习：** → [4.11.1 隐式类型转换](./4.11-type-conversions/4.11.1-隐式类型转换.md)
</details>

### Q4: 条件运算符

```cpp
int grade = 85;
std::string final = (grade < 60) ? "fail" : "pass";
// 嵌套条件运算符
std::string level = (grade >= 90) ? "A" : (grade >= 80) ? "B" : (grade >= 70) ? "C" : "F";
```

> level 是什么？条件运算符的结合方向是什么？

<details>
<summary>答案与复习指引</summary>

**level = "B"**（85 >= 80）

条件运算符 `?:` 是**右结合**的——嵌套条件运算符从右往左组合，等价于 `grade>=90 ? "A" : (grade>=80 ? "B" : (grade>=70 ? "C" : "F"))`。这与 if-else if-else 链的语义一致。

**复习：** → [4.7 条件运算符](./4.2-4.10-operators/4.7-条件运算符.md)
</details>

### Q5: sizeof 与类型

```cpp
struct Empty {};
class Base { virtual void f() {} };
struct Derived : Base { int x; };

std::cout << sizeof(Empty) << " " << sizeof(Base) << " " << sizeof(Derived);
```

> 输出是什么（64 位系统）？为什么 `sizeof(Empty)` 不为 0？

<details>
<summary>答案与复习指引</summary>

**输出：** `1 8 16`（具体取决于编译器/对齐）

- `sizeof(Empty) = 1`：C++ 规定空类大小不为 0（通常为 1），保证不同对象地址唯一
- `sizeof(Base) = 8`：有虚函数的类含 vptr（8 字节指针）
- `sizeof(Derived) = 16`：vptr（8）+ int x（4）+ padding（4，对齐到 8）

**复习：** → [4.9 sizeof 运算符](./4.2-4.10-operators/4.9-sizeof运算符.md)
</details>
