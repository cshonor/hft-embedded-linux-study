# Ch6 · Derived data types（派生数据类型）

> **Level 1 · 相识** · 策略：**🟡 略读**（聚焦 C23 增量 + 指定初始化器）
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

## 本章讲什么

数组（含 VLA）、指针作为不透明类型、结构体、联合、typedef、复合字面量。
K&R Ch5/6 和《C 和指针》已深入覆盖；本章重点在 **C23 结构体增强**和 **复合字面量的现代用法**。

## 一、数组

### 基本概念（K&R 已覆盖，此处速过）

```c
int arr[10];                 // 定长数组
int arr2[10] = {1, 2, 3};    // 部分初始化，其余为 0
int arr3[] = {1, 2, 3};      // 大小由初始化器决定 → [3]

/* C99 指定初始化器 */
int arr4[10] = {[5] = 100, [9] = 200};  // 其余为 0
```

### VLA（变长数组）— C99 引入，C11 转可选

```c
/* C99 VLA：大小运行时确定 */
int n = compute_size();
int buf[n];           // ⚠ VLA：栈上分配，大小运行时才知道

/* ⚠ VLA 的风险：
   - 栈溢出：n 太大时直接 crash（没有 graceful failure）
   - 性能：某些平台 VLA 操作比定长数组慢
   - 调试：sizeof(buf) 在运行时才知道
*/
```

| 标准 | VLA 状态 |
|------|----------|
| C89 | 不存在 |
| C99 | 必须支持 |
| C11 | 可选（`__STDC_NO_VLA__` 定义表示不支持） |
| C23 | 仍可选，但限制为非跳转作用域 |

> **HFT / 内核立场：禁用 VLA**。内核编译选项 `-Wvla`；DPDK 也不用。需要运行时大小用 `malloc` 或栈上定长最大值。

### 柔性数组成员（Flexible Array Member）

```c
/* C99：结构体最后一个成员可以是不定长数组 */
struct msg_buffer {
    uint32_t len;
    uint8_t  data[];      // 柔性数组成员：不占 sizeof，malloc 时额外分配
};

/* 一次分配，减少内存碎片 */
struct msg_buffer *mb = malloc(sizeof(struct msg_buffer) + payload_len);
mb->len = payload_len;
memcpy(mb->data, payload, payload_len);
```

| 要点 | 说明 |
|------|------|
| `sizeof(struct msg_buffer)` | 不含 `data` 的大小（可能含尾部 padding） |
| 分配方式 | `malloc(sizeof(struct) + extra)` |
| 不能嵌套 | 含 FAM 的结构体不能作为另一个结构体的成员（除非也是最后一个） |

> HFT 场景：变长协议消息、批量订单缓冲区。DPDK `rte_mbuf` 的附载数据区也用类似设计。

## 二、结构体

### 基本概念（速过）

```c
struct point {
    int x;
    int y;
};

struct point p1 = {10, 20};                    // 顺序初始化
struct point p2 = {.x = 10, .y = 20};          // C99 指定初始化器
```

### C23 结构体增强

#### 1. 结构体标签增强

```c
/* C23：结构体标签可以出现在更多位置 */
struct ring {
    uint32_t head;
    uint32_t tail;
} __attribute__((aligned(64)));   // GNU 扩展，C23 部分标准化

/* C23：匿名结构体成员更灵活 */
struct msg {
    uint8_t  type;
    union {                         // 匿名联合
        struct order_hdr  order;
        struct cancel_hdr cancel;
    };                              // 直接通过 msg.order / msg.cancel 访问
    uint8_t  payload[];
};
```

#### 2. `[[deprecated]]` 属性

```c
struct config {
    int  new_field;
    [[deprecated("use new_field instead")]] int old_field;
};

config.old_field = 1;  // ⚠ warning: use new_field instead
```

### 内存布局与对齐

```c
struct bad {
    char  c1;     // 1 byte
    int   i;      // 4 bytes — 需要 4 字节对齐 → c1 后填充 3 bytes
    char  c2;     // 1 byte — 末尾填充 3 bytes → sizeof = 12
};

struct good {
    int   i;      // 4 bytes
    char  c1;     // 1 byte
    char  c2;     // 1 byte — 末尾填充 2 bytes → sizeof = 8
};
```

| 规则 | 说明 |
|------|------|
| 对齐 | 每个成员放在其类型对齐的倍数地址上 |
| padding | 编译器自动在成员间插入填充字节 |
| sizeof | 结构体大小是其最大成员对齐的倍数 |
| 优化 | 按大小降序排列成员可最小化 padding |

```c
/* C11：显式控制对齐 */
_Alignas(64) struct ring cache_aligned_ring;  // 强制 64 字节对齐（缓存行）

/* C23：alignas/alignof 作为关键字（不再需要 _Alignas/_Alignof） */
alignas(64) struct ring r;
static_assert(alignof(struct ring) <= 64, "");
```

> **HFT 核心技巧**：`_Alignas(64)` 防伪共享——把生产者/消费者索引放到不同缓存行。详见 [Ch12 内存模型](../ch12-c-memory-model/README.md)。

## 三、联合（Union）

```c
union value {
    int    i;
    float  f;
    uint8_t bytes[4];
};

union value v;
v.i = 0x40490FDB;
printf("%f\n", v.f);     // 3.14159...（以 float 读取同一内存）
```

### C23 联合增强：匿名联合

```c
struct packet {
    uint8_t  type;
    union {                        // 匿名联合：不需要名字
        struct hdr_a  a;
        struct hdr_b  b;
    };                             // 直接用 pkt.a / pkt.b
};
```

### 类型双关（Type Punning）

```c
/* 方法1：union（C99 起允许，C11 明确保证） */
union { uint32_t u; float f; } pun;
pun.f = 3.14f;
uint32_t bits = pun.u;   // ✅ 合法：读取 union 的非活跃成员

/* 方法2：memcpy（永远安全） */
float f = 3.14f;
uint32_t bits;
memcpy(&bits, &f, sizeof(bits));   // ✅ 最安全，编译器优化掉 memcpy

/* 方法3：指针强转 — ⚠ 严格别名违规 */
uint32_t bits = *(uint32_t *)&f;   // ❌ UB（违反 strict aliasing）
```

> **HFT 红线**：不要用指针强转做类型双关！用 `memcpy`（编译器会优化为零成本）或 `union`。
> 详见 [Ch12 Effective Type](../ch12-c-memory-model/README.md)。

## 四、typedef

```c
typedef uint32_t seq_num_t;        // 语义化命名
typedef struct ring *ring_handle_t; // 不透明句柄

seq_num_t seq = 0;
ring_handle_t r = ring_create(1024);
```

| 规则 | 说明 |
|------|------|
| typedef 不创建新类型 | 只是别名，`seq_num_t` 和 `uint32_t` 完全兼容 |
| 用于可移植性 | `typedef unsigned int uint32_t;` 在不同平台映射到不同类型 |
| 用于封装 | `ring_handle_t` 隐藏 `struct ring *` 的细节 |

## 五、复合字面量（Compound Literal，C99）

```c
/* C99：在表达式中间创建临时结构体/数组 */
struct point p = (struct point){.x = 1, .y = 2};

/* 传给函数 */
draw_line((struct point){0, 0}, (struct point){100, 200});

/* 数组复合字面量 */
int *arr = (int[]){1, 2, 3, 4, 5};   // 5 元素数组
```

| 要点 | 说明 |
|------|------|
| 作用域 | 块作用域的复合字面量生命周期到块结束；文件作用域的到程序结束 |
| 可修改 | 复合字面量是左值，可以修改 |
| DPDK/内核 | 常见于 `struct rte_eth_conf conf = (struct rte_eth_conf){...};` |

```c
/* 实际 HFT 用法：一次性初始化配置 */
rte_eth_conf_default = (struct rte_eth_conf){
    .rxmode = {
        .max_rx_pkt_len = RTE_ETHER_MAX_LEN,
    },
    .txmode = {
        .mq_mode = ETH_MQ_TX_NONE,
    },
};
```

## 六、指针作为不透明类型（速过，详见 Ch11）

```c
/* 头文件：只暴露指针类型 */
struct ring;
typedef struct ring ring_t;
ring_t *ring_create(uint32_t cap);

/* 实现文件：定义真正的结构体 */
struct ring {
    _Alignas(64) uint32_t head;
    _Alignas(64) uint32_t tail;
    // ...
};
```

> 详见 [Ch11 指针](../ch11-pointers/README.md)。

## HFT / DPDK 关联

| 特性 | HFT 用途 |
|------|----------|
| 柔性数组成员 | 变长协议消息一次分配 |
| `_Alignas(64)` | 缓存行对齐防伪共享 |
| 指定初始化器 + 复合字面量 | DPDK 配置结构初始化 |
| 匿名联合 | 消息头复用（同一内存按类型解析） |
| `memcpy` 做类型双关 | 安全提取浮点位模式（比强转安全） |
| typedef 不透明句柄 | DPDK `rte_ring`/`rte_mempool` 封装 |

## 自测题

<details><summary>1. 为什么 HFT 代码禁用 VLA？</summary>

VLA 在栈上分配，大小运行时才知道——如果 n 过大，直接栈溢出 crash，没有优雅失败机制。
此外 VLA 的 `sizeof` 是运行时的，某些平台性能差，且不利于调试。
内核和 DPDK 都禁用 VLA（`-Wvla`）。需要运行时大小用 `malloc` 或预分配最大缓冲区。
</details>

<details><summary>2. <code>struct { char c; int i; char c2; }</code> 的 <code>sizeof</code> 是多少？</summary>

通常为 12（64 位系统）。`c` 占 1 字节后填充 3 字节让 `i` 对齐到 4 字节边界，`c2` 占 1 字节后
填充 3 字节让结构体大小为最大成员对齐（4）的倍数。优化写法：把大成员放前面 →
`struct { int i; char c; char c2; }` → sizeof = 8。
</details>

<details><summary>3. 用指针强转做类型双关为什么是 UB？正确做法是什么？</summary>

C 的严格别名规则（strict aliasing）禁止通过不兼容类型的指针访问同一内存——
`*(uint32_t *)&f` 违反规则，编译器优化后可能产生错误结果。
正确做法：① 用 `memcpy`（编译器优化后零成本）；② 用 `union`（C11 明确允许）。
</details>

<details><summary>4. 柔性数组成员和指针成员有什么区别？</summary>

```c
struct with_fam { int len; int data[]; };        // 柔性数组成员
struct with_ptr { int len; int *data; };         // 指针成员
```
FAM：一次 `malloc(sizeof(struct) + n)` 搞定，数据与结构体连续存放，减少内存碎片和 cache miss。
指针：需要两次 `malloc`（结构体 + 数据区），数据不连续，需要两次 `free`。HFT 优先用 FAM。
</details>
