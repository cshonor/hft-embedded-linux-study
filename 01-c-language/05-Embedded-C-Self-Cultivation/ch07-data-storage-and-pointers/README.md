# 第 7 章 数据存储与指针

**Data Storage, Types & Pointers for Kernel, Driver & DPDK**

## 本章目标

建立**数据表示 → 对齐布局 → 类型抽象 → 指针运算**的完整链条。掌握大小端、溢出、强制转换陷阱；读懂 `size_t`、`typedef`、`enum` 在内核中的用法；熟练 **指针/数组/decay**、**二级指针**、**结构体与 offsetof**、**函数指针跳转表**；理解 **const 四形式**、**volatile**（MMIO/IRQ）、**restrict** 与 **void\***。能结合 GDB 调试指针，阅读 Linux 驱动与 DPDK 缓冲区代码。衔接 **ch06 GNU 扩展**、**ch08 位操作**、**ch09 模块化**。

## 前置依赖

| 章节 | 内容 |
|------|------|
| **[ch04](../ch04-compile-link-install-run/)** | 编译链接、ELF 段、符号、`readelf`/`nm` |
| **[ch05](../ch05-memory-stack-management/)** | 栈/堆、`malloc`、对象生命周期 |
| **[ch06](../ch06-gnu-c-extensions/)** | `packed`/`aligned`、`offsetof`、`container_of`、FAM |

## 环境

- **编译器**：GCC 或 Clang，**`-std=gnu11 -Wall -Wextra`**
- **工具**：`gdb`、`gcc -fdump-record-layouts`、`readelf`（复习 ch04）
- **头文件**：`<stdint.h>`、`<stddef.h>`、`<string.h>`
- **拓展**：**demo/**（见下）

## 快速操作 Demo

```bash
cd 00-Linux-Kernel-DPDK-Network-C/05-Embedded-C-Self-Cultivation/ch07-data-storage-and-pointers/demo

make all

# 数组 vs 指针 sizeof、decay
./demo01_array_ptr
gdb -batch -ex run -ex 'print sizeof(buf)' -ex 'print sizeof(p)' -ex 'x/16xb buf' ./demo01_array_ptr

./demo02_double_ptr
./demo03_struct_offset
./demo04_func_ptr

# volatile 汇编对比
make demo05_volatile CFLAGS="-g -O2 -Wall -std=gnu11"
objdump -d demo05_volatile | grep -A8 poll_volatile

./demo06_dangling
./demo07_mmio

make clean
```

## 八大知识模块

| 模块 | 目录 | 核心 |
|------|------|------|
| **1 数据表示** | **7.1** | 字节序、有符号/无符号、溢出、强制转换 |
| **2 对齐布局** | **7.2**、**7.10** | 结构体/联合体 padding、`offsetof`、`packed` |
| **3 可移植类型** | **7.3**、**7.4** | 定宽类型、`size_t`、ABI 差异 |
| **4 类型抽象** | **7.5**、**7.6** | `typedef`、`enum`、内核命名惯例 |
| **5 常量与修饰** | **7.7**、**7.13** | `const` 四形式、`volatile`、`restrict` |
| **6 指针基础** | **7.8** | `&`/`*`、`NULL`、复杂声明、指针算术、GDB |
| **7 数组关系** | **7.9**、**7.11** | decay、`sizeof`、指针数组、`char **` |
| **8 高阶指针** | **7.10**、**7.12** | 结构体指针、FAM、函数指针、jump table |

## Demo 清单

| Demo | 内容 | 对应小节 |
|------|------|----------|
| **demo01** | 数组 vs 指针 `sizeof`、地址、传参 decay | **7.9**、**7.8** |
| **demo02** | `char **` 动态列表、`realloc` 改指针 | **7.11** |
| **demo03** | `offsetof` normal/packed、`->` | **7.10**、**7.2** |
| **demo04** | `typedef` 函数指针、enum jump table | **7.12**、**7.6** |
| **demo05** | `volatile` MMIO/标志位 | **7.7**、**7.13** |
| **demo06** | 悬垂/野指针 | **7.8.3** |
| **demo07** | MMIO `REG32`/`BIT` 宏 | **7.13**、**7.1.1** |

## 考核要点

1. 说明**小端/大端**在内存中的字节排列，并写出检测宏或 union 探测法（**7.1.1**）
2. 解释**有符号溢出 UB** 与**无符号回绕**在环形缓冲区中的不同处理（**7.1.3**）
3. 手算含 padding 的结构体 `sizeof` 与 `offsetof`，对比 **packed**（**7.2.2**、**demo03**）
4. 区分 **`size_t` 与 `int`** 比较陷阱，正确使用 `%zu`（**7.4**）
5. 写出 **const 指针四形式** 并说明 `typedef int *P` 与 `const` 组合结果（**7.7**）
6. 用表格对比**数组与指针**在赋值、`sizeof`、传参三方面的差异（**7.9**、**demo01**）
7. 解析声明 `int *(*fp)(int *)` 与 `char *str[10]`，区分指针数组/数组指针（**7.8.2**、**7.9.3**）
8. 说明为何 `realloc` 需要 **`T **` 参数** 才能更新调用方指针（**7.11.1**、**demo02**）
9. 实现 **enum + 函数指针 jump table** 分派，含边界检查（**7.12**、**demo04**）
10. 解释 **`volatile`** 与 **`restrict`** 在 MMIO 与 `memcpy` 优化中的角色（**7.13**、**demo05/demo07**）

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | **ch04** ELF/符号；**ch05** 堆栈；**ch06** `packed`/`container_of`/FAM |
| 后置 | **ch08** 位操作与 OOP 风格 ops；**ch09** 头文件与模块化；**ch10** OS/驱动模型 |

## 小节

- [7.1 数据类型与存储](./7.1-data-types/7.1-数据类型与存储.md)
  - [7.1.1 大端模式与小端模式](./7.1-data-types/7.1.1-大端模式与小端模式.md)
  - [7.1.2 有符号数和无符号数](./7.1-data-types/7.1.2-有符号数和无符号数.md)
  - [7.1.3 数据溢出](./7.1-data-types/7.1.3-数据溢出.md)
  - [7.1.4 数据类型转换](./7.1-data-types/7.1.4-数据类型转换.md)
- [7.2 数据对齐](./7.2-alignment/7.2-数据对齐.md)
  - [7.2.1 为什么要数据对齐](./7.2-alignment/7.2.1-为什么要数据对齐.md)
  - [7.2.2 结构体对齐](./7.2-alignment/7.2.2-结构体对齐.md)
  - [7.2.3 联合体对齐](./7.2-alignment/7.2.3-联合体对齐.md)
- [7.3 数据的可移植性](./7.3-数据的可移植性.md)
- [7.4 Linux内核中的size_t类型](./7.4-Linux内核中的size_t类型.md)
- [7.5 为什么很多人编程时喜欢用typedef](./7.5-typedef/7.5-为什么很多人编程时喜欢用typedef.md)
  - [7.5.1 typedef的基本用法](./7.5-typedef/7.5.1-typedef的基本用法.md)
  - [7.5.2 使用typedef的优势](./7.5-typedef/7.5.2-使用typedef的优势.md)
  - [7.5.3 使用typedef需要注意的地方](./7.5-typedef/7.5.3-使用typedef需要注意的地方.md)
  - [7.5.4 typedef的作用域](./7.5-typedef/7.5.4-typedef的作用域.md)
  - [7.5.5 如何避免typedef被大量滥用](./7.5-typedef/7.5.5-如何避免typedef被大量滥用.md)
- [7.6 枚举类型](./7.6-enum/7.6-枚举类型.md)
  - [7.6.1 使用枚举的三种方法](./7.6-enum/7.6.1-使用枚举的三种方法.md)
  - [7.6.2 枚举的本质](./7.6-enum/7.6.2-枚举的本质.md)
  - [7.6.3 Linux内核中的枚举类型](./7.6-enum/7.6.3-Linux内核中的枚举类型.md)
  - [7.6.4 使用枚举需要注意的地方](./7.6-enum/7.6.4-使用枚举需要注意的地方.md)
- [7.7 常量和变量](./7.7-constants/7.7-常量和变量.md)
  - [7.7.1 变量的本质](./7.7-constants/7.7.1-变量的本质.md)
  - [7.7.2 常量存储](./7.7-constants/7.7.2-常量存储.md)
  - [7.7.3 常量折叠](./7.7-constants/7.7.3-常量折叠.md)
- [7.8 从变量到指针](./7.8-pointers-basics/7.8-从变量到指针.md)
  - [7.8.1 指针的本质](./7.8-pointers-basics/7.8.1-指针的本质.md)
  - [7.8.2 一些复杂的指针声明](./7.8-pointers-basics/7.8.2-一些复杂的指针声明.md)
  - [7.8.3 指针类型与运算](./7.8-pointers-basics/7.8.3-指针类型与运算.md)
- [7.9 指针与数组的「暧昧」关系](./7.9-arrays/7.9-指针与数组的-暧昧-关系.md)
  - [7.9.1 下标运算符[]](./7.9-arrays/7.9.1-下标运算符.md)
  - [7.9.2 数组名的本质](./7.9-arrays/7.9.2-数组名的本质.md)
  - [7.9.3 指针数组与数组指针](./7.9-arrays/7.9.3-指针数组与数组指针.md)
- [7.10 指针与结构体](./7.10-指针与结构体.md)
- [7.11 二级指针](./7.11-double-pointer/7.11-二级指针.md)
  - [7.11.1 修改指针变量的值](./7.11-double-pointer/7.11.1-修改指针变量的值.md)
  - [7.11.2 二维指针和指针数组](./7.11-double-pointer/7.11.2-二维指针和指针数组.md)
  - [7.11.3 二级指针和二维数组](./7.11-double-pointer/7.11.3-二级指针和二维数组.md)
- [7.12 函数指针](./7.12-函数指针.md)
- [7.13 重新认识void](./7.13-重新认识void.md)
