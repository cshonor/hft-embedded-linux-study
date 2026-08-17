# 第 3 章 数据

**Data**

## 本章讲什么

**底层数据存储与二进制内存模型**：整型/浮点/指针类型、**常量**与 **enum**、**const/volatile/restrict**、**typedef** 定长、**作用域/链接/存储期** 与 **.text/.data/.bss/.rodata/栈/堆**。看懂 DPDK mbuf、内核 struct、交易报文、寄存器读写的地基。

## 学习重点

- 定宽整型、**整数提升**、**unsigned 混合比较**
- HFT：**定点价格** vs float；**htonl** 大小端
- **const** 四式指针、**volatile** MMIO、**restrict** 优化
- **.data/.bss/.rodata** 与初始化/脏栈
- **static** 双义、**extern** 链接
- **enum**、字符串 **.rodata** 只读

## 场景价值

| 方向 | 本章技能 |
|------|----------|
| 驱动/寄存器 | volatile + uint8/16 + 十六进制常量 |
| 报文解析 | 无符号字节、endian、隐式转换 |
| DPDK | typedef、const 缓冲、restrict、enum 类型 |
| 低延迟 | 段划分、减少冗余访存 |

## 实操（建议完成）

1. `sizeof` 各类型（32/64 位）  
2. 改字符串常量 → 段错误  
3. `unsigned char` vs `-1` 比较  
4. volatile 对比汇编（`gcc -S -O2`）  
5. enum + typedef  
6. `readelf -S`  
7. 自实现 **htonl**  

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | ch02 作用域、static/extern |
| 后序 | ch05 运算；ch06 指针；ch10 对齐；ch18 运行时 |
| 配套 | 《C陷阱与缺陷》ch03、ch07 |

## 小节

- 3.1 基本数据类型
  - [3.1.1 整型家族](./3.1-basic-data-types/3.1.1-整型家族.md)
  - [3.1.2 浮点类型](./3.1-basic-data-types/3.1.2-浮点类型.md)
  - [3.1.3 指针](./3.1-basic-data-types/3.1.3-指针.md)
- [3.2 基本声明](3.2-basic-declarations/3.2-基本声明.md)
  - [3.2.1 初始化](./3.2-basic-declarations/3.2.1-初始化.md)
  - [3.2.2 声明简单数组](./3.2-basic-declarations/3.2.2-声明简单数组.md)
  - [3.2.3 声明指针](./3.2-basic-declarations/3.2.3-声明指针.md)
  - [3.2.4 隐式声明](./3.2-basic-declarations/3.2.4-隐式声明.md)
- [3.3 typedef](./3.3-typedef.md)
- [3.4 常量](./3.4-常量.md)
- [3.5 作用域](3.5-scope/3.5-作用域.md)
  - [3.5.1 代码块](./3.5-scope/3.5.1-代码块.md)
  - [3.5.2 文件](./3.5-scope/3.5.2-文件.md)
  - [3.5.3 原型](./3.5-scope/3.5.3-原型.md)
  - [3.5.4 函数](./3.5-scope/3.5.4-函数.md)
- [3.6 链接属性](./3.6-链接属性.md)
- [3.7 存储类型](./3.7-存储类型.md)
- [3.8 static 关键字](./3.8-static关键字.md)
- [3.9 作用域、存储类型示例](./3.9-作用域-存储类型示例.md)
- [3.10 左值（lvalue）](./3.10-左值.md) — 补充：左值定义、可修改性、与指针的关系


---

## 章节自测

> 看代码 → 想答案 → 点开验证。

### Q1: const 四式指针

```c
int val = 42;
const int *p1 = &val;        // (1)
int * const p2 = &val;       // (2)
const int * const p3 = &val; // (3)
int *p4 = &val;             // (4)

// p1 = &other;   // OK? 通过 p1 改 val?
// *p2 = 99;      // OK? p2 = &other?
```

> 四种 const 分别限制什么？

<details>
<summary>答案与复习指引</summary>

**答案：**
| 声明 | 指针可变 | 指向的数据可变 |
|------|---------|--------------|
| `const int *p1` | ✅ | ❌ |
| `int *const p2` | ❌ | ✅ |
| `const int *const p3` | ❌ | ❌ |
| `int *p4` | ✅ | ✅ |

**口诀：** `const` 在 `*` 左边修饰数据，在 `*` 右边修饰指针。

**复习：** → 3.3 Constants — const 指针

</details>

### Q2: volatile MMIO

```c
// 硬件寄存器映射
volatile unsigned int *reg = (volatile unsigned int*)0x40021000;

// 不加 volatile 会怎样？
// unsigned int *reg = (unsigned int*)0x40021000;
// while (*reg & READY_BIT) ;
```

> `volatile` 防止编译器做什么？

<details>
<summary>答案与复习指引</summary>

**答案：** `volatile` 防止编译器将 `*reg` 的读取**缓存到寄存器**。不加 `volatile`，编译器看到 `while (*reg & READY_BIT)` 中循环体不修改 `*reg`，可能优化为只读一次 → 死循环或错过状态变化。

`volatile` 强制每次访问都从内存重新读取。用于 MMIO（内存映射 I/O）、中断修改的变量、信号处理中的变量。

**注意：** `volatile` 不保证原子性，也不加内存屏障。

**复习：** → 3.3 Constants — volatile

</details>

### Q3: 整数提升与 unsigned 比较

```c
if (sizeof(int) > -1)
    printf("yes\n");
else
    printf("no\n");
```

> 输出 yes 还是 no？为什么？

<details>
<summary>答案与复习指引</summary>

**输出：** `no`

**解析：** `sizeof` 返回 `size_t`（无符号）。比较 `size_t > -1` 时，`-1` 被转为无符号 = `0xFFFFFFFFFFFFFFFF`（64 位），远大于 `4`。

这是无符号与有符号混算的经典陷阱。

**复习：** → 3.2 Basic Types — 整数提升与 unsigned 混合

</details>

### Q4: typedef 定宽

```c
typedef uint32_t u32;
typedef int64_t  i64;

u32 addr = 0xDEADBEEF;
i64 timestamp = 1699000000;
```

> 为什么 HFT / 内核用 `uint32_t` 而不是 `int`？

<details>
<summary>答案与复习指引</summary>

**答案：** `int` 宽度跨平台不固定（2/4/8 字节）。`uint32_t`（`<stdint.h>`）保证恰好 4 字节。协议字段、寄存器、ABI 接口需要精确宽度。

**复习：** → 3.4 typedef

</details>

---

## 代码自测

**题目 1：** 以下代码在 C 语言中能编译吗？说明了 C 的什么特性？
```c
#include <stdio.h>
int main() {
    printf("hello\n");
}
```

<details>
<summary>参考答案</summary>

能编译。C 是编译型语言——源代码经过预处理、编译、汇编、链接四个阶段生成可执行文件。这个程序体现了 C 的基本结构：包含头文件、main 函数入口、标准库函数调用。C 的设计哲学是"信任程序员"——它不会像 Java 那样强制检查很多东西。

</details>
