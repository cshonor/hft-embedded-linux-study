## 3.8 数组分配和访问

### 3.8.1 基本原则

- 数组元素 **连续存放**；`T A[N]` 大小 `N * sizeof(T)`
- 访问 `A[i]` → 基址 + `i * sizeof(T)` — 汇编里常见 `(%base,%idx,scale)`

### 3.8.2 指针运算

**两层概念（口述）：**

1. 指针 **存的是地址**（x86-64 下 8 字节的数字）
2. 指针 **带类型** — `p+n` 不是地址裸 `+n`，而是 **+ n × sizeof(元素类型)**

\[
\texttt{p + n} \Rightarrow \text{地址} + n \times \sizeof(T)
\]

```c
int *p = A;
*(p + i)  // 等价 A[i]
p + i     // 地址增加 i * sizeof(int)，不是 i 字节
```

| 指针类型 | `p+1` 典型步长 |
|----------|----------------|
| `char *` | +1 字节 |
| `int *` | +4 字节 |
| `long *` | LP64 常 +8，ILP32 常 +4 |

**易混：**

```c
(int *)q + 1;              /* 下一个 int，地址 +4 */
(uintptr_t)q + 1;          /* 地址只 +1 — 不是指针运算 */
```

**为何这样设计：** 数组连续同类型存放 — `p++` = 「下一个元素」，不必手算偏移。HFT ring buffer / 定长数组扫描依赖此语义。

→ 口述详解：[§3.8 指针步长详解](./section-3.8-指针步长详解.md) · [pointer-stride-demo.c](../../code/pointer-stride-demo.c)

- **指针减法** 得 **元素个数**（同类型、同数组内）

### 3.8.3 嵌套数组

`A[i][j]` 行主序：`&A[i][j] = A + (i * N + j) * sizeof(T)`

### 3.8.4 定长数组 vs 3.8.5 变长数组 (VLA)

- 定长 — 栈或全局；大小编译期可知
- **VLA** — C99，栈上 `alloca` 式分配；**HFT 热路径避免**（不可预测栈扩展、GCC 扩展）

### HFT 布局

| 模式 | 说明 |
|------|------|
| **SoA** (Structure of Arrays) | 同字段连续 — SIMD、顺序扫描友好 |
| **AoS** (Array of Structures) | 对象导向 — 单条记录局部性好 |
| **Ring buffer** | 定长数组 + 头尾指针 — 行情队列标配 |

→ cache 与局部性：[Ch 6](../../chapter-06-memory-hierarchy/) · [Ch 1.5](../../chapter-01-tour-of-computer-systems/notes/section-1.5-高速缓存至关重要.md)

### 自测题

<details>
<summary>1. `a[i]` 和 `*(a+i)` 等价吗？多维数组 `a[i][j]` 的地址怎么算？</summary>

是的，`a[i]` ≡ `*(a+i)`（C 标准定义）。多维数组 `int a[R][C]` 的 `a[i][j]` 地址 = `base + (i*C + j) * sizeof(int)`——C 多维数组是**行主序**连续存储，编译器用 `lea (%rax,%rcx,4), ...` 计算地址。注意：`a[i]` 和 `a+i` 的类型不同——前者是 `int*`，后者是 `int(*)[C]`（指向数组的指针）。

</details>

<details>
<summary>2. 数组名和指针有什么区别？`sizeof(a)` 什么时候不等于 `sizeof(int*)`？</summary>

数组名在大多数上下文中**退化为**指向首元素的指针（array decay），但在两个场合不退化：1. `sizeof(a)` —— 返回整个数组大小
2. `&a` —— 返回指向数组的指针（类型 `int(*)[N]`）

`int a[10]; sizeof(a)` = 40（10×4），不是 8（指针大小）。数组作为函数参数时退化为指针——`void f(int a[10])` 中 `sizeof(a)` = 8（指针大小）。

</details>


---

← [本章导读](../README.md)
