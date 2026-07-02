## 2.1.2 数据大小

> **Ch2 §2.1 一条线：** [§2.1.1 十六进制](./section-2.1.1-十六进制表示法.md) → **§2.1.2 本节** → [§2.1.3 寻址与字节序](./section-2.1.3-寻址与字节序.md)

---

### `sizeof(T)` 是什么？

- **编译期常量** — 编译器根据 **目标架构 + ABI** 在编译阶段填好，**不是**运行时 `malloc` 量出来的
- 同一份 `.c`，换 `-m32`/`-m64`、换 OS、换交叉编译目标，**sizeof 可以变**
- CSAPP 提醒：**可移植代码不要假设 `int` 一定是 4 字节**（老架构/部分嵌入式上 `int` 可能是 2 字节）

### 典型 ABI 对照（64 位时代最容易混）

| 类型 | ILP32（32 位） | LP64（Linux/macOS x86-64） | LLP64（Windows x64） |
|------|----------------|----------------------------|----------------------|
| `int` | 4 | 4 | 4 |
| `long` | 4 | **8** | **4** |
| 指针 | 4 | 8 | 8 |
| `size_t` | 4 | 8 | 8 |

**要点：** 同样是 64 位 CPU，`sizeof(long)` 在 Linux 上常为 **8**，在 Windows x64 上常为 **4** — 差在 **ABI（LP64 vs LLP64）**，不是 CPU 位数 alone。

→ ABI 完整笔记：[§2.1.2 ABI 延伸阅读](./section-2.1.2-abi-application-binary-interface.md)

### LP64 Linux x86-64 常见宽度

| C 类型 | 字节 |
|--------|------|
| `char` | 1 |
| `short` | 2 |
| `int` | 4 |
| `long` | 8 |
| `float` | 4 |
| `double` | 8 |
| 指针 | 8 |

- **字长 (word size)** — 机器「自然」一次处理的数据宽度；64 位机里指针/`long`（LP64）常为 8 字节

### 本机一眼确认

```c
#include <stdio.h>

int main(void) {
    printf("sizeof(char)=%zu int=%zu long=%zu void*=%zu\n",
           sizeof(char), sizeof(int), sizeof(long), sizeof(void*));
    return 0;
}
```

→ 也可跑 [ch02-endian-and-padding-demo.c](../../code/ch02-endian-and-padding-demo.c) 的 `demo_sizeof()`

### 跨平台怎么写

```c
#include <stdint.h>

int32_t  price_tick;   /* 协议字段：宽度写死 */
int64_t  qty;
uint64_t seq;
/* 避免：wire 格式用 int/long，换平台长度就变 */
```

### HFT / 跨平台踩坑

| 坑 | 后果 |
|----|------|
| wire 协议用 `long` / 本地 `struct` 直接 `send()` | LLP64 vs LP64 长度不一致 |
| 假设 `int` 永远 4 字节 | 嵌入式 ABI 上布局全错 |
| Rust/C++ 共享内存未约定 layout | 宽度 + padding + endian 三重不一致 |

**HFT：** 二进制 schema 用 **SBE / 手写 offset + 固定宽度**；不能靠本地 `long`。

---

## 口述巩固 · 自测

1. `sizeof(long)` 在 Linux x64 和 Windows x64 各是多少？为什么同为 64 位却不同？
2. 为什么协议字段用 `int32_t` 而不是 `int`？
3. API 和 ABI 分别管什么？→ [ABI 笔记](./section-2.1.2-abi-application-binary-interface.md)

---

→ 下一节：[§2.1.3 寻址与字节序](./section-2.1.3-寻址与字节序.md) · ← [§2.1.1](./section-2.1.1-十六进制表示法.md) · [Ch2 导读](../README.md)
