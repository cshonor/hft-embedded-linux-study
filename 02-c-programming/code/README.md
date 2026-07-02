# 02 · C 语言 · 动手代码

**分工（别混）：**

| 目录 | 放什么 |
|------|--------|
| **[01-CSAPP-3rd/code/](../../01-CSAPP-3rd/code/)** | CSAPP **章节实验** — Ch2 字节序/sizeof/padding 等 |
| **本目录 `02-c-programming/code/`** | **C 语言专练** — K&R / *Pointers on C* 配套小练习 |
| **[外部 cpp-learning-notes / 11-C](https://github.com/cshonor/cpp-learning-notes/tree/main/11-Linux-Kernel-DPDK-Network-C)** | **笔记正文** 写在外部仓 |

读序仍是 `01 CSAPP` → **`02` C** → `03 Hennessy`；Ch2 概念在 **01 笔记 + 01/code** 学，**02/code** 练指针写法。

---

## 本目录实验

| 文件 | 何时做 | 对接 |
|------|--------|------|
| [pointer-and-bytes.c](./pointer-and-bytes.c) | Ch2 §2.1.3 endian | `unsigned char *` 逐字节 |
| [pointer-stride-demo.c](./pointer-stride-demo.c) | [指针步长笔记](../notes/pointer-arithmetic-and-stride.md) | `char*` vs `int*` 的 `p+1` |
| *(待补)* | 外部 `01` K&R Ch5 | 指针与函数 |
| *(待补)* | 外部 `02` Pointers on C | 动态内存 |

---

## CSAPP Ch2 实验（在 01，不在本目录）

→ [01-CSAPP-3rd/code/ch02-endian-and-padding-demo.c](../../01-CSAPP-3rd/code/ch02-endian-and-padding-demo.c)

```bash
cd ../../01-CSAPP-3rd/code
gcc -Wall -Wextra -std=c11 -o ch02_demo ch02-endian-and-padding-demo.c
./ch02_demo
```

**预期（x86 小端）：** `0x11223344` → `44 33 22 11`；struct padding `sizeof == 12`。

---

## 编译本目录

```bash
gcc -Wall -Wextra -std=c11 -o pointer_bytes pointer-and-bytes.c
./pointer_bytes
```

← [02 导读](../README.md) · [CSAPP Ch2 §2.1.3](../../01-CSAPP-3rd/chapter-02-representing-information/notes/section-2.1.1-2.1.3-十六进制寻址与字节序.md)
