# 第 7 章 输入与输出

**Input and Output**

## 本章讲什么

面向**终端、文件、字符流**的标准 I/O：`FILE*` 流对象、字符读写、格式化 `printf`/`scanf`、二进制 `fread`/`fwrite`、定位与错误处理。程序与外部数据交互的基础；承接第 6 章 struct，为第 8 章 UNIX 系统接口铺垫。

## 排版说明

K&R 原书的小节顺序是：7.1 getchar/putchar → 7.2 printf → 7.3 变长参数 → 7.4 scanf → 7.5 FILE* → 7.6 stderr → 7.7 fgets → 7.8 其它函数。

本笔记重新编排为**围绕 `FILE*` 对象**的逻辑顺序：先讲 stdio 核心对象 `FILE`（7.1），再按 I/O 粒度递增展开（字符→行→格式化→二进制），错误处理放最后，变长参数作为附录。原 K&R 节号在每节开头标注。

| 本笔记节号 | 标题 | 原 K&R 节号 |
|-----------|------|------------|
| 7.1 | 流模型与 FILE 对象 | 7.5（前半） |
| 7.2 | 字符 I/O | 7.1 |
| 7.3 | 行 I/O | 7.7 |
| 7.4 | 格式化输出 printf | 7.2 |
| 7.5 | 格式化输入 scanf | 7.4 |
| 7.6 | 二进制 I/O 与文件定位 | 7.5（后半） |
| 7.7 | 错误处理 | 7.6 |
| 7.8 | 其它函数 | 7.8 |
| 7.9 | 变长参数表（附录） | 7.3 |

## 学习重点

- **`FILE*` 流对象**：封装 fd + 缓冲 + 位置 + 标志，所有 stdio 函数围绕它
- **字符流统一模型**；**`EOF`** vs 错误（`feof`/`ferror`）
- **文本 vs 二进制**（`b` 模式、换行转换坑）
- **`snprintf`/`fgets`** 替代不安全 `sprintf`/`gets`
- **`fread`/`fwrite` + struct** 与 **padding/endian** 不可移植
- **`stderr`** 与 **`fseek`** 随机访问

## 场景映射

| 方向 | 本章技能 |
|------|----------|
| OS / UEFI | 镜像读写、GOP 类 printf、stdout/stderr 调试 |
| HFT | tick 落盘/回放、snprintf 日志、sscanf 轻量解析 |
| 嵌入式 Linux | 配置/传感器文件、串口字符流 |

## 难点

文本/二进制模式、缓冲区溢出、`fseek` 大文件、struct 整块读写跨平台。

## 小节

- [7.1 流模型与 FILE 对象](./7.1-流模型与FILE对象.md)
- [7.2 字符 I/O](./7.2-字符IO.md)
- [7.3 行 I/O](./7.3-行IO.md)
- [7.4 格式化输出 printf](./7.4-格式化输出printf.md)
- [7.5 格式化输入 scanf](./7.5-格式化输入scanf.md)
- [7.6 二进制 I/O 与文件定位](./7.6-二进制IO与文件定位.md)
- [7.7 错误处理](./7.7-错误处理.md)
- [7.8 其它函数](./7.8-other-functions/7.8-其它函数.md)
  - [7.8.1 字符串操作函数](./7.8-other-functions/7.8.1-字符串操作函数.md)
  - [7.8.2 字符类别测试和转换函数](./7.8-other-functions/7.8.2-字符类别测试和转换函数.md)
  - [7.8.3 ungetc 函数](./7.8-other-functions/7.8.3-ungetc函数.md)
  - [7.8.4 命令执行函数](./7.8-other-functions/7.8.4-命令执行函数.md)
  - [7.8.5 存储管理函数](./7.8-other-functions/7.8.5-存储管理函数.md)
  - [7.8.6 数学函数](./7.8-other-functions/7.8.6-数学函数.md)
  - [7.8.7 随机数发生器函数](./7.8-other-functions/7.8.7-随机数发生器函数.md)
- [7.9 变长参数表（附录）](./7.9-变长参数表.md)

---

## 章节自测

> I/O 是程序与外部交互的桥梁。看代码 → 想答案 → 点开验证。

### Q1: printf 格式控制

```c
int x = 42;
double pi = 3.14159;

printf("[%5d]\n", x);     // (1)
printf("[%-5d]\n", x);    // (2)
printf("[%05d]\n", x);    // (3)
printf("[%.2f]\n", pi);   // (4)
printf("[%8.3f]\n", pi);  // (5)
```

> 五行各输出什么？

<details>
<summary>答案与复习指引</summary>

**输出：**
```
[   42]
[42   ]
[00042]
[3.14]
[   3.142]
```

**解析：**
- `%5d` — 宽度 5，右对齐，空格填充
- `%-5d` — 左对齐
- `%05d` — 零填充
- `%.2f` — 小数点后 2 位
- `%8.3f` — 总宽 8，小数 3 位

**注意：** 用 `snprintf` 替代 `sprintf` 防缓冲区溢出。

**复习：** → [7.4 格式化输出printf](./7.4-格式化输出printf.md)

</details>

### Q2: scanf 返回值

```c
int a, b;
int n = scanf("%d %d", &a, &b);
printf("n=%d a=%d b=%d\n", n, a, b);

// 输入：10 20
// 输入：10 abc
```

> 两种输入下 `n` 分别是多少？

<details>
<summary>答案与复习指引</summary>

**输入 `10 20`：** `n=2 a=10 b=20`（成功匹配 2 项）

**输入 `10 abc`：** `n=1 a=10 b=未初始化`（`%d` 匹配 `10` 成功，`abc` 不是数字匹配失败）

**解析：** `scanf` 返回**成功匹配的项数**，不是读入的字符数。生产代码必须检查返回值，不要假设全部成功。`b` 未被赋值，值不确定（UB）。

**教训：** 永远检查 `scanf` 返回值。更安全的做法是用 `fgets` + `sscanf`。

**复习：** → [7.5 格式化输入scanf](./7.5-格式化输入scanf.md)

</details>

### Q3: fgets vs gets（安全）

```c
char buf1[10];
char buf2[10];

// gets(buf1);      // K&R 时代函数，现已废弃
fgets(buf1, 10, stdin);  // 安全替代
```

> `gets` 为什么被废弃？`fgets` 怎么保证安全？

<details>
<summary>答案与复习指引</summary>

**答案：** `gets` 不接受长度参数，无法限制输入长度 → **缓冲区溢出**漏洞（经典的 Morris 蠕虫就利用了 `gets`）。C11 标准已彻底删除 `gets`。

`fgets(buf, 10, stdin)` 最多读 9 个字符（留 1 个给 `\0`），第 10 个字符及之后留在输入流。

**区别：** `fgets` 会在缓冲区中**保留换行符** `\n`（如果一行能放下），`gets` 会丢弃。处理时需手动去掉 `\n`。

**复习：** → [7.3 行IO](./7.3-行IO.md)

</details>

### Q4: 文本 vs 二进制模式

```c
struct Record { int id; char name[8]; };

struct Record r = {42, "test"};

FILE *ft = fopen("data.txt", "w");   // 文本
FILE *fb = fopen("data.bin", "wb");  // 二进制

fwrite(&r, sizeof(r), 1, ft);
fwrite(&r, sizeof(r), 1, fb);

fclose(ft);
fclose(fb);
```

> 两个文件大小一定相同吗？struct 直接 `fwrite` 有什么跨平台风险？

<details>
<summary>答案与复习指引</summary>

**答案：** 不一定相同。在 Linux 上文本和二进制模式几乎无差异；但在 Windows 上文本模式会对 `\n` 做 `\r\n` 转换，文件大小可能不同。

**跨平台风险：**
1. **padding**：`struct Record` 大小因对齐而异（可能 12 或 16 字节，取决于编译器）
2. **字节序**：`int id = 42` 在小端机存 `2A 00 00 00`，大端机存 `00 00 00 2A`
3. **类型宽度**：`int` 在不同平台可能 2/4/8 字节

**教训：** struct 整块 `fwrite`/`fread` **不可移植**。跨平台需用定宽类型 + 手动序列化。

**复习：** → [7.6 二进制IO与文件定位](./7.6-二进制IO与文件定位.md)

</details>

### Q5: stderr 不缓冲

```c
fprintf(stdout, "A");    // stdout 默认行缓冲
fprintf(stderr, "B");    // stderr 默认无缓冲
fprintf(stdout, "C\n");

// 如果输出重定向到文件：./a.out > log.txt
// 文件里是 ABC 还是 BAC？
```

> 重定向到文件时，`log.txt` 里的顺序是 `ABC\n` 还是 `BAC\n`？为什么？

<details>
<summary>答案与复习指引</summary>

**答案：** 重定向到文件时，`stdout` 变为**全缓冲**（非行缓冲），`stderr` 始终无缓冲。所以 `B` 先输出，`AC` 缓存在 stdout 缓冲区，遇到 `\n` 时 flush。输出顺序可能是 `BAC\n`。

但实际结果取决于**程序退出时是否 flush**：
- 如果正常 `return 0` → exit 刷所有缓冲 → 文件里可能看到 `BAC`
- 如果异常退出（如 `abort()`）→ stdout 可能没刷 → 只有 `B`

**教训：** 错误信息用 `fprintf(stderr, ...)`，确保立刻输出不被缓冲。调试时尤其重要。

**复习：** → [7.7 错误处理](./7.7-错误处理.md) · [7.1 流模型与FILE对象](./7.1-流模型与FILE对象.md)

</details>

---

## 代码自测

**题目 1：** 以下代码中 printf 的格式串和参数不匹配，会发生什么？
```c
printf("%d\n", 3.14);
```


<details>
<summary>参考答案</summary>

未定义行为。%d 期望 int 参数，但传入 double。printf 是可变参数函数，不做类型检查——它按 %d 的方式读取栈上的 4 字节（int），但 3.14 是 8 字节 double。结果可能输出垃圾值。K&R 时代没有格式串检查，现代 GCC/Clang 可以用 -Wformat 检查这类错误。

</details>
