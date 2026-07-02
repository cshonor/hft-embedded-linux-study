# 指针运算 · 地址 vs 类型步长

> **02 C 专练** · 配合 K&R / *Pointers on C* · 与 [CSAPP §3.8](../../01-CSAPP-3rd/chapter-03-machine-level-programs/notes/section-3.8-数组与指针运算.md) 对照  
> **动手：** [pointer-stride-demo.c](../code/pointer-stride-demo.c)

---

## 核心拆解：指针是地址，但指针运算自带「类型步长」

### 1. 先分清两层东西

| | 含义 |
|--|------|
| `p` **存的内容** | **内存地址**（纯数字；x86-64 下指针本身占 8 字节） |
| `p` **的类型** | 如 `char *` — 「从这个地址起，按 **char（1 字节）** 解释」 |

指针加减数字，**不是** 给地址数字裸 +1，而是：

\[
\text{新地址} = \text{原地址} + n \times \sizeof(\text{指针指向的类型})
\]

---

### 2. 拿 `char *p` 举例

- `sizeof(char) == 1`
- `p + 1` → 地址数值 **+ 1×1 = +1 字节** — 与「逐字节扫描内存」一致

**对比（同一数组基址，不同类型指针 `+1` 跳多远）：**

| 指针类型 | 元素大小 | `p + 1` 地址增量 |
|----------|----------|------------------|
| `char *` | 1 | +1 |
| `short *` | 2 | +2 |
| `int *` | 4 | +4 |
| `long *`（LP64 Linux） | 8 | +8 |
| `long long *` | 8 | +8 |

---

### 3. 为什么这样设计？（HFT / 底层）

**目的：** 方便 **按元素** 遍历数组 — 数组 = 一片 **连续、同类型** 的内存。

```c
char arr[5] = {0x11, 0x22, 0x33, 0x44, 0x55};
char *p = arr;
p++;   /* 自动到下一个 char，不用手写地址+1 */
```

若指针运算只做「地址 +1」：

- 遍历 `int` 数组每次都要手动 **+4**
- 遍历 `struct Order` 每次都要手算 **+sizeof(Order)**

编译器把步长编码进 **指针类型**，`p++` 就是「下一个元素」。

**HFT：** ring buffer、`mbuf` 链、定长数组扫描 — 热路径里 `T *head; head++` 的语义就是 **跳过一个 T**。

---

### 4. 两个极易混淆的写法

```c
char *p = ...;   /* p 指向某 char */

p + 1;           /* 指针运算：地址 + sizeof(char) → +1 字节 */
*(p + 1);        /* 解引用：读「下一个 char」位置的值 */

(uint64_t)(uintptr_t)p + 1;  /* 把地址当整数加 1 — 不是指针运算 */
```

对 `char *`，「指针 +1」与「地址整数 +1」**碰巧** 都是 1 字节；对 `int *`：

```c
int *q = arr;
q + 1;                      /* 地址 +4 */
(uint64_t)(uintptr_t)q + 1; /* 地址只 +1 — 完全不是下一个 int */
```

**规则：** 遍历/跳元素用 **指针运算**；只在需要和整数地址混算时用 `uintptr_t` 显式转换。

---

### 5. 与 §2.1.2 sizeof / ABI 串联

- 步长 = **`sizeof(目标类型)`**
- `sizeof` 随 **架构 + ABI** 变（见 [Ch2 §2.1.2](../../01-CSAPP-3rd/chapter-02-representing-information/notes/section-2.1.1-2.1.3-十六进制寻址与字节序.md)）
- 例：`long *p; p+1` 在 **LP64 Linux** 常 +8，**32 位 ILP32** 常 +4

**可移植代码：** 不要硬编码「+4」「+8」扫内存；用 `T *` 或 `char *` 逐字节（endian 解析时用 `unsigned char *`）。

---

### 6. 和 Ch2 字节序的关系

- **`unsigned char *` 逐字节走** — 解析 `0x11223344` 内存布局（endian），每次 `p++` **+1 字节**
- **`int *` 走** — 一次跳整个 `int`，适合 **int 数组**，不适合拆字节

读多字节整数的 endian → 用 `char *` / `unsigned char *`；读 `int arr[i]` → 用 `int *`。

---

## 一句话总结

指针 **存地址**，但 **自带类型**；`p + n` = 向前跳 **n 个该类型元素**，地址增量 = `n × sizeof(元素类型)`。  
`char` 为 1 字节，所以 `char *` 的 `p+1` 只挪 1 字节 — 这正是 Ch2 逐字节看 endian 时用它的原因。

---

## 口述巩固 · 自测

1. `int *p; p+1` 比 `p` 大几个字节？（典型 LP64）
2. 为什么解析 endian 用 `unsigned char *` 而不是 `int *`？
3. `(uintptr_t)p + 1` 和 `p + 1` 对 `int *` 有何不同？
4. `long *` 的步长为什么不能在跨平台代码里写死？

---

← [02 导读](../README.md) · [code/pointer-stride-demo.c](../code/pointer-stride-demo.c)
