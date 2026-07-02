# 指针运算 · 地址 vs 类型步长

> **01 CSAPP · Ch3 §3.8 延伸阅读** · 与 [§3.8 数组与指针运算](./section-3.8-数组与指针运算.md) 对照  
> **动手：** [pointer-stride-demo.c](../../code/pointer-stride-demo.c)

---

## 核心拆解：指针是地址，但指针运算自带「类型步长」

### 1. 先分清两层东西

| | 含义 |
|--|------|
| `p` **存的内容** | **内存地址**（x86-64 下指针本身占 8 字节） |
| `p` **的类型** | 如 `char *` — 「从这个地址起，按 **char（1 字节）** 解释」 |

指针加减数字，**不是** 给地址数字裸 +1，而是：

\[
\text{新地址} = \text{原地址} + n \times \sizeof(\text{指针指向的类型})
\]

---

### 2. 拿 `char *p` 举例

- `sizeof(char) == 1` → `p + 1` 地址 **+1 字节**

| 指针类型 | `p + 1` 地址增量 |
|----------|------------------|
| `char *` | +1 |
| `short *` | +2 |
| `int *` | +4 |
| `long *`（LP64 Linux） | +8 |

---

### 3. 为什么这样设计？（HFT / 底层）

数组 = **连续、同类型** 内存 — `p++` = 下一个元素，不必手算 `+sizeof(T)`。

**HFT：** ring buffer、`mbuf` 链 — `T *head; head++` 即跳过一个 `T`。

---

### 4. 两个极易混淆的写法

```c
int *q = arr;
q + 1;                      /* 地址 +4 — 指针运算 */
(uintptr_t)q + 1;           /* 地址 +1 — 不是下一个 int */
```

---

### 5. 与 Ch2 §2.1.2 sizeof / ABI 串联

- 步长 = **`sizeof(目标类型)`**，随 **ABI** 变 → [§2.1.2 数据大小](../../chapter-02-representing-information/notes/section-2.1.2-数据大小与sizeof.md) · [ABI 笔记](../../chapter-02-representing-information/notes/section-2.1.2-abi-application-binary-interface.md)
- 不要硬编码「+4」「+8」扫内存

---

### 6. 和 Ch2 字节序的关系

- **endian 解析** → `unsigned char *` 逐字节，`p++` 每次 +1
- **int 数组** → `int *`，`p++` 每次 +sizeof(int)

→ [pointer-and-bytes.c](../../code/pointer-and-bytes.c)

---

## 一句话总结

`p + n` = 跳 **n 个该类型元素**；`char *` 的 `p+1` 只挪 1 字节 — 所以 Ch2 拆 endian 用它。

---

## 口述巩固 · 自测

1. `int *p; p+1` 比 `p` 大几个字节？（典型 LP64）
2. 为什么解析 endian 用 `unsigned char *` 而不是 `int *`？
3. `(uintptr_t)p + 1` 和 `p + 1` 对 `int *` 有何不同？
4. `long *` 的步长为何不能写死？→ [ABI 笔记](../../chapter-02-representing-information/notes/section-2.1.2-abi-application-binary-interface.md)

---

← [Ch3 导读](../README.md) · [Ch2 §2.1.3 字节序](../../chapter-02-representing-information/notes/section-2.1.3-寻址与字节序.md)
