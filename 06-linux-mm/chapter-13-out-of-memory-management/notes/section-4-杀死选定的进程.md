# Ch 13 §4 杀死选定的进程（`oom_kill_process` + oom_reaper）

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪** · 源码核验：Linux v6.6

---

## 本节讲什么

原书 §4 讲击杀动作：通知该 task + 共享 `mm_struct` 的所有线程、提优先级、设 `PF_MEMALLOC` + `PF_MEMDIE`、`CAP_SYS_RAWIO` 用 `SIGTERM` 否则 `SIGKILL`。

到 v6.6，这条链演变成**三层**：`oom_kill_process`（外层，处理 memcg 群杀）→ `__oom_kill_process`（信号投递 + 共享 mm 清扫）→ **oom_reaper**（内核线程主动收割地址空间，这是原书完全没有的新机制）。

---

## 1. `oom_kill_process()` 外层（oom_kill.c:1010）

```c
static void oom_kill_process(struct oom_control *oc, const char *message)
{
    struct task_struct *victim = oc->chosen;
    struct mem_cgroup *oom_group;

    task_lock(victim);
    if (task_will_free_mem(victim)) {          // ① 受害者其实已在退出
        mark_oom_victim(victim);
        queue_oom_reaper(victim);
        task_unlock(victim);  put_task_struct(victim);
        return;                                 //   只给 reserves，不再补发信号
    }
    task_unlock(victim);

    if (__ratelimit(&oom_rs))
        dump_header(oc, victim);                // ② 打日志（限速防刷屏）

    oom_group = mem_cgroup_get_oom_group(victim, oc->memcg);  // ③ 是否要杀整个 cgroup

    __oom_kill_process(victim, message);        // ④ 核心击杀

    if (oom_group) {                            // ⑤ memory.oom.group=1：群杀
        mem_cgroup_scan_tasks(oom_group, oom_kill_memcg_member, (void *)message);
        mem_cgroup_put(oom_group);
    }
}
```

**新能力 `oom_group`**：cgroup 可以设 `memory.oom.group=1`，让 OOM 时**整个 cgroup 的所有进程一起被杀**（而不是只杀得分最高的一个）。这对「一组必须同生共死的服务」有用——避免杀了 A 留下 B 独活成孤儿。

## 2. `__oom_kill_process()` 核心（oom_kill.c:914）

```c
static void __oom_kill_process(struct task_struct *victim, const char *message)
{
    struct task_struct *p;  struct mm_struct *mm;
    bool can_oom_reap = true;

    p = find_lock_task_mm(victim);              // ① 找持有 mm 的线程
    if (!p) {  /* victim 已退出，跳过 */  return; }
    ...  mm = victim->mm;  mmgrab(mm);          // ② 安全引用 mm

    count_vm_event(OOM_KILL);
    memcg_memory_event_mm(mm, MEMCG_OOM_KILL);  // ③ 统计

    do_send_sig_info(SIGKILL, SEND_SIG_PRIV, victim, PIDTYPE_TGID);  // ④ 先发信号
    mark_oom_victim(victim);                    // ⑤ 再授予 memory reserves
    pr_err("%s: Killed process %d (%s) total-vm:%lukB, anon-rss:%lukB, "
           "file-rss:%lukB, shmem-rss:%lukB, UID:%u pgtables:%lukB oom_score_adj:%hd\n",
           message, ...);

    rcu_read_lock();
    for_each_process(p) {                       // ⑥ 杀共享 mm 的其他线程组进程
        if (!process_shares_mm(p, mm))  continue;
        if (same_thread_group(p, victim))  continue;
        if (is_global_init(p)) {  can_oom_reap = false;  set_bit(MMF_OOM_SKIP, &mm->flags);  continue; }
        if (unlikely(p->flags & PF_KTHREAD))  continue;
        do_send_sig_info(SIGKILL, SEND_SIG_PRIV, p, PIDTYPE_TGID);
    }
    rcu_read_unlock();

    if (can_oom_reap)
        queue_oom_reaper(victim);               // ⑦ 排队收割
    ...
}
```

### 两个必须理解的顺序

1. **先 `SIGKILL` 再 `mark_oom_victim`（④→⑤）**。注释明确说：*「We should send SIGKILL before granting access to memory reserves in order to prevent the OOM victim from depleting the memory reserves from the user space under its control.」* —— 如果先给 reserves，victim 用户态还活着时会**先挥霍掉本就紧张的预留内存**，所以必须先用 SIGKILL 让它走向退出，再授予 `TIF_MEMDIE`。

2. **信号一律 `SIGKILL`，没有原书的 `SIGTERM` 分支**。原书里 `CAP_SYS_RAWIO` 进程用 SIGTERM「优雅退出」的路径在 v6.6 已删除——OOM 场景下**没时间等优雅退出**，`SIGKILL`（不可捕获、不可忽略、立即生效）是唯一选择。

## 3. `mark_oom_victim()`：从 `PF_MEMDIE` 到 `TIF_MEMDIE`（oom_kill.c:756）

```c
static void mark_oom_victim(struct task_struct *tsk)
{
    struct mm_struct *mm = tsk->mm;
    WARN_ON(oom_killer_disabled);
    if (test_and_set_tsk_thread_flag(tsk, TIF_MEMDIE))  // ① 已标记则返回
        return;
    if (!cmpxchg(&tsk->signal->oom_mm, NULL, mm))       // ② 绑定 oom_mm
        mmgrab(tsk->signal->oom_mm);
    __thaw_task(tsk);                                   // ③ 唤醒被 freezer 冻结的进程
    atomic_inc(&oom_victims);                           // ④ 全局受害者计数 +1
    trace_mark_victim(tsk->pid);
}
```

**版本断崖**：原书的 `PF_MEMDIE` 是 **process flag**，v6.6 改成了 **thread flag `TIF_MEMDIE`**，并把「受害者 mm」单独存到 `signal->oom_mm`（而非复用 mm 指针）。这样**同一线程组里只有被杀的那个线程**带 `TIF_MEMDIE`，其余共享 mm 的线程不共享这个标志，语义更精确。

**`TIF_MEMDIE` 的作用**：它让 `__alloc_pages_slowpath` 在**受害者退出路径上**仍能拿到 memory reserves（`ALLOC_OOM`），否则 victim 会因为「分配页也失败」而卡死在退出代码里，形成活锁。

## 4. oom_reaper：原书没有的收割机制（oom_kill.c:505-740）

老内核靠「受害者自然退出」释放内存，但如果 victim 卡在 `mmap_lock` 上，退出会非常慢甚至死锁。v4.7 起引入 **oom_reaper 内核线程**，**不等进程退出，主动 unmapping 受害者的地址空间**：

```c
static bool __oom_reap_task_mm(struct mm_struct *mm)   // oom_kill.c:513
{
    set_bit(MMF_UNSTABLE, &mm->flags);   // ① 标记「地址空间已不稳定」
    for_each_vma(vmi, vma) {
        if (vma->vm_flags & (VM_HUGETLB|VM_PFNMAP))  continue;   // ② 跳过特殊 VMA
        if (vma_is_anonymous(vma) || !(vma->vm_flags & VM_SHARED)) {  // ③ 只收匿名/私有
            tlb_gather_mmu(&tlb, mm);
            unmap_page_range(&tlb, vma, range.start, range.end, NULL);  // ④ 真释放页表
            tlb_finish_mmu(&tlb);
        }
    }
    return ret;
}
```

| 机制 | 说明 |
|------|------|
| 触发方式 | `queue_oom_reaper(victim)`（:690）→ 设 `OOM_REAPER_DELAY = 2*HZ` 的 timer（给 victim 2 秒自然退出时间）→ `wake_oom_reaper` 入队 |
| 收割内容 | **只收匿名页 + 私有映射**（文件页已在回收阶段处理，且不想阻塞 exit_mmap） |
| 收割手段 | `unmap_page_range`（TLB gather 批量失效），**跳过 VM_HUGETLB/VM_PFNMAP** |
| 重试 | `oom_reap_task` 最多 `MAX_OOM_REAP_RETRIES = 10` 次，每次 `schedule_timeout_idle(HZ/10)` |
| `MMF_UNSTABLE` | 收割期间，任何 `get_user`/`copy_from_user` 命中已收割页会 `VM_FAULT_SIGBUS`（`check_stable_address_space`，oom.h:88） |
| 兜底 | 收不干净就设 `MMF_OOM_SKIP`，让后续 OOM 轮次**不再把它当候选** |

**关键：`MMF_OOM_SKIP` 的双重身份**——既是「reaper 已收割、不再评分」的标记（§3），也是「退出路径 exit_mmap 已接管 mm、reaper 停止工作」的标记（`oom_reap_task` 末尾置位）。

## 5. 受害者退出闭环：`exit_oom_victim()`（oom_kill.c:783）

```c
void exit_oom_victim(void)
{
    clear_thread_flag(TIF_MEMDIE);
    if (!atomic_dec_return(&oom_victims))
        wake_up_all(&oom_victims_wait);
}
```

受害者退出路径（`exit_mm` 后）调用它：清 `TIF_MEMDIE`、`oom_victims` 减一；当计数归零时唤醒所有等待者（`oom_killer_disable` 就靠这个等待队列）。

---

## HFT / 嵌入式关联

| 关注点 | 建议 | 理由 |
|--------|------|------|
| oom_reaper | 别依赖 victim「快速自然退出」 | 若交易进程被误杀，2 秒后 reaper 就强制 unmapping 它的 mm，**内存回收速度不再取决于受害者自觉** |
| `MMF_UNSTABLE` | 理解收割期 SIGBUS | 收割期间进程若继续访问私有匿名页会 `SIGBUS` 而非缺页——这是 OOM 下的**数据一致性最后防线** |
| 信号一律 SIGKILL | 别指望优雅收尾 | OOM 场景「时间就是内存」，`SIGTERM` 优雅路径已删，进程收到的是不可捕获的 SIGKILL |

**要点**：OOM 击杀的现代设计哲学是**「信号 + 主动收割双保险」**——SIGKILL 负责「让进程知道自己该死」，oom_reaper 负责「进程死得慢时我来动手收内存」，两者解耦，确保内存归还**不卡在受害者退出路径上**。

---

## 衔接

§4 讲完了击杀与收割。下一节 §5 把 Ch13 所有 v6.6 变化收拢成演进全景，并给出 HFT 精读 checklist。

---

<details>
<summary>自测 5 问（点开看答案）</summary>

**Q1：为什么 `__oom_kill_process` 必须「先 SIGKILL 再 mark_oom_victim」？**

防止 victim 在用户态还活着时，先挥霍掉本就紧张的 memory reserves（oom_kill.c:944-949 的注释）。先 SIGKILL 让它进入退出路径，再授予 `TIF_MEMDIE`，保证 reserves 只用于「退出时必要的分配」。

**Q2：原书的 `PF_MEMDIE` 在 v6.6 变成了什么？为什么改？**

变成 **thread flag `TIF_MEMDIE`**（`mark_oom_victim` 里 `test_and_set_tsk_thread_flag`，oom_kill.c:764），受害者 mm 单独存 `signal->oom_mm`。这样只有**被杀的那个线程**带标志，共享 mm 的其他线程不共享，语义更精确。

**Q3：oom_reaper 收割时跳过哪些 VMA？为什么？**

跳过 `VM_HUGETLB` 和 `VM_PFNMAP`（oom_kill.c:524），因为大页和 PFN 映射需要特殊处理、OOM 紧急路径上「耗不起」；只收割 `vma_is_anonymous` 或非 `VM_SHARED` 的 VMA（匿名/私有页释放性价比最高）。

**Q4：`MMF_UNSTABLE` 置位后，进程访问已收割页会怎样？**

`get_user`/`copy_from_user` 会得到 `VM_FAULT_SIGBUS`（通过 `check_stable_address_space`，oom.h:88），而不是正常缺页——因为页内容已被回收，返回零页会导致静默数据损坏。

**Q5：`oom_killer_disable` 是如何知道「所有受害者都退出了」的？**

它 `wait_event_interruptible_timeout(oom_victims_wait, !atomic_read(&oom_victims), timeout)`（oom_kill.c:838）。而 `exit_oom_victim`（:783）在每次受害者退出时 `atomic_dec_return(&oom_victims)`，归零时 `wake_up_all` 唤醒该等待队列。

</details>
