# Ch 13 §5 2.6 内核的新变化（OOM 子系统演进全景）

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪** · 源码核验：Linux v6.6

---

## 本节讲什么

原书 §5 只列了两条 2.6 时代的变化：`VM_ACCOUNT` VMA 记账、严格 overcommit 上限（`committed_AS ≤ RAM×ratio + swap`），哲学是「**记账前移，在承诺时就拒绝，而非用尽后杀人**」。

到 v6.6，OOM 子系统经历了一轮**「从启发式到语义化」的重构**。本节把 Ch13 涉及的所有断崖收拢成一张演进全景表，并落到 HFT 精读 checklist。

---

## 1. 演进全景表（2.6 → v6.6）

| 维度 | 原书（2.6） | v6.6 | 断崖性质 |
|------|-------------|------|----------|
| 评分算法 | `badness = Total VM / sqrt(CPU time)`，root/cap ÷4 | `oom_badness = RSS + swapents + pgtables + oom_score_adj`（oom_kill.c:201） | **重写**，目标从「杀可疑的」变「杀最占内存的」 |
| 调节接口 | `oom_adj`（-16~15） | `oom_score_adj`（-1000~1000，uapi/linux/oom.h:9） | **替换**，旧接口废弃但保留兼容 |
| 受害者标记 | 进程标志 `PF_MEMDIE` | 线程标志 `TIF_MEMDIE` + `signal->oom_mm`（:764） | **语义细化**（精确到线程） |
| 击杀信号 | `SIGTERM`（CAP_SYS_RAWIO）/ `SIGKILL` | 一律 `SIGKILL`（:943） | **简化**，删掉优雅退出 |
| 内存归还 | 靠受害者自然退出 | oom_reaper 主动收割（:505-740） | **新增机制** |
| 约束分类 | 无（全局一刀切） | `enum oom_constraint` 四类（oom.h:22） | **新增**，区分 memcg/cpuset/mempolicy |
| cgroup | 无 | memcg OOM + `memory.oom.group` 群杀（:1031） | **新增** |
| panic 控制 | 无 | `panic_on_oom` 0/1/2 三级（:1059） | **新增** |
| 主动免杀 | 无 | `oom_kill_allocating_task` sysctl（:1138） | **新增** |
| 用户态收割 | 无 | `process_mrelease` syscall（:1240，pidfd 触发） | **新增**（v5.15+，Android 低内存场景） |

## 2. 一条「记账前移」哲学的两条落点

原书哲学「**承诺时就拒绝，而非用尽后杀人**」在 v6.6 演变成**两条独立防线**：

```
第一道防线（记账前移，§1）
    brk/mmap/mremap ──► __vm_enough_memory()（mm/util.c:931）
        │  overcommit=2 时：vm_committed_as < commit_limit 才放行
        │  失败：返回 -ENOMEM，用户态可处理，系统没事
        ▼
第二道防线（用尽止损，§2~§4）
    页分配失败 ──► __alloc_pages_may_oom()（page_alloc.c:3273）
        │  out_of_memory() → select_bad_process() → oom_kill_process()
        │  失败：SIGKILL + oom_reaper，暴力止损
        ▼
```

**两者的关系不是「替代」而是「分层」**：第一道防的是「**承诺**过度」（虚拟内存超额），第二道防的是「**实际**耗尽」（物理页真没了）。很多 HFT 调优只关 overcommit 却忽略 OOM killer，结果就是「承诺控制住了，但物理耗尽时依然被杀」。

## 3. 版本断崖时间线（关键节点）

| 版本 | 变化 |
|------|------|
| 2.6.36 | `oom_score_adj` 取代 `oom_adj`（-1000~1000） |
| 2.6.38 | `oom_kill_allocating_task` 引入 |
| 3.x | memcg OOM 逐步完善 |
| 4.7 | **oom_reaper** 内核线程引入，主动收割 victim mm |
| 4.12 | `oom_group`（memory.oom.group）群杀 |
| 5.15 | `process_mrelease` syscall（pidfd 驱动的用户态收割） |

## 4. HFT 精读 checklist（更新版）

| 手段 | 目的 | 防线 |
|------|------|------|
| 足够物理 RAM + `mlock` 关键映射 | 根本：让系统**走不到** OOM | 前置 |
| `overcommit_memory=2` + 精确 `overcommit_ratio` | 承诺层面卡死上限，失败在 malloc 时显式暴露 | 第一道 |
| `oom_score_adj=-1000`（交易进程）/ `+1000`（辅助进程） | 明确「谁该死谁不该死」 | 第二道 |
| cgroup `memory.max` + `memory.oom.group=0` | 隔离非关键服务，触发 memcg OOM 而非全局 | 第二道 |
| `panic_on_oom=0` | 避免 OOM 升级成全机重启（几十秒中断） | 兜底 |
| 监控 dmesg `Killed process ... oom_score_adj` | 事后定位 victim + score + adj | 观测 |
| **串联**：Ch2 水位 → Ch10 回收 → Ch11 swap → **Ch13 OOM** | 把「内存压力 → 回收 → 换出 → 击杀」整条因果链打通 | 学习 |

## 5. 原书两条变化在 v6.6 的落点

- **`VM_ACCOUNT` VMA 记账**：仍存在（`vm_flags & VM_ACCOUNT`），作用于 shmem 等映射的 `security_vm_enough_memory_mm` 检查（mm/mmap.c 多处调用，如 :1366/:1941/:3086）。它保证**共享内存**这种「多进程共同承诺」的内存也被纳入 overcommit 记账。
- **严格 overcommit 上限**：落地为 `vm_commit_limit()`（mm/util.c:875）—— `(totalram - hugetlb) × ratio/100 + swap`，与 §1 完全呼应。

---

## 衔接

Ch13 至此收官。下一站是 Ch14 结束语（VM 全局交互图），把 Ch2~Ch13 的机制串成一张「内存管理全景」。

---

<details>
<summary>自测 5 问（点开看答案）</summary>

**Q1：Ch13 最重要的版本断崖是什么？为什么？**

`badness()` 评分算法被重写：原书 `Total VM / sqrt(CPU time) + root÷4` → v6.6 `oom_badness = RSS + swapents + pgtables + oom_score_adj`（oom_kill.c:201）。因为它把「杀谁」的决策从「隐式启发式（运行时间、cap）」改成「显式语义（真实占用 + 管理员声明）」，可预测性根本性提升。

**Q2：原书「记账前移」哲学在 v6.6 落成哪两条独立防线？**

第一道 `__vm_enough_memory()`（mm/util.c:931，overcommit 记账，失败 -ENOMEM 提前拒绝）；第二道 `out_of_memory()`（oom_kill.c:1103，页分配器用尽后暴力止损）。前者防「承诺过度」，后者防「物理耗尽」，分层而非替代。

**Q3：`oom_score_adj` 什么时候取代 `oom_adj`？范围分别多少？**

2.6.36 取代。`oom_score_adj` 范围 -1000~1000（uapi/linux/oom.h:9-10），旧 `oom_adj` 范围 -16~15（:18-19），保留兼容但废弃。

**Q4：`process_mrelease` syscall 是干什么的？什么时候引入？**

通过 pidfd 触发对指定进程 mm 的收割（oom_kill.c:1240），v5.15 引入，主要用于 Android 等低内存场景**由用户态主动回收**内存，无需等内核 OOM killer。

**Q5：HFT 场景为什么 `panic_on_oom` 应保持 0？**

`panic_on_oom` 一旦命中就是**全机 panic 重启**，对低延迟交易系统意味着几十秒级中断与状态丢失；保持 0 让 OOM killer 精准止损（配合 `oom_score_adj` 保住交易进程），代价是「单进程被杀」而非「全机停机」。

</details>
