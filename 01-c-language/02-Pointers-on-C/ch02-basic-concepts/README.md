# 第 2 章 基本概念

**Basic C Concepts**

## 本章讲什么

全书**语法规则地基**：Token 词法、注释与空白、**声明/定义**、**作用域**、**存储类**、程序**翻译与执行**模型。承接 ch01 骨架，为指针、结构体、内存布局建立标准化认知。

## 学习重点

- **五类 Token** + **最长匹配**（`a+++b`）
- **声明 vs 定义** → 链接 `multiple definition` / `undefined reference`
- **四类作用域** + **遮蔽**
- **static** 两义（局部生命周期 / 文件内链接）、**extern**
- ELF **.text / .data / .bss / stack**；bss 清零 vs 栈脏数据
- **if 分号**、工程命名与 **2.3 风格**

## 场景价值（内核 / DPDK / HFT）

| 价值 | 说明 |
|------|------|
| 链接理论 | extern/static 解 90% 多文件报错 |
| 内存生命周期 | static 池、单次硬件 init |
| 符号隔离 | static 私有函数 |
| 防御 bug | 遮蔽、词法、分号陷阱 |

## 实操（建议完成）

1. 多文件 static vs 全局，观察链接器  
2. static 局部只初始化一次  
3. 变量遮蔽实验  
4. if 分号错误  
5. `readelf -S ./a.out`  

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | ch01-quick-start |
| 后序 | ch03 数据；ch04 语句；ch06 指针；ch18 运行时 |
| 配套 | 《C陷阱与缺陷》ch01 词法、ch02 语法、ch04 链接 |

## 小节

- [2.1 环境](2.1-the-environment/2.1-环境.md)
  - [2.1.1 翻译](./2.1-the-environment/2.1.1-翻译.md)
  - [2.1.2 执行](./2.1-the-environment/2.1.2-执行.md)
- 2.2 词法规则
  - [2.2.1 字符](./2.2-lexical-rules/2.2.1-字符.md)
  - [2.2.2 注释](./2.2-lexical-rules/2.2.2-注释.md)
  - [2.2.3 自由形式的源代码](./2.2-lexical-rules/2.2.3-自由形式的源代码.md)
  - [2.2.4 标识符](./2.2-lexical-rules/2.2.4-标识符.md)
  - [2.2.5 程序的形式](./2.2-lexical-rules/2.2.5-程序的形式.md)
- [2.3 程序风格](./2.3-程序风格.md)


---

## 章节自测

> 看代码 → 想答案 → 点开验证。

### Q1: 声明 vs 定义

```c
// file1.c
int counter;          // (1)

// file2.c
extern int counter;   // (2)
```

> `(1)` 和 `(2)` 哪个是定义？哪个是声明？如果 file1.c 也有 `int counter;` 会怎样？

<details>
<summary>答案与复习指引</summary>

**答案：** `(1)` 是定义（分配内存），`(2)` 是声明（告诉编译器变量在别处）。

两个 `.c` 都写 `int counter;` → 链接报错 `multiple definition of 'counter'`。

**复习：** → 2.1 Identifiers — 声明 vs 定义

</details>

### Q2: 最长匹配词法

```c
int a = 1, b = 2;
int c = a+++b;
printf("a=%d b=%d c=%d\n", a, b, c);
```

> 输出什么？`a+++b` 怎么解析？

<details>
<summary>答案与复习指引</summary>

**输出：** `a=2 b=2 c=3`

**解析：** C 词法分析器用**最长匹配**规则。`a+++b` 解析为 `(a++) + b` 而非 `a + (++b)`。先取 `a++`（后置自增，返回旧值 1），再加 `b` = 3。之后 `a` 变 2。

**复习：** → 2.2 Lexical Rules — 最长匹配

</details>

### Q3: static 双义

```c
// file_a.c
static int file_local = 42;      // (1) 文件内可见
void f(void) {
    static int call_count = 0;   // (2) 值保持
    call_count++;
}
```

> `(1)` 和 `(2)` 的 `static` 含义一样吗？

<details>
<summary>答案与复习指引</summary>

**答案：** 不一样。
- `(1)` 文件作用域 `static` → **限制链接性**（仅本文件可见，其他文件 `extern` 也找不到）
- `(2)` 块作用域 `static` → **延长生命周期**（值在函数调用间保持，但作用域仍在块内）

两者都存 `.data` 或 `.bss` 段，不是栈。

**复习：** → 2.1 Identifiers — static / extern

</details>

### Q4: bss 清零 vs 栈脏数据

```c
int global_val;        // (1) bss 段
int main(void) {
    int local_val;     // (2) 栈上
    printf("global=%d local=%d\n", global_val, local_val);
    return 0;
}
```

> `global_val` 和 `local_val` 的值分别是什么？

<details>
<summary>答案与复习指引</summary>

**答案：** `global_val = 0`（bss 段自动清零）。`local_val` 不确定（栈上未初始化的值是上次遗留的"脏数据"）。

**教训：** 局部变量必须初始化。全局变量虽然自动清零，但显式初始化更清晰。

**复习：** → 2.1 Identifiers — 存储类与段

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
