# Ch12 · The C memory model（C 内存模型）

> **Level 2 · 相知** · 策略：**🔴 精读**
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

## 本章讲什么

C 的统一内存模型：所有对象本质上都是字节数组。**Effective Type** 规则决定"能否把一块内存
当某类型访问"——这是 DPDK 零拷贝解析网络报文的理论基础。对齐（alignment）决定数据在内存中
的摆放规则——`_Alignas(64)` 防伪共享是 HFT 核心技巧。

**本章是全书对 HFT 最关键的两章之一**（另一章是 Ch21 原子）。

## 一、统一内存模型

### 核心思想：对象 = 字节序列

C 标准把内存抽象为：**所有对象都是 `unsigned char` 数组的视图**。

```
内存（字节序列）：
┌────┬────┬────┬────┬────┬────┬────┬────┐
│ 00 │ 01 │ 02 │ 03 │ 04 │ 05 │ 06 │ 07 │
└────┴────┴────┴────┴────┴────┴────┴────┘
  ←── uint32_t ──→  ←── uint32_t ──→
  ←  int16_t →       ←  int16_t →
  ← char → ← char →  ← char → ← char →
```

**任何对象都可以通过 `unsigned char *` 访问其底层字节**，这是 C 的保证：

```c
uint32_t val = 0xDEADBEEF;
unsigned char *bytes = (unsigned char *)&val;

/* 检查字节序 */
printf("byte 0: %02X\n", bytes[0]);  // 小端: EF / 大端: DE
```

| 规则 | 说明 |
|------|------|
| `unsigned char *` 可以别名任何类型 | 安全地以字节视角访问任何对象 |
| `char *` 同上 | `char` 与 `unsigned char` 等价（也包含 `signed char`） |
| 其它类型指针不能别名 | `int *` 和 `float *` 指向同一内存 = 严格别名违规 |

> 这就是为什么 `memcpy` 的实现可以安全工作：它通过 `char *` 访问源和目标，不违反别名规则。

## 二、Effective Type 规则（核心）

### 什么是 Effective Type

**Effective Type** = 编译器认为"这块内存当前应该按什么类型来访问"。

| 内存来源 | Effective Type |
|----------|----------------|
| 声明的变量 `int x;` | `int`（声明类型即 effective type） |
| `malloc` 返回的内存 | **无**（最初没有 effective type） |
| 通过 `malloc` 内存存储值后 | **存储操作的左值类型**（如果带声明类型） |
| `memcpy` 到 `malloc` 内存后 | **源对象的类型** |

### 关键规则：何时可以 cast

```c
/* 场景1：声明变量 → 类型固定 */
int x = 42;
float *fp = (float *)&x;    // ❌ UB：effective type 是 int，不能当 float 读

/* 场景2：malloc 内存 → 可以"赋予"类型 */
void *buf = malloc(64);
uint32_t *seq = buf;        // ✅ 通过 uint32_t 左值写入 → effective type 变为 uint32_t
*seq = 1;

/* 场景3：网络报文 → DPDK 零拷贝 */
void *pkt = rte_pktmbuf_mtod(mbuf, void *);  // malloc 类内存
struct eth_hdr *hdr = pkt;                   // ✅ 可以：内存无 effective type
hdr->ether_type = 0x0800;                     // 写入 → effective type 变为 struct eth_hdr
```

### HFT 核心应用：零拷贝报文解析

```c
/* DPDK 收到的报文在 mbuf 中，是 malloc 类内存（无 effective type） */
struct rte_mbuf *mbuf = rte_eth_rx_burst(...);

/* 直接 cast 成协议头结构 → 合法！因为内存无 effective type */
struct eth_hdr  *eth  = rte_pktmbuf_mtod(mbuf, struct eth_hdr *);
struct ip_hdr    *ip   = (struct ip_hdr *)((char *)eth + sizeof(*eth));
struct tcp_hdr   *tcp  = (struct tcp_hdr *)((char *)ip + (ip->ihl * 4));

/* 直接读取字段 → 合法！写操作赋予了 effective type */
uint16_t dst_port = ntohs(tcp->dst_port);
```

| 如果 effective type 规则不允许 | 后果 |
|-------------------------------|------|
| 需要先 `memcpy` 到结构体再读 | 多一次拷贝 → HFT 延迟增加 |
| 不能直接 cast 网络字节 | 失去零拷贝优势 |

> **Effective type 规则是 DPDK 零拷贝的理论基础**：malloc 类内存没有预设类型，
> 可以安全地 cast 成协议结构体直接访问。如果内存来自声明变量（`struct eth_hdr hdr;`），
> 则 effective type 已固定，不能再 cast 成其它类型读。

### 严格别名规则（Strict Aliasing）

**两种合法的类型双关方式**：

```c
/* ✅ 方式1：通过 char 指针（永远合法） */
float f = 3.14f;
unsigned char *bytes = (unsigned char *)&f;
uint32_t bits = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

/* ✅ 方式2：memcpy（编译器优化为零成本） */
uint32_t bits;
memcpy(&bits, &f, sizeof(bits));

/* ✅ 方式3：union（C11 明确允许） */
union { float f; uint32_t u; } pun;
pun.f = 3.14f;
uint32_t bits = pun.u;

/* ❌ 方式4：不兼容类型指针强转 → UB */
uint32_t bits = *(uint32_t *)&f;   // 严格别名违规！
```

| 允许别名的类型对 | 说明 |
|-----------------|------|
| 任何类型 ↔ `char`/`unsigned char`/`signed char` | 字节视角访问 |
| `uint32_t` ↔ `int`（如果底层相同） | 兼容类型 |
| `struct A *` ↔ `struct B *`（含相同初始成员） | 首成员兼容（有限制） |
| union 的不同成员 | C11 允许读取非活跃成员 |

> **HFT 红线**：永远不用指针强转做类型双关。用 `memcpy`（编译器优化后零成本）或 `union`。

## 三、对齐（Alignment）

### 对齐基础

**对齐** = 数据在内存中的起始地址必须是其大小的倍数（或实现定义的值）。

```
地址:  0x00  0x01  0x02  0x03  0x04  0x05  0x06  0x07
       ┌─────────────────┐
uint32_t (4字节对齐)       ← 地址必须是 4 的倍数
       ┌─────────────────────────────────┐
uint64_t (8字节对齐)                          ← 地址必须是 8 的倍数
```

| 类型 | 典型对齐（x86-64） | 说明 |
|------|-------------------|------|
| `char` | 1 | 任何地址 |
| `int16_t` | 2 | 偶数地址 |
| `int32_t` | 4 | 4 的倍数 |
| `int64_t` / `double` | 8 | 8 的倍数 |
| 指针 | 8 | 8 的倍数 |
| `struct` | 最大成员的对齐 | 整个结构体也要对齐 |

### `alignof` / `alignas`

```c
/* C11：查询对齐 */
_Static_assert(alignof(int) == 4, "int must be 4-aligned");
_Static_assert(alignof(double) == 8, "double must be 8-aligned");

/* C23：alignof/alignas 成为关键字（不再需要 _Alignof/_Alignas） */
static_assert(alignof(uint64_t) == 8, "");

/* 强制对齐 */
alignas(16) int buf[4];          // buf 地址 16 字节对齐
alignas(64) struct ring cache_ring;  // 64 字节对齐（缓存行）
```

### 缓存行对齐防伪共享（HFT 核心技巧）

**伪共享** (false sharing)：多个 CPU 核修改同一缓存行中的不同变量，导致缓存行在核间反复弹跳。

```
没有对齐 — 两个变量在同一缓存行（64 bytes）：
┌─────────────────────────────────────────────────────────────┐
│  head (4B)  │  ...padding...  │  tail (4B)  │  ...padding... │
└─────────────────────────────────────────────────────────────┘
  ← CPU 0 写 head →  ← CPU 1 写 tail →
  ↑ 同一缓存行！每次写都使对方的缓存行失效 → 性能灾难

对齐后 — 两个变量在不同缓存行：
┌─────────────────────────────────────────────────────────────┐
│  head (4B)  │  ...60 bytes padding...                      │  ← 缓存行 0（CPU 0 独占）
├─────────────────────────────────────────────────────────────┤
│  tail (4B)  │  ...60 bytes padding...                      │  ← 缓存行 1（CPU 1 独占）
└─────────────────────────────────────────────────────────────┘
  各自修改自己的缓存行，互不干扰
```

```c
/* DPDK rte_ring 的实际做法 */
struct rte_ring {
    alignas(64) uint32_t head;      // 生产者索引 — 独占缓存行
    alignas(64) uint32_t tail;      // 消费者索引 — 独占缓存行
    // ...
} __rte_cache_aligned;
```

| 内核对应 | C11/C23 写法 |
|----------|-------------|
| `____cacheline_aligned_in_smp` | `alignas(64)` |
| `__cacheline_aligned` | `alignas(64)` |

> **缓存行大小**：x86-64 通常 64 字节；ARM 可能 64 或 128。用 `sizeof(struct rte_ring)` 或
> `sysconf(_SC_LEVEL1_DCACHE_LINESIZE)` 查询。DPDK 定义 `RTE_CACHE_LINE_SIZE`。

### 结构体布局优化

```c
/* ❌ 差：padding 浪费空间 */
struct bad {
    char  c1;     // 1B + 3B padding
    int   i;      // 4B
    char  c2;     // 1B + 3B padding
};  // sizeof = 12

/* ✅ 好：按大小降序排列 */
struct good {
    int   i;      // 4B
    char  c1;     // 1B
    char  c2;     // 1B + 2B padding
};  // sizeof = 8
```

## 四、`void *` 与未指定对象

`void *` 是"通用对象指针"——可以指向任何对象，但不能解引用（不知道读多少字节）。

```c
void *p = malloc(64);      // malloc 返回 void *
int *ip = p;               // void * → int *，隐式转换（C 中不需要强转）
*ip = 42;                  // 通过 int * 访问 → 赋予 effective type

/* C 中 void * 不需要强转（C++ 才需要） */
int *arr = malloc(10 * sizeof(int));   // ✅ C 合法
int *arr = (int *)malloc(10 * sizeof(int));  // C++ 风格，C 中多余但不报错
```

| 规则 | 说明 |
|------|------|
| `void *` 可隐式转换为任何对象指针 | C 的特性（C++ 需要显式转换） |
| 不能解引用 `void *` | 编译器不知道读多少字节 |
| 不能做指针算术 `p + 1` | 步长未知（GCC 扩展按 1 字节算，但不可移植） |
| `malloc` 返回 `void *` | 分配的内存无 effective type |

## 五、`volatile` 语义

`volatile` 告诉编译器：这个变量的值可能在编译器不知道的时候改变（硬件、其它线程、信号处理器）。

```c
/* MMIO 寄存器映射 */
volatile uint32_t *status_reg = (volatile uint32_t *)0xFE000000;

/* 编译器不会优化掉对 volatile 的读写 */
while (*status_reg & BUSY_BIT) {
    // 每次都真正读寄存器，不会被优化成只读一次
}
```

| `volatile` 的效果 | 说明 |
|-------------------|------|
| 禁止缓存到寄存器 | 每次访问都从内存读 |
| 禁止重排 | volatile 访问顺序保持（相对于其它 volatile） |
| 不保证原子性 | 多字节 volatile 读写仍可能撕裂 |
| 不保证线程同步 | C11 `_Atomic` 才做这个 |

### `volatile` vs `_Atomic` vs 内存屏障

| 需求 | 工具 |
|------|------|
| MMIO 寄存器 | `volatile` |
| 信号处理器的 flag | `volatile sig_atomic_t` |
| 多线程共享计数器 | `_Atomic int`（C11） |
| 无锁数据结构 | `_Atomic` + 内存序（C11，见 Ch21） |
| 内核中内存屏障 | `smp_wmb()` / `smp_rmb()`（非 C 标准） |

> **HFT 常见误区**：用 `volatile` 做多线程同步——**不行！** `volatile` 不保证原子性也不保证内存序。
> 多线程共享数据必须用 `_Atomic`（C11）或 `memory_order_*`（C11/C23）。详见 [Ch21](../ch21-atomic-access-memory-consistency/README.md)。

## 六、隐式与显式转换

### 整数转换

```c
/* 隐式转换： widening（安全） vs narrowing（可能丢数据） */
int    i = 42;
long   l = i;           // widening：安全
double d = i;           // widening：安全
char   c = i;           // narrowing：截断（可能丢数据，编译器警告）

/* 显式转换 */
int truncated = (int)3.99;   // = 3（向零截断）
```

### 指针转换

```c
/* 合法转换 */
void *vp = &x;                    // 任何对象指针 → void *（隐式）
int  *ip = vp;                    // void * → int *（隐式，C）
char *cp = (char *)&x;           // 任何指针 → char *（合法，字节视角）

/* 非法转换（UB） */
float *fp = (float *)&x;        // ❌ 严格别名违规
uint32_t *up = (uint32_t *)&f;  // ❌ 同上
```

## HFT / DPDK 关联总结

| 概念 | HFT 应用 |
|------|----------|
| **Effective type** | DPDK 零拷贝报文解析（malloc 内存 cast 成协议头） |
| **严格别名** | 不要用指针强转做类型双关，用 `memcpy` 或 `union` |
| **`alignas(64)`** | 缓存行对齐防伪共享（rte_ring head/tail 分离） |
| **`char *` 别名** | 安全地以字节视角访问任何数据 |
| **`volatile`** | MMIO 寄存器、信号 flag（不做多线程同步！） |
| **`_Atomic`** | 多线程共享数据（详见 Ch21） |
| **结构体布局** | 按大小降序排列减少 padding |

## 自测题

<details><summary>1. 为什么能安全地把 malloc 返回的内存 cast 成任意结构体指针？</summary>

malloc 返回的内存没有 effective type（它是原始字节）。通过任意类型的左值写入时，
effective type 变为该左值的类型。所以可以安全地 cast 成 `struct eth_hdr *` 并访问字段。
但注意：如果内存来自声明变量 `struct foo x;`，则 effective type 已固定为 `struct foo`，
不能再 cast 成其它不兼容类型。DPDK 零拷贝解析正是利用 malloc 内存无 effective type 的特性。
</details>

<details><summary>2. <code>*(uint32_t *)&amp;f</code>（f 是 float）为什么是 UB？正确做法是什么？</summary>

float 变量的 effective type 是 `float`，通过 `uint32_t *` 访问违反严格别名规则（strict aliasing）。
编译器开启 `-O2` 后可能产生错误结果。正确做法：
① `memcpy(&bits, &f, sizeof(bits))`——编译器优化为零成本；
② `union { float f; uint32_t u; }`——C11 明确允许读取非活跃成员。
</details>

<details><summary>3. 伪共享是什么？怎么用 <code>alignas</code> 解决？</summary>

伪共享：多个 CPU 核频繁修改同一缓存行（64 字节）中的不同变量，导致缓存行在核间反复失效和传输，
性能大幅下降。解决：用 `alignas(64)` 把需要独立修改的变量放到不同缓存行。
DPDK rte_ring 的 head/tail 各自 `alignas(64)` 就是为了让生产者和消费者操作不同缓存行。
</details>

<details><summary>4. <code>volatile</code> 能做多线程同步吗？为什么？</summary>

不能。`volatile` 只保证每次访问都真正读写内存（不缓存到寄存器），但不保证：
① 原子性（多字节读写可能被中断撕裂）；② 内存序（不阻止编译器/CPU 重排非 volatile 访问）；
③ 可见性（不保证其它核心看到最新值）。多线程同步必须用 C11 `_Atomic`（带内存序）或锁。
内核中用 `smp_wmb()`/`smp_rmb()` 等内存屏障。
</details>

<details><summary>5. 什么时候用 <code>unsigned char *</code> 访问其它类型的数据？</summary>

当你需要以字节视角查看对象底层表示时：① 检查字节序；② 实现序列化/反序列化；
③ 实现 `memcpy`/`memcmp` 等字节级操作；④ 检查内存内容（调试）。
`unsigned char *` 是唯一可以合法别名任何类型的指针（C 标准保证）。
</details>
