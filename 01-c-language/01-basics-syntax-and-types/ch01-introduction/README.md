# 第 1 章 导言

**A Tutorial Introduction（教程式导言）**

## 本章讲什么

快速上手能运行的 C 小程序，只搭最基础语法骨架：**不讲指针、结构体、复杂运算符**（留到第 2、5、6 章）。全部是短小可运行的文本处理、数值计算示例。

## 学习重点

- C 程序 = **函数 + 变量**，`main` 是唯一入口
- 函数参数**只有传值调用**（本章不涉及传地址）
- 字符串 = 以 `\0` 结尾的 **char 数组**
- `#define` 做符号常量，预处理在编译前展开
- 与 MikanOS / Linux 系统编程的衔接：本章建立代码思维，内存与指针底层在第 2、5 章展开

## 小节

- [1.1 入门](./1.1-入门.md)
  - [1.1.1 命令行参数 `argc/argv`](./1.1.1-命令行参数-main-argc-argv.md)
  - [1.1.2 `printf` 格式与安全](./1.1.2-printf格式与安全.md)
- [1.2 变量与算术表达式](./1.2-变量与算术表达式.md)
- [1.3 for语句](./1.3-for语句.md)
- [1.4 符号常量](./1.4-符号常量.md)
- [1.5 字符输入输出](./1.5-character-io/1.5-字符输入输出.md)
  - [1.5.1 文件复制](./1.5-character-io/1.5.1-文件复制.md)
  - [1.5.2 字符计数](./1.5-character-io/1.5.2-字符计数.md)
  - [1.5.3 行计数](./1.5-character-io/1.5.3-行计数.md)
  - [1.5.4 单词计数](./1.5-character-io/1.5.4-单词计数.md)
- [1.6 数组](./1.6-数组.md)
- [1.7 函数](./1.7-函数.md)
- [1.8 参数传值调用](./1.8-参数传值调用.md)
  - [1.8.1 指针形参、`&` 取地址、`*` 解引用](./1.8.1-指针形参与取地址.md)
- [1.9 字符数组](./1.9-字符数组.md)
- [1.10 外部变量与作用域](./1.10-外部变量与作用域.md)

---

## 章节自测

> 每题对应一个小节。看代码 → 想答案 → 点开验证。看不懂就按「复习」去翻对应笔记。

### Q1: 程序入口与基本输出

```c
#include <stdio.h>

int main() {
    printf("Hello");
    printf("World\n");
    return 0;
}
```

> 这段程序输出什么？`return 0` 是什么意思？

<details>
<summary>答案与复习指引</summary>

**输出：** `HelloWorld`（两行 printf 紧挨，中间无空格，末尾换行）

**解析：** `printf` 不自动加空格或换行，需手动写 `\n`。`main` 是程序唯一入口，`return 0` 表示正常退出，操作系统据此判断程序成功。

**复习：** → [1.1 入门](./1.1-入门.md)

</details>

### Q2: 变量与算术表达式

```c
int fahr;
for (fahr = 0; fahr <= 300; fahr = fahr + 20)
    printf("%3d %6.1f\n", fahr, (5.0/9.0)*(fahr-32));
```

> 这段代码做什么？为什么用 `5.0/9.0` 而不是 `5/9`？

<details>
<summary>答案与复习指引</summary>

**答案：** 华氏度转摄氏度表，每 20 度一行。`%3d` 右对齐 3 位整数，`%6.1f` 右对齐 6 位 1 位小数。

**解析：** `5/9` 是整数除法，结果为 0（截断小数）。`5.0/9.0` 强制浮点除法，得到 0.555...。整数与浮点混算时，整数会被提升为浮点。

**复习：** → [1.2 变量与算术表达式](./1.2-变量与算术表达式.md) · [1.3 for语句](./1.3-for语句.md)

</details>

### Q3: 符号常量

```c
#define LOWER 0
#define UPPER 300
#define STEP  20

int fahr;
for (fahr = LOWER; fahr <= UPPER; fahr += STEP)
    printf("%3d %6.1f\n", fahr, (5.0/9.0)*(fahr-32));
```

> 用 `#define` 比直接写数字 0、300、20 有什么好处？

<details>
<summary>答案与复习指引</summary>

**答案：** 预处理阶段将 `LOWER` 替换为 `0`、`UPPER` 替换为 `300`、`STEP` 替换为 `20`。好处：① 一改全改（修改上限只需改一处）；② 比魔数 `300` 可读性强。

**注意：** `#define` 是文本替换，不做类型检查；C99 后也可用 `const int UPPER = 300;`。

**复习：** → [1.4 符号常量](./1.4-符号常量.md)

</details>

### Q4: 字符数组与字符串

```c
char s[] = "abc";
printf("%d\n", sizeof(s));
printf("%s\n", s);
```

> 输出什么？`"abc"` 在内存中占几个字节？

<details>
<summary>答案与复习指引</summary>

**输出：**
```
4
abc
```

**解析：** 字符串字面量 `"abc"` 末尾隐含 `\0`，所以 `sizeof(s)` = 4（a, b, c, \0）。`printf` 用 `%s` 打印时遇到 `\0` 停止。

**复习：** → [1.9 字符数组](./1.9-字符数组.md)

</details>

### Q5: 传值调用

```c
void swap(int a, int b) {
    int temp = a;
    a = b;
    b = temp;
}

int main() {
    int x = 3, y = 5;
    swap(x, y);
    printf("%d %d\n", x, y);
    return 0;
}
```

> 输出什么？为什么 swap 没生效？

<details>
<summary>答案与复习指引</summary>

**输出：** `3 5`（没交换）

**解析：** C 函数参数是**传值**——形参 `a`、`b` 是实参 `x`、`y` 的副本，修改形参不影响实参。要真正交换需传指针（见 1.8.1）。

**复习：** → [1.8 参数传值调用](./1.8-参数传值调用.md) · [1.8.1 指针形参与取地址](./1.8.1-指针形参与取地址.md)

</details>

### Q6: 外部变量与作用域

```c
int count = 0;

void increment(void) {
    count++;
}

int main() {
    increment();
    increment();
    printf("%d\n", count);
    return 0;
}
```

> 输出什么？`count` 定义在函数外面叫什么变量？

<details>
<summary>答案与复习指引</summary>

**输出：** `2`

**解析：** `count` 是外部变量（全局变量），定义在所有函数之外，全程有效且保持值。`increment` 直接修改它。注意：全局变量增加了耦合性，能不用就不用。

**复习：** → [1.10 外部变量与作用域](./1.10-外部变量与作用域.md)

</details>

---

## 代码自测

**题目 1：** 以下 K&R 风格的程序有什么 C89 不兼容的问题？
```c
main()
{
    printf("hello, world\n");
}
```

<details>
<summary>参考答案</summary>

main() 没有返回类型声明——C89 隐式返回 int 可以编译，C99 起是错误。也没有 return 语句——C89 中 main 的返回值不确定，C99 起隐式 return 0。K&R 第二版（1989 ANSI C）已改为 int main(void) 形式。现代 C 应写：int main(void) { printf("hello, world\n"); return 0; }。

</details>
