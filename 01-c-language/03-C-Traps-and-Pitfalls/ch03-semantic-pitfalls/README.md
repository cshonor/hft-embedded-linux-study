# 第 3 章 语义「陷阱」

**Semantic Pitfalls** — Andrew Koenig, *C Traps and Pitfalls*

## 本章目标

词法（[ch01](../ch01-lexical-pitfalls/)）解决 **怎么切 token**，语法（[ch02](../ch02-syntactic-pitfalls/)）解决 **怎么组语句**；**语义** 决定 **类型、内存、指针、运算的实际含义**。

> 全书 **最难、底层踩坑最多** 的一章：多为 **UB、隐式转换、指针/数组混淆**，编译器 **不报错**，运行 **随机出错**。

## 小节索引

| 节 | 主题 |
|----|------|
| [3.1](./3.1-指针与数组混淆.md) | 数组 decay、`sizeof(ptr)` 陷阱 |
| [3.2](./3.2-空指针解引用.md) | `NULL` 解引用 UB |
| [3.3](./3.3-有符号无符号转换.md) | `int` vs `unsigned` 比较、死循环 |
| [3.4](./3.4-整型溢出.md) | 有符号 UB vs 无符号回绕 |
| [3.5](./3.5-指针运算规则.md) | 步长、`void*` |
| [3.6](./3.6-实参求值顺序.md) | `f(i++, i)` UB |
| [3.7](./3.7-数组下标越界.md) | 无边界检查 |
| [3.8](./3.8-结构体与位域.md) | padding、位域符号 |
| [3.9](./3.9-字符串字面量只读.md) | `"..."` 不可写 |

## 工程强制规范（内核 / HFT / 嵌入式）

1. 传数组 **必带长度**；禁止 `sizeof(ptr)` 当缓冲区大小
2. **不混用** 有符号/无符号比较；循环计数类型统一
3. 解引用前 **判空**
4. 实参 **无副作用**（无 `++`/`--` 混用）
5. 字符串字面量 **不修改**；用 `char[]` 或 `const char *`
6. 有符号累加考虑 **溢出**；长度/索引用 `size_t`

## 前后章节

| | 章节 |
|---|------|
| **前置** | [ch02 语法](../ch02-syntactic-pitfalls/) |
| **后置** | [ch04 连接](../ch04-linking/) — extern、静态符号 |
| **交叉** | [Expert C ch04–ch07](../04-Expert-C-Programming/) |

## Demo

```bash
cd demo && make all
./demo01_array_ptr/main
./demo02_signed_unsigned/main
./demo03_unsigned_loop/main
./demo04_arg_order/main
./demo05_bounds/main
./demo06_ro_string/main
```

## 面试题

1. `sizeof(arr)` vs `sizeof(p)`，`p=arr`？
2. `int i=-1; unsigned u=10; i<u` 结果？
3. `for (unsigned i=n; i>=0; i--)` 为何死循环？
4. 有符号 vs 无符号溢出标准差异？
5. `char *s="x"; s[0]='y'` 为何错？
