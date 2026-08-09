# RCU 现代实现

> 笨叔《奔跑吧 Linux 内核》读书笔记
> 对应旧书: ULK3 / LKD3 (Linux 2.6)
> 对应现代内核: Linux 5.x / 6.x

---

## 本节要点

RCU（Read-Copy-Update）是内核中最重要的无锁读机制。现代内核相比 LKD3 时代有重大演进：

- **Tree RCU（层级 RCU）**：2.6 时代的大核 RCU 在千核系统上扩展性差，Tree RCU 按 NUMA 拓扑组织 rcu_node 树，每层 fanout 限制锁竞争
- **SRCU（Sleepable RCU）**：允许读者在 RCU 临界区内睡眠（标准 RCU 不允许），代价是 `synchronize_srcu()` 更慢
- **Tasks RCU（Tasks Trace）**：5.x 引入，追踪特定任务而非全 CPU quiescent state，用于保护 per-task 数据
- **Poll RCU / lazy RCU**：6.x 引入 `call_rcu_lazy()`，延迟批量回收，减少 cache 抖动
- **RCU stall detector 增强**：`rcu_cpu_stall_timeout` 可配置多级超时 + 自动打印栈

核心思想不变：**读者无锁**（`rcu_read_lock` 仅禁止抢占），**写者拷贝更新 + 延迟释放**（等所有读者退出后 `synchronize_rcu` / `call_rcu` 回收旧数据）。

---

## 与旧书对比

| ULK3 / LKD3 (2.6) | 笨叔 (5.x/6.x) | 变化原因 |
|--------------------|-----------------|----------|
| 经典 RCU（flat） | Tree RCU（hierarchical） | 千核系统扩展性 |
| `synchronize_rcu()` 阻塞 | `call_rcu()` + kfree_rcu() 异步 | 避免写者阻塞 |
| `rcu_read_lock` = preempt_disable | 同上（但 PREEMPT_RT 下用 rwlock） | RT 内核需要可抢占 |
| 无 SRCU | `srcu_read_lock` / `synchronize_srcu` | 需要在 RCU 临界区睡眠 |
| stall 超时固定 60s | 可配置 + 多级 + 自动 dump | 大系统调试需要 |
| `kfree` 在回调中 | `kfree_rcu(ptr, offset)` 宏自动 | 减少回调函数样板代码 |
| 无 lazy RCU | `call_rcu_lazy()` 6.x+ | 减少 cache line bouncing |

---

## 关键数据结构 / 函数

```
// 源码路径: kernel/rcu/tree.c (Tree RCU)
//          kernel/rcu/srcutree.c (SRCU)
//          include/linux/rcupdate.h

// Tree RCU 的层级节点
struct rcu_node {
    raw_spinlock_t lock;        // 保护本节点
    unsigned long gp_seq;       // 宽限期序号
    unsigned long qsmask;       // 待报告 quiescent state 的 CPU 位图
    struct rcu_node *parent;    // 父节点
    // ...
};

struct rcu_state {
    struct rcu_node node[RCU_NUM_LVLS];  // 层级树根
    unsigned long gp_seq;      // 当前宽限期序号
    // ...
};

// 读者 API（无开销）
static __always_inline void rcu_read_lock(void);    // preempt_disable()
static __always_inline void rcu_read_unlock(void);  // preempt_enable()

// 写者 API
void synchronize_rcu(void);     // 等待所有读者退出（阻塞）
void call_rcu(struct rcu_head *head, rcu_callback_t func);  // 异步回调

// 现代 API
void kfree_rcu(ptr, rhf);       // 异步 kfree（6.x 用 offset 而非字段名）
void call_rcu_lazy(head, func); // 延迟批量回收（6.x+）

// SRCU（可睡眠 RCU）
int srcu_read_lock(struct srcu_struct *ssp);
void srcu_read_unlock(struct srcu_struct *ssp, int idx);
void synchronize_srcu(struct srcu_struct *ssp);
```

---

## HFT 关联

RCU 对 HFT 的核心价值是**读者零开销**：

1. **行情数据热路径**：行情分发用 RCU 保护读者链表——交易策略线程 `rcu_read_lock()` 读取行情快照（仅 preempt_disable，无锁无原子操作），行情更新线程 `synchronize_rcu()` 后释放旧快照
2. **路由表/配置热更新**：HFT 路由表用 RCU 保护——查表无锁，更新时拷贝新表 + `call_rcu` 释放旧表
3. **PREEMPT_RT 注意**：RT 内核中 `rcu_read_lock` 在某些路径下用 rwlock（可抢占），性能略降但仍是 O(1) 读
4. **SRCU 场景**：如果读者需要睡眠（如 RCU 临界区内调 `kmalloc(GFP_KERNEL)`），用 SRCU 替代标准 RCU
5. **stall 风险**：HFT 线程 SCHED_FIFO 长时间不退出 RCU 临界区 → RCU stall → 内核告警。确保 RCU 临界区极短（微秒级）

**建议**：HFT 热路径用标准 RCU（reader 零开销），避免 SRCU（reader 有原子操作）。RCU 临界区不要包含 IO/睡眠/长时间计算。

---

## 自测

<details>
<summary>Q1: RCU 读者为什么不需要锁？写者如何保证读者看到一致的数据？</summary>

RCU 读者通过 `rcu_read_lock()`（= preempt_disable()）保护——禁止抢占保证读者不会被迁移，但不需要任何锁或原子操作。写者不修改现有数据，而是创建新副本（copy），然后通过 `rcu_assign_pointer()` 原子地切换指针。旧数据通过 `synchronize_rcu()` 或 `call_rcu()` 延迟释放——等到所有在指针切换前进入 RCU 临界区的读者都退出后（宽限期 grace period），才释放旧数据。新读者看到新副本，旧读者看到旧副本，两者都一致。

</details>

<details>
<summary>Q2: Tree RCU 相比经典 RCU 解决了什么问题？</summary>

经典 RCU（flat）用一个全局位图追踪所有 CPU 的 quiescent state（静止状态）。千核系统上每个 CPU 完成宽限期报告都要写同一个位图 → cache line bouncing 严重。Tree RCU 按 NUMA 拓扑构建 rcu_node 树：叶子节点管理少量 CPU（如 16 个），中间节点合并子节点状态，根节点最终确认。每层只在本层锁上竞争，fanout 限制了竞争范围。千核系统宽限期从秒级降到毫秒级。

</details>

<details>
<summary>Q3: HFT 中使用 RCU 有什么风险？如何避免 RCU stall？</summary>

风险1：RCU stall——如果读者在 RCU 临界区内长时间不退出（如 SCHED_FIFO 线程被绑核后不被抢占），写者的 `synchronize_rcu()` 会等待超过 stall 超时（默认 60s），触发内核告警甚至 panic。避免：RCU 临界区保持极短（微秒级），不包含 IO/睡眠/长时间循环。风险2：PREEMPT_RT 下 `rcu_read_lock` 可能用 rwlock（可抢占），reader 不再完全零开销。避免：在 RT 内核上测量 RCU 临界区延迟，必要时改用 SRCU。风险3：`synchronize_rcu()` 阻塞写者线程。避免：用 `call_rcu()` 异步回调代替。

</details>
