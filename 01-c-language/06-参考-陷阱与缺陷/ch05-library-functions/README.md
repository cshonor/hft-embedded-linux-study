# 第 5 章 库函数

**Library Functions** — Andrew Koenig, *C Traps and Pitfalls*

## 本章目标

[ch04 连接](../ch04-linking/) 解决符号与链接后，本章聚焦 **C 标准库** 调用层陷阱：格式串、字符串、堆、ctype、I/O、数学库 —— 多为 **编译无告警、运行内存破坏或数据错乱**。

## 小节索引

| 节 | 主题 |
|----|------|
| [5.1](./5.1-printf与可变参数.md) | `printf` 格式符、漏参、`%s` NULL |
| [5.2](./5.2-字符串函数.md) | `strcpy` / `strncpy` / `strlen` / `strcmp` |
| [5.3](./5.3-malloc与free.md) | `malloc(0)`、判空、双重 free |
| [5.4](./5.4-ctype字符分类.md) | `unsigned char` 入参 |
| [5.5](./5.5-文件IO.md) | 禁用 `gets`、`fgets` 换行、`fopen` 判空 |
| [5.6](./5.6-数学库隐式转换.md) | `sqrt`、`-lm`、整型截断 |

## 底层工程强制规范

1. 格式符与实参类型 **严格对应**；64 位用 `%lld` / `%zu`
2. 禁止裸 `strcpy`/`strcat`；带长度 + 手动 `\0`
3. `malloc` 判空；同一指针 **只 free 一次**；不 free 栈地址
4. `ctype` 入参：`(unsigned char)ch`
5. 禁用 `gets`；`fgets` + 去 `\n`
6. 自写可变参函数须校验参数个数

## 前后章节

| | 章节 |
|---|------|
| **前置** | [ch04 连接](../ch04-linking/) |
| **后置** | [ch06 预处理器](../ch06-preprocessor/) |
| **交叉** | [附录 A stdarg](../appendix-a-printf-varargs-stdarg/) |

## Demo

```bash
cd demo && make all
./demo01_printf/main
./demo02_strncpy/main
./demo03_strcpy/main
./demo04_malloc/main
./demo05_ctype/main
./demo06_fgets/main
./demo07_math/main
```

## 面试题

1. `printf("%f", 10)` 为何能编译通过却输出乱码？
2. `strncpy` 与 `strcpy` 各有什么坑？
3. `free(NULL)` 是否安全？双重 free 后果？
4. 为何 `isprint((char)0xFF)` 可能 UB？
5. `gets` 为何被标准移除？`fgets` 读一行后要注意什么？

## 章节自测

> 库函数陷阱：printf、字符串、堆、ctype、IO。看代码 → 想答案 → 点开验证。

### Q1: printf 格式符不匹配

```c
long big = 1000000000L;
printf("big = %d\n", big);       // A
printf("big = %ld\n", big);     // B

size_t sz = sizeof(int);
printf("sz = %d\n", sz);         // C
printf("sz = %zu\n", sz);       // D
```

> A 和 C 在 64 位系统上可能出什么问题？

<details>
<summary>答案与复习指引</summary>

**答案：**
- A：`%d` 读 `int`（4 字节），但 `long` 在 LP64 上是 8 字节——读了一半，或者后续参数错位。**UB**，可能输出错误值。
- B：`%ld` 正确匹配 `long`。
- C：`size_t` 在 64 位上是 `unsigned long`（8 字节），`%d` 读 4 字节——**UB**。
- D：`%zu` 正确匹配 `size_t`。

**规则：** 格式符必须与类型严格对应。64 位系统用 `%ld`/`%lld`/`%zu`。

**复习：** → [5.1 printf 与可变参数](./5.1-printf与可变参数.md)

</details>

### Q2: strncpy 的坑

```c
char buf[5];
strncpy(buf, "hello world", 5);
buf[5] = '\0';
printf("%s\n", buf);
```

> 这段代码有什么问题？

<details>
<summary>答案与复习指引</summary>

**答案：** 两个问题：
1. **越界写：** `buf[5]` 越界（数组只有 0-4）。`strncpy` 在源串 ≥ n 时不追加 `\0`，需要手动加——但这里位置错了。
2. **无终止符：** `strncpy(buf, "hello world", 5)` 只拷贝 5 字节 `hello`，**不追加 `\0`**。正确做法是 `buf[4] = '\0'` 或 `buf[sizeof(buf)-1] = '\0'`。

**规则：** `strncpy` 后手动在最后一个位置加 `\0`——`buf[n-1] = '\0'`。

**复习：** → [5.2 字符串函数](./5.2-字符串函数.md)

</details>

### Q3: malloc 与 free 陷阱

```c
char *p = malloc(0);
if (p == NULL)
    printf("NULL\n");
else {
    printf("non-NULL\n");
    free(p);
    free(p);   // 双重 free
}
```

> `malloc(0)` 返回什么？双重 free 会怎样？

<details>
<summary>答案与复习指引</summary>

**答案：**
- `malloc(0)`：**实现定义**——可能返回 NULL，也可能返回一个非空但不可解引用的指针。两种都合法。
- 双重 free：**UB**——堆元数据已被破坏，可能导致堆崩溃或安全漏洞。`free(NULL)` 是安全的（什么也不做），但 `free(非空指针)` 两次是 UB。

**规则：** free 后置 `p = NULL`；同一指针只 free 一次。

**复习：** → [5.3 malloc 与 free](./5.3-malloc与free.md)

</details>

### Q4: ctype 入参陷阱

```c
char c = 0xFF;   // char signed 时 c = -1
if (isprint(c))
    printf("printable\n");
else
    printf("not printable\n");
```

> 在 `char` 为有符号的平台上，这段代码可能出什么问题？

<details>
<summary>答案与复习指引</summary>

**答案：** **UB**。`isprint` 的参数要求是 `EOF`(-1) 或 `unsigned char` 范围（0-255）的值。当 `char` 有符号时，`c = 0xFF` 变成 `-1`，与 `EOF` 冲突——实现可能用 `-1` 作为查找表索引，导致**越界访问**。

**修正：** `isprint((unsigned char)c)` — 强制转换为 `unsigned char` 再传给 ctype 函数。

**复习：** → [5.4 ctype 字符分类](./5.4-ctype字符分类.md)

</details>

### Q5: fgets 换行处理

```c
char buf[16];
FILE *f = fopen("data.txt", "r");
if (f) {
    fgets(buf, sizeof(buf), f);
    // 文件第一行是 "hello\n"
    printf("[%s]\n", buf);
}
```

> 输出中是否包含换行符？如何去掉？

<details>
<summary>答案与复习指引</summary>

**答案：** 输出 `[hello\n]`（`fgets` **保留换行符**）。与 `gets`（已移除）不同，`fgets` 读到 `\n` 时把它存入缓冲区。

**去掉换行：**
```c
buf[strcspn(buf, "\n")] = '\0';
```

**规则：** `fgets` 后必须处理换行符——`strcspn` 比 `strlen-1` 更安全（不假设最后一定是 `\n`）。

**复习：** → [5.5 文件 IO](./5.5-文件IO.md)

</details>
