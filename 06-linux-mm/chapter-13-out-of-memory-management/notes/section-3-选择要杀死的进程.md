# Ch 13 §3 选择要杀死的进程（`oom_badness` / `select_bad_process`）

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪** · 源码核验：Linux v6.6

---

## 本节讲什么

原书 §3 的核心是 `badness()` 打分：

```
badness ∝ Total VM / sqrt(CPU 运行时间)     （运行久 → 分母大 → 分数低）
```

外加「root / `CAP_SYS_ADMIN` / `CAP_SYS_RAWIO` 分数 ÷4」的权限保护。

**到 v6.6，这套算法被彻底推翻**。新 `oom_badness()`（oom_kill.c:201）的注释直言：

> *The heuristic for determining which task to kill is made to be **as simple and predictable as possible**.*
> *The goal is to return the highest value for the task **consuming the most memory**.*

**「杀最占内存的」取代了「杀最可疑的」**。这是 Ch13 最重要的版本断崖，本节重点拆它。

---

## 1. `oom_badness()` 新算法（oom_kill.c:201）

```c
long oom_badness(struct task_struct *p, unsigned long totalpages)
{
    long points, adj;

    if (oom_unkillable_task(p))                 // ① init / 内核线程 → 不参评
        return LONG_MIN;

    p = find_lock_task_mm(p);                   // ② 取线程组里持有 mm 的那个线程
    if (!p)
        return LONG_MIN;

    adj = (long)p->signal->oom_score_adj;       // ③ 显式调节值
    if (adj == OOM_SCORE_ADJ_MIN ||             //    -1000：显式「别杀我」
        test_bit(MMF_OOM_SKIP, &p->mm->flags) || //    已被 reaper 收割
        in_vfork(p)) {                          //    vfork 中（父进程等子进程）
        task_unlock(p);
        return LONG_MIN;
    }

    /* 评分基线 = 进程占用的真实内存 */
    points = get_mm_rss(p->mm)                    // RSS（匿名+文件）
           + get_mm_counter(p->mm, MM_SWAPENTS)   // 已换出的 swap 页
           + mm_pgtables_bytes(p->mm) / PAGE_SIZE;// 页表占的页
    task_unlock(p);

    /* oom_score_adj 归一化到 totalpages 量纲 */
    adj *= totalpages / 1000;                     // -1000~1000 → 按 totalpages 缩放
    points += adj;

    return points;
}
```

### 与原书算法的对比（版本断崖）

| 维度 | 原书 `badness()`（2.6） | v6.6 `oom_badness()` |
|------|------------------------|----------------------|
| 分子 | `Total VM`（虚拟内存，含未用） | `RSS + swapents + pgtables`（**真实占用**） |
| 分母 | `sqrt(CPU time)`（运行久 → 分低） | 无分母，**运行时间完全不参与** |
| 权限保护 | root/`CAP_SYS_ADMIN`/`CAP_SYS_RAWIO` ÷4 | 删掉，改为 `oom_score_adj` 显式调节 |
| 目标 | 「杀短命暴涨的」 | 「杀**最占内存**的」 |
| 可预测性 | 低（CPU 时间、cap 都动态） | 高（只看内存 + 显式 adj） |

**为什么推翻**：原书算法里「sqrt(CPU time)」会让**长期运行的 daemon 几乎免疫**——但它们恰恰可能吞掉最多内存（比如泄漏的老服务）。新算法回归本质：**谁占的内存最多，谁就该为内存不足负责**，管理员想让谁免疫，用 `oom_score_adj=-1000` **显式声明**，而不是靠 CPU 运行时间这种隐式代理。

## 2. 谁「不参评」（`oom_unkillable_task` + 跳过条件）

```c
// oom_kill.c:162
static bool oom_unkillable_task(struct task_struct *p)
{
    if (is_global_init(p))       // pid 1（init/systemd）——杀了系统就崩
        return true;
    if (p->flags & PF_KTHREAD)   // 内核线程——没有用户 mm，杀了没意义
        return true;
    return false;
}
```

`oom_badness` 里返回 `LONG_MIN`（负无穷，等于「永不选中」）的三种额外情况：

| 条件 | 含义 |
|------|------|
| `oom_score_adj == OOM_SCORE_ADJ_MIN`（-1000） | 显式声明「OOM 不可杀」 |
| `MMF_OOM_SKIP` 置位 | 已被 oom_reaper 收割，杀了也白杀 |
| `in_vfork(p)` | 子进程 vfork 期间，父进程在等它 exec，杀了会破坏 vfork 语义 |

## 3. `oom_evaluate_task` / `select_bad_process` 遍历（oom_kill.c:308/:364）

```c
static int oom_evaluate_task(struct task_struct *task, void *arg)
{
    struct oom_control *oc = arg;
    long points;

    if (oom_unkillable_task(task))          goto next;
    if (!is_memcg_oom(oc) && !oom_cpuset_eligible(task, oc))  goto next; // cpuset 不符
    if (!is_sysrq_oom(oc) && tsk_is_oom_victim(task)) {  // 已有 victim 且非 MMF_OOM_SKIP
        if (test_bit(MMF_OOM_SKIP, &task->signal->oom_mm->flags))  goto next;
        goto abort;                          // 多个 victim 争 reserves → 中止
    }
    if (oom_task_origin(task)) {             // 本次分配 origin（set_current_oom_origin）
        points = LONG_MAX;  goto select;     // 直接选中，无需评分
    }
    points = oom_badness(task, oc->totalpages);
    if (points == LONG_MIN || points < oc->chosen_points)  goto next;
select:
    ... oc->chosen = task; oc->chosen_points = points;
    return 0;
abort:
    oc->chosen = (void *)-1UL;  return 1;    // 中止信号：调用方退出循环
}

static void select_bad_process(struct oom_control *oc)
{
    oc->chosen_points = LONG_MIN;
    if (is_memcg_oom(oc))
        mem_cgroup_scan_tasks(oc->memcg, oom_evaluate_task, oc);  // 只扫 cgroup 内
    else {
        rcu_read_lock();
        for_each_process(p) if (oom_evaluate_task(p, oc)) break;  // 扫全系统
        rcu_read_unlock();
    }
}
```

**两个亮点**：

1. **`oom_task_origin`**（oom.h:54）：`set_current_oom_origin()` 可把某个进程标记为「本次 OOM 的源头」（比如 kswapd 之外的换入路径），评分直接 `LONG_MAX` **无条件选中**，跳过所有比较。
2. **`abort` 语义**：若已经有一个 victim 正拿着 memory reserves 且未 MMF_OOM_SKIP，再选第二个会**稀释 reserves**，于是 `chosen = (void *)-1UL` 让 `out_of_memory` 放弃本次击杀。

## 4. `oom_score_adj` 全貌

用户态接口（uapi/linux/oom.h:9-10）：

```c
#define OOM_SCORE_ADJ_MIN   (-1000)
#define OOM_SCORE_ADJ_MAX   1000
#define OOM_ADJUST_MIN      (-16)   // 旧 oom_adj 接口（废弃但保留兼容）
#define OOM_ADJUST_MAX      15
```

- **`/proc/<pid>/oom_score_adj`**：`-1000~1000`，`-1000` 完全免疫、`1000` 极优先被杀。
- **`/proc/<pid>/oom_score`**：只读，**动态计算**的「被杀倾向」，见 fs/proc/base.c:550：

```c
static int proc_oom_score(...)
{
    totalpages = totalram_pages() + total_swap_pages;
    badness = oom_badness(task, totalpages);
    if (badness != LONG_MIN)
        points = (1000 + badness * 1000 / (long)totalpages) * 2 / 3;  // 缩放到 0~2000 量程
    seq_printf(m, "%lu\n", points);
}
```

**注意 `/proc/pid/oom_score` 与 `/proc/pid/oom_score_adj` 不是一回事**：前者是「结果」（含 oom_score_adj 影响的实时倾向），后者是「输入」（用户显式调节的偏移量）。

## 5. HFT / 嵌入式关联

| 手段 | 写法 | 效果 |
|------|------|------|
| 交易主进程免疫 | `echo -1000 > /proc/<pid>/oom_score_adj` | `oom_badness` 直接返回 `LONG_MIN`，**永不参评** |
| 子进程继承 | `systemd` 的 `OOMScoreAdjust=` | 让整个服务树统一免疫/优先 |
| 优先牺牲辅助进程 | `echo 1000 > .../oom_score_adj` | 让日志/监控进程在 OOM 时**最先被牺牲** |

**边界**：`oom_score_adj=-1000` 只保证「**不被选中杀死**」，**完全不防** swap stall / direct reclaim 延迟尖刺（Ch10）。真正的低延迟保障靠**物理 RAM + mlock + cgroup 隔离**三件套，`oom_score_adj` 只是最后一道「别杀错人」的保险。

---

## 衔接

§3 讲清了「选谁」。下一节 §4 讲「怎么杀」：`oom_kill_process` / `__oom_kill_process` 的信号投递顺序，以及 v6.6 新增的 **oom_reaper** 收割机制。

---

<details>
<summary>自测 5 问（点开看答案）</summary>

**Q1：v6.6 的 `oom_badness` 评分基线由哪三部分构成？**

`get_mm_rss()`（RSS，匿名+文件页）+ `get_mm_counter(mm, MM_SWAPENTS)`（已换出的 swap 页）+ `mm_pgtables_bytes(mm)/PAGE_SIZE`（页表占页数），三者相加（oom_kill.c:230-232）。

**Q2：原书 `badness() = Total VM / sqrt(CPU time)` 为什么被推翻？**

「sqrt(CPU time)」让长期运行的 daemon 几乎免疫被杀，但它们可能是真正的内存泄漏大户；「Total VM」含大量未实际使用的虚拟内存，会误杀「VM 大但 RSS 小」的进程。新算法回归「谁真实占内存最多谁负责」，把保护权交给显式的 `oom_score_adj`。

**Q3：`oom_badness` 返回 `LONG_MIN` 的四种情况分别是什么？**

① `oom_unkillable_task`（pid 1 或内核线程）；② `oom_score_adj == -1000`（显式免疫）；③ `MMF_OOM_SKIP`（已被 reaper 收割）；④ `in_vfork`（vfork 语义保护）。

**Q4：`oom_task_origin` 命中时评分是多少？为什么？**

`LONG_MAX`，直接 `goto select` 无条件选中（oom_kill.c:323-326）。它表示该进程是本次内存压力的「源头」（通过 `set_current_oom_origin` 标记），优先于一切评分比较。

**Q5：`/proc/pid/oom_score` 和 `/proc/pid/oom_score_adj` 的区别？**

`oom_score_adj` 是**输入**（用户显式写 `-1000~1000` 的偏移量）；`oom_score` 是**输出**（实时计算的结果倾向，由 `oom_badness` 套公式 `(1000 + badness*1000/totalpages)*2/3` 缩放而来，fs/proc/base.c:550），前者影响后者。

</details>
