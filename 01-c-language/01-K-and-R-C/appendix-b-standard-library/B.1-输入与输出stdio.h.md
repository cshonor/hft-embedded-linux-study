# B.1 输入与输出 `<stdio.h>`

## 本节讲什么

**standard input output header** —— C 标准库 **标准输入输出头文件**。提供 **控制台、文件读写** 相关的类型、宏、函数声明；没有 `#include <stdio.h>`，编译器不认识 `printf`、`FILE`、`stdin` 等，**直接报错**。

ch01 从 [1.1 Hello world](../ch01-introduction/1.1-入门.md) 起即用；[1.5 `getchar`](../ch01-introduction/1.5-character-io/1.5-字符输入输出.md) 流式 I/O；[ch07](../ch07-input-and-output/) 系统展开文件与缓冲。

## 一、作用概览

| 类别 | 提供什么 |
|------|----------|
| **函数声明** | `printf`、`fopen`、`fread`… |
| **类型** | `FILE`、`size_t`（常经 stdio 间接） |
| **宏** | `stdin`、`stdout`、`stderr`、`EOF`、`BUFSIZ`… |
| **实现** | 在 **C 标准库**（如 glibc）中实现，链接时 `-lc` |

**性质**：`<stdio.h>` 是 **标准库头文件**，**不是** C 语言语法关键字；C89 / C99 / C11 均自带，标准 C 环境通用。

## 二、最常用功能分类

### （1）控制台输入输出（终端）

| 函数 | 作用 |
|------|------|
| **`printf`** | 格式化输出到 **stdout** |
| **`scanf`** | 格式化从 **stdin** 读（用法需谨慎，见易错点） |
| **`puts`** | 输出字符串 + 换行 |
| **`putchar`** | 输出单个字符 |
| **`getchar`** | 从 **stdin** 读单个字符（返回 `int`，含 `EOF`） |
| **`gets`** | **已废弃，C11 移除** — 永不使用 |

```c
#include <stdio.h>

int main(void)
{
    printf("测试打印\n");
    putchar('A');
    return 0;
}
```

### （2）文件读写

| 函数 | 作用 |
|------|------|
| **`fopen` / `fclose`** | 打开 / 关闭文件，返回 `FILE *` |
| **`fread` / `fwrite`** | 二进制块读写 |
| **`fprintf` / `fscanf`** | 针对 `FILE *` 的格式化读写 |
| **`fgets`** | 读一行到缓冲（带长度限制，**安全**） |
| **`fgetc` / `fputc`** | 文件单字符读写 |

详见 [ch07 输入与输出](../ch07-input-and-output/)。

### （3）标准流宏 — 三个默认打开的数据流

| 宏 | 含义 | 典型绑定 |
|----|------|----------|
| **`stdin`** | 标准输入 | 键盘、管道、重定向 `< file` |
| **`stdout`** | 标准输出 | 屏幕、`printf` 默认目标、重定向 `> file` |
| **`stderr`** | 标准错误 | 报错输出；**无缓冲** 倾向，与 stdout 分离 |

```bash
./prog < in.txt > out.txt 2> err.log
```

[1.5](../ch01-introduction/1.5-character-io/1.5-字符输入输出.md) 的 `getchar`/`putchar` 即操作 **stdin/stdout**。

### （4）辅助宏与类型

| 名称 | 说明 |
|------|------|
| **`FILE`** | 文件流对象类型；`fopen` 返回 `FILE *`，所有 stdio 文件 API 依赖它 |
| **`EOF`** | End Of File，通常 **-1**；`getchar` 等返回 `EOF` 表示结束 |
| **`BUFSIZ`** | stdio 缓冲建议大小（实现定义） |
| **`NULL`** | 常经 `<stdio.h>` 间接获得（亦在 `<stddef.h>`） |

## 三、使用规则

```c
#include <stdio.h>   /* 必须：在调用 printf 等之前 */
```

- 放在翻译单元 **顶部**（`#include` 区），或任何使用声明 **之前**
- 仅 **声明** 函数与类型；**实现** 在 libc，链接阶段解析
- 与 [4.11 `#include`](../ch04-functions-and-program-structure/4.11-c-preprocessor/4.11.1-文件包含.md) 预处理机制：尖括号在 **系统头文件路径** 搜索

## 四、与标识符 / 关键字（2.1）的关联

| 名称 | 能否整词当变量名 | 嵌入标识符 |
|------|----------------|------------|
| `printf` | **不推荐**（库函数名） | `my_printf_hook` 语法允许 |
| `FILE` | **不推荐** | `file_buf` 常见 |
| `EOF` | **不推荐**（宏） | `eof_flag` 允许 |
| `stdin` 等 | **禁止宏劫持**（见 [1.4](../ch01-introduction/1.4-符号常量.md)） | — |

`FILE`、`EOF` 是 **类型名 / 宏名**，不是关键字；**不要** `#define printf my_printf` 或 `#define FILE int`（[C 陷阱 6.5](../../03-C-Traps-and-Pitfalls/ch06-preprocessor/6.5-宏名冲突.md)）。

## 五、stdio vs 系统调用（HFT / 内核视角）

| 层次 | API | 特点 |
|------|-----|------|
| **stdio** | `printf`、`fread`、`FILE *` | **带缓冲**；易用；热路径延迟难控 |
| **POSIX** | `read(2)`、`write(2)` | 无 stdio 缓冲；ch08 |
| **DPDK / 内核** | `rte_*`、直接 mmap/ring | 往往 **不用** stdio |

学习路径：ch01 **stdio** 入门 → ch07 文件与缓冲 → ch08 **read/write** 绕过 stdio。

## 六、高频易错点

1. **忘记 `#include <stdio.h>`** → `implicit declaration of function 'printf'`
2. **`getchar` 返回值赋给 `char`** → `EOF` 判错（[1.5](../ch01-introduction/1.5-character-io/1.5-字符输入输出.md)）
3. **使用 `gets`** → 溢出；用 **`fgets`**
4. **`scanf("%s", buf)` 无宽度** → 溢出；用 **`scanf("%99s", buf)`** 或 `fgets`
5. **`printf` 格式与实参类型不匹配** → UB（`%d` 对 `long long` 等）
6. 忘记 **`fclose`** → 资源泄漏

## 与后续衔接

- **ch01 1.1–1.10**：`printf`、`getchar`、最长行 — **`printf` 详解**：[1.1.2](../ch01-introduction/1.1.2-printf格式与安全.md)
- **ch07**：`fopen`、`setvbuf`、`sprintf` 族
- **ch08**：`read`/`write` 与 stdio 对比
- **附录 B**：`stdlib.h`、`string.h` 等并列标准库

## 面试题

1. `<stdio.h>` 提供什么？不写 `#include` 会怎样？
2. `stdin`、`stdout`、`stderr` 分别是什么？重定向如何影响它们？
3. `getchar` 与 `fgets` 有何区别？为何不用 `gets`？
4. stdio 缓冲与 `read(2)` 在低延迟场景如何选型？
5. `FILE` 是关键字吗？能否 `int FILE;`？
