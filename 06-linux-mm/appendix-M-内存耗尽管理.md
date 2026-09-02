# 附录 M 内存耗尽管理 · Out of Memory Management

> **Code Commentary** · Mel Gorman · **选读** · 源码核验：Linux v6.6（`mm/oom_kill.c`，1257 行）

概念总览 → [./chapter-13-out-of-memory-management/](./chapter-13-out-of-memory-management/) · 源码 **`mm/oom_kill.c`**（Ch13 推荐阅读第一站）

---

## 本节走读什么

原书附录 M 走读 **OOM killer**。v6.6 相对原书（2.6 时代）发生**两处断崖**：**① 评分函数 `badness()` 被重写为 `oom_badness()`（公式完全不同）**；**② 击杀从「SIGTERM→等 victim 退出」改为「SIGKILL + `oom_reaper` 异步收割」**。本附录按「触发 → 评分 → 选择 → 击杀 → 收割」主线走读。

---

## 1. 触发入口：`out_of_memory`（:1103）

```
__alloc_pages_may_oom()                       // page_alloc.c:3273（附录 F 慢路径）
   └─ out_of_memory(oc)                       // oom_kill.c:1103
        ├─ check_panic_on_oom(oc)             // :1059 先查是否 panic
        ├─ select_bad_process(oc)             // :364  选出 victim
        ├─ oom_kill_process(oc, message)      // :1010 执行击杀
        └─ schedule_timeout_killable(1)       // 若没杀成，给系统缓冲时间
```

**`struct oom_control`**（oom.h:33）携带触发上下文：`zonelist`、`nodemask`、`memcg`、`order`、`gfp_mask`、`totalpages`、`chosen`（选中的 victim）。`check_panic_on_oom`（:1059）按 `vm.panic_on_oom` 分级：**0** 正常击杀；**1** 仅当约束允许时 panic；**2** 无条件 panic。

---

## 2. 评分：`oom_badness`（:201）—— 版本断崖 ⭐

原书（2.6）评分：`badness = Total VM / sqrt(CPU time) + root 用户 ÷4`。v6.6 彻底重写为**「简单可预测」**（源码注释原文 "simple and predictable"）：

```c
long oom_badness(struct task_struct *p, unsigned long totalpages)  // :201
   points = get_mm_rss(p->mm)          // 常驻内存页数
          + get_mm_counter(p->mm, MM_SWAPENTS)   // swap 页数
          + mm_pgtables_bytes(p->mm) / PAGE_SIZE // 页表页数
   points *= 1000 / totalpages          // 归一化到 0~1000
   adj = p->signal->oom_score_adj;      // 用户可调的修正值
   points += adj > 0 ? adj * (1000 - adj) / 1000
                    : adj * (points + 1000) / 1000;
   return points;
```

**走读要点**：新版**删掉了「CPU 时间越长越不该杀」和「root 减分」两个启发式**——因为它们在容器/多租户时代不再合理（一个跑很久的泄漏进程不该被豁免）。评分 = **内存占用归一化 + `oom_score_adj` 用户修正**。`/proc/pid/oom_score` 就是读这个值（fs/proc/base.c:550）。

---

## 3. 选择：`select_bad_process`（:364）

```
select_bad_process(oc)                       // :364
   └─ 遍历所有进程 → oom_evaluate_task(task, oc)  // :308
        ├─ oom_unkillable_task()            // :162 豁免（init、内核线程、PF_MEMDIE 等）
        ├─ points = oom_badness()           // 打分
        └─ 记录最高分 → oc->chosen
```

`oom_evaluate_task`（:308）逐进程打分，选出 `points` 最高者；`oom_unkillable_task`（:162）跳过 init、内核线程、已标记 `PF_MEMDIE` 的进程。`dump_tasks`（:423）在杀不掉时打印全量进程表（OOM 日志里那段 `[ pid ] uid tgid total_vm rss ...`）。

---

## 4. 击杀：`__oom_kill_process`（:914）

```
oom_kill_process(oc, message)                // :1010（找线程、打印日志、计数）
   └─ __oom_kill_process(victim, message)    // :914
        ├─ 遍历 victim 的所有线程，do_send_sig_info(SIGKILL)   // 一律 SIGKILL
        ├─ mark_oom_victim(tsk)              // :756 标记 PF_MEMDIE → TIF_MEMDIE
        ├─ signal->oom_mm = mm               // 记录 victim 的 mm
        └─ wake_oom_reaper(mm)               // 启动收割线程
```

**两处断崖**：① **击杀信号从 SIGTERM（可被忽略）改为一律 SIGKILL**；② **`PF_MEMDIE` 退化为内部标志，用户态可见的是 `TIF_MEMDIE`**（`mark_oom_victim` :756 里设置，并写 `signal->oom_mm`）。

---

## 5. 异步收割：oom_reaper（:638）

v6.6 的 OOM 不再**等 victim 自然退出**，而是**主动收割它的内存**：

```
wake_oom_reaper(timer)                       // :660 延迟 OOM_REAPER_DELAY(2*HZ) 启动
   └─ oom_reaper()                           // :638 收割内核线程
        └─ oom_reap_task(tsk)                // :607
             └─ oom_reap_task_mm(mm)         // :566
                  └─ __oom_reap_task_mm(mm)  // :510
                       ├─ MMF_UNSTABLE 标记 + 加 mmap_lock
                       └─ unmap_page_range() 主动 unmap + 释放页
```

**走读要点**：`oom_reaper` 用 `unmap_page_range` **直接撤销 victim 的页映射并回收内存**，不依赖 victim 调度执行——即使 victim 卡在 D 状态（不可中断睡眠），内存也能被释放。重试上限 **10 次**（`MAX_OOM_REAP_RETRIES`），防止无限收割。

---

## 与正文对应

| 附录内容 | 正文落点 |
|----------|----------|
| `out_of_memory` 触发 | Ch13 §2 |
| `oom_badness` 重写 | Ch13 §3 ⭐ |
| `select_bad_process` | Ch13 §3 |
| `__oom_kill_process` + SIGKILL | Ch13 §4 |
| oom_reaper | Ch13 §4 |

---

## HFT / 嵌入式关联

| 手段 | 落点 |
|------|------|
| `oom_score_adj=-1000` | 给关键交易进程打 -1000，**永不被 OOM 选中**（`oom_badness` 直接归零） |
| 禁用 panic_on_oom | HFT 服务节点设 `vm.panic_on_oom=0`，宁可杀进程也不整机 panic |
| 内存预留 | 关键路径用 `mlock` 锁内存，避免进入 `__alloc_pages_may_oom` 的慢路径 |

---

## 相关章节

- 上一章：[appendix-L-共享内存虚拟文件系统.md](./appendix-L-共享内存虚拟文件系统.md)

---

<details>
<summary>自测 5 问（点开看答案）</summary>

**Q1：`oom_badness`（:201）与原书 `badness()` 的两处核心差异？**

① 删掉「CPU 时间越长越不杀」（`sqrt(CPU time)`）；② 删掉「root 减分」。新版 = 内存占用（RSS + swap + 页表）归一化 + `oom_score_adj` 修正，注释明确要 "simple and predictable"。

**Q2：`oom_score_adj` 如何影响评分？**

正数按 `adj * (1000 - adj) / 1000` 加到 points，负数按 `adj * (points + 1000) / 1000` 扣减。设 `-1000` 直接让评分归零，永不被选中。

**Q3：击杀信号从原书到 v6.6 改了什么？**

从 SIGTERM（可被进程忽略/处理）改为**一律 SIGKILL**，杜绝 victim 拒绝退出导致内存无法回收。

**Q4：oom_reaper 解决什么问题？**

不等 victim 自然退出，用 `unmap_page_range`（__oom_reap_task_mm :510）主动撤销映射并回收内存，即使 victim 卡在 D 状态也能释放。延迟 `OOM_REAPER_DELAY`（2*HZ）启动，重试上限 10 次。

**Q5：`check_panic_on_oom` 的 0/1/2 分别什么含义？**

0 正常击杀进程；1 仅在约束允许时 panic；2 无条件 panic（`vm.panic_on_oom` 对应）。

</details>
