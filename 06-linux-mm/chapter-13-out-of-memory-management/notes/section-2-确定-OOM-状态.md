# Ch 13 §2 确定 OOM 状态（`out_of_memory` + `oom_control`）

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪** · 源码核验：Linux v6.6

---

## 本节讲什么

原书 §2 讲 `out_of_memory()` 的「防误杀」启发式：还有 swap？距上次失败 > 5s？过去 1s 无失败？过去 5s 失败 < 10 次？——用来区分「I/O 慢 / 回收进行中」与「真的没人可杀了」。

到 v6.6，这套**时间窗启发式已被重构**：判定逻辑集中到 `struct oom_control`（oom.h:33）+ `enum oom_constraint`（oom.h:22）四分类 + 几条**硬条件**上。本节把 `out_of_memory()`（oom_kill.c:1103）逐段拆开。

---

## 1. `struct oom_control`（include/linux/oom.h:33）

这是贯穿整个 OOM killer 的「上下文包」，一次 OOM 事件只创建一份，在各阶段间传递：

```c
struct oom_control {
    struct zonelist *zonelist;        // 用于判定 cpuset 约束
    nodemask_t *nodemask;             // 用于判定 mempolicy 约束
    struct mem_cgroup *memcg;         // NULL = 全局 OOM，非 NULL = memcg OOM
    const gfp_t gfp_mask;             // 触发分配时的 GFP 标志
    const int order;                  // -1 表示 sysrq 触发，否则仅供显示
    unsigned long totalpages;         // 本次评分的「总内存」分母
    struct task_struct *chosen;       // 选中的受害者
    long chosen_points;               // 受害者得分
    enum oom_constraint constraint;   // 约束类型（见下）
};
```

**关键字段 `memcg`**：`NULL` 是**全局 OOM**（系统物理内存真不够了），非 `NULL` 是 **cgroup OOM**（只是某个 cgroup 达到 `memory.max`，系统整体可能还有空闲）。`is_memcg_oom(oc)` 就是 `oc->memcg != NULL`，整条逻辑链据此分叉。

## 2. `enum oom_constraint` 四约束（oom.h:22）

```c
enum oom_constraint {
    CONSTRAINT_NONE,            // 无约束，全局物理内存不足
    CONSTRAINT_CPUSET,          // 受 cpuset 墙限制（只能用某几个 node）
    CONSTRAINT_MEMORY_POLICY,   // 受 mempolicy 限制（mbind/set_mempolicy）
    CONSTRAINT_MEMCG,           // 受 memcg 内存上限限制
};
```

判定函数 `constrained_alloc()`（oom_kill.c:302）的决策树：

```
is_memcg_oom(oc)? ──是──► CONSTRAINT_MEMCG（totalpages = memcg 上限）
        │否
totalpages = totalram_pages() + total_swap_pages   // 默认全局
        │
!CONFIG_NUMA ? ──是──► CONSTRAINT_NONE
        │否
gfp_mask & __GFP_THISNODE ? ──是──► CONSTRAINT_NONE
        │否
nodemask 受限（非全内存节点）? ──是──► CONSTRAINT_MEMORY_POLICY
        │否
cpuset 墙拦了某些 zone ? ──是──► CONSTRAINT_CPUSET
        │否
CONSTRAINT_NONE
```

**为什么约束重要**：它决定 `totalpages`（评分分母）用多大内存池。比如 cpuset 把进程锁在 1 个 node 上，即使全系统还有 100GB 空闲，OOM 评分也只按**那个 node 的 present pages** 来算——**约束决定了「什么算内存不足」的基准**。

## 3. `out_of_memory()` 逐段拆解（oom_kill.c:1103）

```c
bool out_of_memory(struct oom_control *oc)
{
    unsigned long freed = 0;

    if (oom_killer_disabled)                     // ① killer 被禁用 → 直接返回
        return false;

    if (!is_memcg_oom(oc)) {                     // ② 全局 OOM：先问 notifier
        blocking_notifier_call_chain(&oom_notify_list, 0, &freed);
        if (freed > 0 && !is_sysrq_oom(oc))
            return true;   // "Got some memory back in the last second."
    }

    if (task_will_free_mem(current)) {           // ③ 当前进程已在退出/有 SIGKILL
        mark_oom_victim(current);
        queue_oom_reaper(current);
        return true;      // 让它优先拿 reserves，别杀别人
    }

    if (!(oc->gfp_mask & __GFP_FS) && !is_memcg_oom(oc))  // ④ 不能做 FS 回收
        return true;      // 全局 OOM 且无 __GFP_FS：先放弃，等可回收

    oc->constraint = constrained_alloc(oc);      // ⑤ 判定约束
    if (oc->constraint != CONSTRAINT_MEMORY_POLICY)
        oc->nodemask = NULL;
    check_panic_on_oom(oc);                      // ⑥ panic_on_oom 检查

    if (!is_memcg_oom(oc) && sysctl_oom_kill_allocating_task && ...) {  // ⑦ 直接杀触发者
        oom_kill_process(oc, "Out of memory (oom_kill_allocating_task)");
        return true;
    }

    select_bad_process(oc);                      // ⑧ 遍历评分选受害者
    if (!oc->chosen) {                           // ⑨ 没得杀 → 死锁
        dump_header(oc, NULL);
        pr_warn("Out of memory and no killable processes...\n");
        if (!is_sysrq_oom(oc) && !is_memcg_oom(oc))
            panic("System is deadlocked on memory\n");
    }
    if (oc->chosen && oc->chosen != (void *)-1UL)
        oom_kill_process(oc, ...);               // ⑩ 执行击杀
    return !!oc->chosen;
}
```

## 4. 与原书「防误杀启发式」的对应

| 原书启发式（2.6） | v6.6 对应机制 | 变化 |
|-------------------|---------------|------|
| 仍有 swap 空间 → 不杀 | `task_will_free_mem` + `__GFP_FS` 检查（:1114/:1127） | 从「看 swap 余量」改为「看当前进程能不能自己退」 |
| 距上次失败 > 5s → 不杀 | `blocking_notifier_call_chain` 查 freed（:1108） | 从时间窗改为**事件驱动**：有人已还内存就不杀 |
| 过去 1s 无失败 / 5s 失败 < 10 次 | 无直接对应，由 allocator 的 `did_some_progress` 提前拦截 | 回收进展检查上移到 page_alloc，不留在 OOM 里 |
| 5s 内已杀过 → 等释放 | `task_will_free_mem(current)` + oom_reaper（:1114） | 受害者机制从「等待」升级为「主动收割」 |

**核心变化**：原书靠「时间窗 + 计数器」做启发式，v6.6 靠「**谁正在退出、谁已经还了内存、能不能做 FS 回收**」这些**语义更硬**的条件判断，误杀窗口更窄。

## 5. `check_panic_on_oom()` 三级（oom_kill.c:1059）

```c
static void check_panic_on_oom(struct oom_control *oc)
{
    if (likely(!sysctl_panic_on_oom)) return;         // 0：不 panic（默认）
    if (sysctl_panic_on_oom != 2) {                   // 1：只对全局 OOM panic
        if (oc->constraint != CONSTRAINT_NONE) return; // cpuset/mempolicy/memcg 不 panic
    }
    if (is_sysrq_oom(oc)) return;                     // sysrq 触发不 panic
    dump_header(oc, NULL);
    panic("Out of memory: %s panic_on_oom is enabled\n",
          sysctl_panic_on_oom == 2 ? "compulsory" : "system-wide");
}
```

| `panic_on_oom` | 行为 |
|----------------|------|
| `0` | 正常走 OOM killer（默认） |
| `1` | **只有全局（CONSTRAINT_NONE）OOM 才 panic**；cpuset/mempolicy/memcg 不 panic |
| `2` | **无条件 panic**（"compulsory"，含 memcg/cpuset 场景） |

---

## HFT / 嵌入式关联

| 关注点 | 建议 | 理由 |
|--------|------|------|
| `panic_on_oom` | 交易主机**保持 0** | panic 是「宁可全机重启」，对低延迟系统意味着**几十秒级中断**，不如让 OOM killer 精准止损 |
| cgroup 隔离 | 非关键服务放 `memory.max` 限死的 cgroup | 触发的是 **memcg OOM**（`CONSTRAINT_MEMCG`），只杀 cgroup 内进程，**不拖垮全局** |
| `oom_kill_allocating_task` | 默认 0，别开 | 开了会让「当前分配者」直接被杀，可能误杀无辜的 mmap 大额分配者 |

**要点**：`out_of_memory` 的 `return true` 分支（②③④）是**「暂时不杀」**的信号——它告诉分配器「这次先算了」，分配器会重试。真正的击杀只在走到 ⑩ 时发生。理解这个「返回 true ≠ 杀了人」的语义，读 OOM 日志才不会误判。

---

## 衔接

§2 讲了 `out_of_memory` 何时「决定杀人」。下一节 §3 讲**杀谁**：`select_bad_process` / `oom_badness` 的评分算法，以及它与原书 `badness()` 的根本差异。

---

<details>
<summary>自测 5 问（点开看答案）</summary>

**Q1：`struct oom_control` 里 `memcg == NULL` 和 `!= NULL` 分别代表什么？**

`NULL` 是**全局 OOM**（系统物理内存整体不足）；非 `NULL` 是 **memcg OOM**（某个 cgroup 撞上 `memory.max`，系统整体可能还有空闲）。`is_memcg_oom(oc)` 判断这个字段，整条逻辑据此分叉。

**Q2：`enum oom_constraint` 有哪四种？各在什么条件下判定？**

`CONSTRAINT_NONE`（全局）、`CONSTRAINT_CPUSET`（cpuset 墙拦了某些 zone）、`CONSTRAINT_MEMORY_POLICY`（nodemask 受限）、`CONSTRAINT_MEMCG`（memcg 上限）。判定在 `constrained_alloc()`（oom_kill.c:302）。

**Q3：`out_of_memory` 里 `task_will_free_mem(current)` 命中时做了什么？为什么？**

`mark_oom_victim(current)` + `queue_oom_reaper(current)` 后 `return true`（oom_kill.c:1114-1118）。因为当前进程**已经在退出/持有 pending SIGKILL**，让它优先拿到 memory reserves 快速退出释放内存，比另杀一个无辜进程更划算。

**Q4：`!(gfp_mask & __GFP_FS)` 且非 memcg 时为什么直接 `return true` 不杀？**

`__GFP_FS` 缺失意味着这次分配**不允许触发文件系统 I/O 回收**（比如在回写/锁依赖路径上）。此时 OOM killer 无从判断真实内存压力，硬杀反而可能死锁，所以先放弃让分配器重试（:1127）。memcg OOM 除外，因为它必须响应。

**Q5：`panic_on_oom=1` 和 `=2` 的区别？**

`1`：只有**全局 OOM（CONSTRAINT_NONE）**才 panic，cpuset/mempolicy/memcg 不 panic；`2`：**无条件** panic（"compulsory"）。sysrq 触发的 OOM 两者都不 panic。

</details>
