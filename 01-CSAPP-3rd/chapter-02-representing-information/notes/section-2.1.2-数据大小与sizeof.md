## 2.1.2 数据大小

> **Ch2 §2.1 一条线：** [§2.1.1 十六进制](./section-2.1.1-十六进制表示法.md) → **§2.1.2 本节** → [§2.1.3 寻址与字节序](./section-2.1.3-寻址与字节序.md)

---

### `sizeof(T)` 是什么？

- **编译期常量** — 编译器根据 **目标架构 + ABI** 在编译阶段填好，**不是**运行时 `malloc` 量出来的
- **结果类型是无符号的 `size_t`** — 别拿有符号负数和 `sizeof(...)` 比大小（→ [§2.2 有/无符号坑](./section-2.2-整数表示与类型转换.md)）
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

**同一段 C 的直观例子：**

```c
sizeof(long);   /* ILP32（-m32）→ 4；Linux x86-64 LP64 → 8 */
sizeof(void*);  /* ILP32 → 4；LP64 → 8 — 与上表一致 */
```

换 ABI 再编译，**数字自己变** — 正是表格里的差异。

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

### `sizeof`：跨 ABI 该用，但别用错场景

| 问题 | 答 |
|------|----|
| 跨 ABI 项目最好 **不用** `sizeof`？ | **刚好相反** — 本机分配/步长/对齐 **更该用** `sizeof`，**别硬编码** `4`/`8` |
| 那 `if (sizeof(long) == 8)` 呢？ | **用错了** — 把 `sizeof` 当成「查文档写死数字」的分支条件 |

**`sizeof` 真正价值：** 在 **当前 ABI** 下，让编译器给出正确类型大小。

```c
/* ✓ 分配 / 跨度：切 LP64 / LLP64 / ILP32 都对 */
long *a = malloc(10 * sizeof(long));

/* ✓ 结构体偏移、数组步长同理 */
size_t stride = sizeof(MyLocalStruct);

/* ✗ 拿结果硬判 4/8 做业务分支 — 等于又写死了「平台表」 */
if (sizeof(long) == 8) { /* … */ }

/* wire / 文件 / 网络：不用 long，用固定宽度 */
int32_t tick;   /* 不用 sizeof(long) 当协议字段宽度 */
```

- **本机布局：** `malloc(n * sizeof(T))`、`offsetof`、栈帧直觉 — 靠 `sizeof`  
- **跨机协议：** `int32_t`/`int64_t` + 显式 endian — **不用** `long`，也 **不要** 用 `sizeof(long)` 当 wire 宽度  

老架构上 `int` 还可能是 **2 字节** — 硬编码 `4` 更危险；`sizeof` 跟当前目标走。

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
2. 跨 ABI 项目该不该用 `sizeof`？`if (sizeof(long)==8)` 为何是误用？  
3. 为什么协议字段用 `int32_t` 而不是 `int`/`long`？  
4. API 和 ABI 分别管什么？→ [ABI 笔记](./section-2.1.2-abi-application-binary-interface.md)

---

→ 下一节：[§2.1.3 寻址与字节序](./section-2.1.3-寻址与字节序.md) · ← [§2.1.1](./section-2.1.1-十六进制表示法.md) · [Ch2 导读](../README.md)
