# RCU 进阶 — Tree RCU / SRCU / lazy RCU

> **原文:** [Tree RCU](https://lwn.net/Articles/305782/) + [RCU进阶](https://lwn.net/Articles/683487/) (LWN)
> **内核版本:** Tree RCU (2.6.30+) / SRCU (2.6.18+) / lazy RCU (6.2+)
> **对标旧书:** ULK3 Ch5 (仅基础 RCU)

---

## 核心观点

ULK3 时代的 RCU 是单一全局宽限期设计，在现代多核系统 (64+ CPU) 上扩展性差。6.x 内核引入了多种 RCU 变体应对不同场景。

### Tree RCU (层次化 RCU)

**问题：** 全局 `synchronize_rcu()` 需要检查所有 CPU 是否经过静止状态 (quiescent state)，64+ 核系统上单次宽限期可能数秒。

**解决：** 将 CPU 组织成树形结构 (rcu_node tree)，每个节点汇总子节点的静止状态报告：

```
            root rcu_node
           /      |      \
       node      node     node       ← Level 1 (每组 16 CPU)
      /  |  \    / | \    / | \
    CPU CPU CPU CPU CPU CPU CPU CPU  ← Leaf (每 CPU 一个 rcu_data)
```

- 每个 CPU 只向上一级报告静止状态
- 宽限期完成只需 O(log N) 次传播，而非 O(N)
- 支持多个并发宽限期 (gp_kthread)

### SRCU (Sleepable RCU)

经典 RCU 读临界区**不能睡眠**。SRCU 允许在读临界区内睡眠：

```c
// SRCU 读端（可睡眠）
int idx = srcu_read_lock(&sp);
p = srcu_dereference(gptr, &sp);
// 可以睡眠、可以调用 kmalloc(GFP_KERNEL)
srcu_read_unlock(&sp, idx);

// SRCU 写端
synchronize_srcu(&sp);  // 比 synchronize_rcu() 慢得多
```

**代价：** SRCU 的 `srcu_read_lock()` 返回一个索引，需要传给 `srcu_read_unlock()`。宽限期比经典 RCU 慢（需要两次遍历所有 reader）。

### expedited RCU (加速 RCU)

普通宽限期等待调度器自然推进（可能数百毫秒），expedited 模式主动向所有 CPU 发 IPI 强制静止状态：

```c
synchronize_rcu_expedited();  // 快但耗 CPU（IPI 风暴）
```

### lazy RCU (6.2+)

延迟批量回收：`call_rcu()` 的回调不立即执行，而是积攒一段时间后批量执行，减少 wake-up 次数和功耗。

### RCU 变体选型表

| 变体 | 读端能否睡眠 | 宽限期速度 | 适用场景 |
|------|-------------|-----------|---------|
| **RCU** (`rcu_read_lock`) | ❌ | 普通 | 内核大部分读多写少场景 |
| **RCU expedited** | ❌ | 快 (IPI) | 模块卸载等需要快速回收 |
| **SRCU** (`srcu_read_lock`) | ✅ | 慢 | 需要在读临界区睡眠 |
| **Tasks RCU** | ✅ (voluntary) | 很慢 | voluntary context 场景 |
| **Tasks Trace RCU** | ✅ | 中等 | tracing 场景 |

---

## 与旧书差异

| ULK3 讲的 | 6.x 现代实现 |
|-----------|-------------|
| 单一全局 RCU | Tree RCU 层次化 |
| 无 SRCU | SRCU 广泛使用 (VFS、内存管理) |
| `synchronize_rcu()` 简单 | expedited / lazy 多种模式 |
| `rcu_ctrlblk` 全局结构 | `rcu_state` + `rcu_node` 树 |

### 关键代码变更

```c
// ULK3 时代 (2.6)
struct rcu_ctrlblk {
    long cur;       // 当前宽限期编号
    long completed; // 已完成的宽限期编号
    // ... 全局结构
};

// 6.x Tree RCU
struct rcu_state {
    struct rcu_node node[NUM_RCU_NODES];  // 树形节点
    unsigned long gp_seq;                  // 宽限期序列号
    unsigned long gp_max;                  // 最长宽限期
    // ... 每个 rcu_state 一棵树
};
```

---

## HFT 关联

| 场景 | RCU 变体选择 |
|------|-------------|
| **行情数据读取** | 经典 RCU (`rcu_read_lock`) — 读取路径不能阻塞但零开销 |
| **需要分配内存的读取** | SRCU — 读取路径可能需要 `kmalloc` |
| **模块卸载** | `synchronize_rcu_expedited()` — 快速回收，不长时间阻塞 |
| **避免尾延迟** | 不用 expedited — IPI 风暴会干扰交易线程 |

> **HFT 实盘：** 交易线程用经典 RCU 读取行情数据。**绝不在交易线程调用 `synchronize_rcu()` 或 `synchronize_rcu_expedited()`** ——前者阻塞数十毫秒，后者 IPI 干扰所有核。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** Tree RCU 为什么能扩展到 4096+ CPU？

> Tree RCU 将 CPU 组织成树形结构，每个 CPU 只向父节点报告静止状态，宽限期完成只需 O(log N) 次传播。全局 RCU 需要检查每个 CPU (O(N))，4096 核时差异巨大。

**Q2:** SRCU 的 `srcu_read_lock()` 为什么要返回索引？经典 RCU 不需要。

> SRCU 使用双计数器：reader 进入时锁当前索引，writer 需要等两个索引的 reader 都退出。索引告诉 `srcu_read_unlock()` 释放哪个计数器。经典 RCU 依赖禁用抢占，不需要区分。

**Q3:** expedited RCU 适合在 HFT 系统中使用吗？

> **不适合在运行时使用。** expedited RCU 通过 IPI 强制所有 CPU 进入静止状态，IPI 中断会干扰交易线程的延迟。仅适合在系统初始化或模块加载/卸载时使用，绝不在交易时段调用。

</details>
