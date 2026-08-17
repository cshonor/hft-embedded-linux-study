## 2. 原子操作 (Atomic Operations)

---

### 一、定义与地位

**原子操作**：不可被中断的一个或一系列操作 — **其他同步原语的基石**。

---

### 二、x86 硬件支撑

| 机制 | 作用 | 指令示例 |
|------|------|----------|
| 对齐单次读写 **自然原子性** | 对齐的 1/2/4/8 字节读写硬件保证原子 | `MOV` (对齐) |
| **`LOCK` 前缀** | 锁总线 / 锁 Cache 行 — 跨核可见 | `LOCK ADD`, `LOCK XADD` |
| **缓存一致性协议** (MESI) | 多核 Cache 行状态同步 | 硬件自动 |
| **`CMPXCHG` (CAS)** | **比较并交换** — 无锁数据结构核心 | `LOCK CMPXCHG` |
| **`MFENCE`/`SFENCE`/`LFENCE`** | 内存屏障 — 防止指令重排 | `MFENCE` |

**CAS 语义：**

```c
/* CAS 伪代码 — 硬件保证原子性 */
bool CAS(uint64_t *addr, uint64_t expected, uint64_t new_val) {
    if (*addr == expected) {    /* 比较 */
        *addr = new_val;        /* 交换 */
        return true;            /* 成功 */
    }
    return false;               /* 失败 — 有其他核抢先写入 */
}

/* 实际 x86 指令 */
/* lock cmpxchg [rdi], rsi  ;  如果 [rdi]==rax，写入 rsi，设 ZF=1 */
```

---

### 三、内存模型与内存序

**x86 是 TSO（Total Store Order）模型** — 比 ARM 弱模型更强，但仍需屏障：

| 屏障 | x86 指令 | 语义 | 典型场景 |
|------|----------|------|----------|
| 全屏障 `rte_mb()` | `MFENCE` | Load 和 Store 都不能跨屏障重排 | 多生产者发布数据 |
| 写屏障 `rte_wmb()` | `SFENCE` | Store 不能跨屏障重排 | 发布数据后更新标志位 |
| 读屏障 `rte_rmb()` | `LFENCE` | Load 不能跨屏障重排 | 读取标志位后消费数据 |
| Acquire `rte_smp_rmb()` | `LFENCE` (x86) | 后续读不提前 | 读锁/标志后读数据 |
| Release `rte_smp_wmb()` | `SFENCE` (x86) | 前写不延后 | 写数据后写锁/标志 |

**发布-消费模式：**

```c
/* 生产者：写数据 → 写屏障 → 发布标志 */
data[idx] = value;
rte_wmb();                        /* 确保数据先写完 */
rte_atomic32_set(&ready, 1);      /* 发布标志 */

/* 消费者：读标志 → 读屏障 → 读数据 */
if (rte_atomic32_read(&ready) == 1) {
    rte_rmb();                    /* 确保标志先读到 */
    use(data[idx]);               /* 数据已可见 */
}
```

> **注意：** x86 的 TSO 模型下，Load-Load 和 Store-Store 天然有序，`rte_smp_rmb()` 和 `rte_smp_wmb()` 实际编译为空操作（`compiler barrier`）。但 ARM 平台上这些是真正的内存屏障指令。DPDK 统一 API 保证跨平台正确性。

---

### 四、DPDK：`rte_atomic.h`

| API | 用途 | 底层 |
|-----|------|------|
| `rte_atomic32_inc()` | 原子 +1 | `LOCK INC` |
| `rte_atomic32_add()` | 原子加 | `LOCK XADD` |
| `rte_atomic64_cmpset()` | CAS 64-bit | `LOCK CMPXCHG` |
| `rte_atomic32_exchange()` | 原子交换 | `XCHG` (自带 LOCK) |

```c
/* DPDK 原子计数器 — 跨核统计 */
static rte_atomic64_t rx_total;

/* 收包热路径 — 原子递增 */
rte_atomic64_inc(&rx_total);

/* 读取（非热路径） */
uint64_t total = rte_atomic64_read(&rx_total);
```

---

### 五、与无锁 ring 的关系

`rte_ring` 多生产者入队：**CAS 更新 `prod_head`** — 同一时刻仅一核成功，失败则重试 → [5. 无锁机制](./section-5-无锁机制.md)。

---

### 六、对照

- 内核侧同类原语 → [ULK Ch5 §3 基础同步原语](../../../../16-linux-kernel-deep/chapter-05-kernel-synchronization/notes/section-3-基础同步原语.md)
- 内存序 / 屏障 → [02-CSAPP](../../../../02-computer-systems/) · [Ch2 Cache 一致性](../../chapter-02-cache-and-memory/notes/section-4-Cache一致性与无锁设计.md)
- C++ 内存模型 → [04-cpp M3 并发](../../../../04-cpp/M3-deep-principles/)

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 读写锁](./section-3-读写锁.md)
