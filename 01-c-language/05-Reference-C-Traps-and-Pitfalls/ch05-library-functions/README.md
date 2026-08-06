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
