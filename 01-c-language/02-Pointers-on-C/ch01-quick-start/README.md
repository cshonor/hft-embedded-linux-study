# 第 1 章 快速上手

**Quick Start**

## 本章讲什么

全书**铺垫章**：最小可运行 C 程序、**WSL/Linux 编译四阶段**、运行与 **gdb**、极简 I/O 与 **`&` 取地址**。不深入语法，只回答：**一段 C 代码从编写到 CPU 执行的完整链路** —— 后续 DPDK、内核、交易程序构建流程的缩影。

## 学习重点

- 最简骨架：`#include`、`int main(void)`、`printf`、`return 0`
- **预处理 → 编译 → 汇编 → 链接**；动态 vs **`-static`**
- **`scanf(..., &val)`** 与返回值校验；**stdio vs write** 延迟
- 栈上大数组风险；**gdb** 断点与段错误定位
- 书中示例：**read_column_numbers** + **rearrange** 串联 I/O 与控制流

## 场景价值

| 方向 | 本章收获 |
|------|----------|
| DPDK / 内核 | 四阶段 = 所有底层构建本质 |
| HFT | 静态链接、绕过 stdio 缓冲的意识 |
| 嵌入式 | 栈限制、退出码、Makefile 模板 |

## 前后章节

| 方向 | 章节 |
|------|------|
| 前序 | 无（全书开篇） |
| 后序 | ch02 基本概念；ch03 数据；ch06 指针；ch15 I/O；ch18 运行时 |
| 配套 | 《C陷阱与缺陷》ch01/ch02 规避词法新手 bug |

## 实操（建议完成）

1. `-E/-S/-c` 查看各阶段产物  
2. 动态 vs 静态二进制大小  
3. gdb 断点 + `print &变量`  
4. 故意省略 `&` 用 gdb 定位  
5. 循环读入 + scanf 返回值容错  

## 小节

- [1.1 简介](./1.1-introduction/1.1-introduction.md)
  - [1.1.1 空白和注释](./1.1-introduction/1.1.1-空白和注释.md)
  - [1.1.2 预处理指令](./1.1-introduction/1.1.2-预处理指令.md)
  - [1.1.3 main 函数](./1.1-introduction/1.1.3-main函数.md)
  - [1.1.4 read_column_numbers 函数](./1.1-introduction/1.1.4-read_column_numbers函数.md)
  - [1.1.5 rearrange 函数](./1.1-introduction/1.1.5-rearrange函数.md)
- [1.2 补充说明](./1.2-补充说明.md)
- [1.3 编译](./1.3-编译.md)


---

## 章节自测

> 看代码 → 想答案 → 点开验证。

### Q1: 编译四阶段

```bash
# 以下四条命令分别对应编译的哪个阶段？
gcc -E hello.c -o hello.i    # (1)
gcc -S hello.i -o hello.s    # (2)
gcc -c hello.s -o hello.o     # (3)
gcc hello.o -o hello          # (4)
```

<details>
<summary>答案与复习指引</summary>

**答案：**
1. `-E` 预处理（展开 `#include` / `#define`）
2. `-S` 编译（C → 汇编）
3. `-c` 汇编（汇编 → 目标文件 `.o`）
4. 链接（`.o` + 库 → 可执行文件）

**复习：** → [1.3 编译](./1.3-编译.md)

</details>

### Q2: scanf 与返回值

```c
int val;
int ret = scanf("%d", &val);
// 输入 "abc" 时 ret 是多少？输入 "42" 时呢？
```

<details>
<summary>答案与复习指引</summary>

**答案：** 输入 `abc` → `ret = 0`（匹配失败）；输入 `42` → `ret = 1`（匹配 1 项）。

**解析：** `scanf` 返回成功匹配的项数。生产代码必须检查返回值。`&val` 是取地址——C 只有传值，传指针是模拟传引用。

**复习：** → [1.1 Introduction](./1.1-introduction/1.1-introduction.md) — scanf 与 `&`

</details>

### Q3: 栈上大数组

```c
int main(void) {
    int big[1000000];  // 4MB on stack
    big[0] = 42;
    printf("%d\n", big[0]);
    return 0;
}
// 会发生什么？
```

<details>
<summary>答案与复习指引</summary>

**答案：** 段错误（stack overflow）。默认栈大小约 8MB（Linux），4MB 数组接近极限，加上其他栈帧可能溢出。

**教训：** 大数组用 `malloc` 放堆上，或用 `static` / 全局变量。

**复习：** → [1.2 补充说明](./1.2-补充说明.md)

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
