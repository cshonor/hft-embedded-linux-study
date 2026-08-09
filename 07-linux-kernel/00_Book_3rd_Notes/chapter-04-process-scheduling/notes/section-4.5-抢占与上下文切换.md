## ⑤ 抢占与上下文切换 · Preemption and Context Switch

调度「决定换人」之后，真正换寄存器与地址空间的动作叫 **上下文切换**。

#### `context_switch()` 换什么

| 切换内容 | 说明 |
|----------|------|
| **处理器状态** | 通用寄存器、PC、栈指针、特权级相关 |
| **虚拟内存** | 用户进程间切换时换 `mm` / 页表（同 mm 的线程可优化） |
| **内核栈** | 切到目标任务的内核栈 |

切换本身有成本（缓存/TLB 扰动）— HFT 才强调 **少切换、绑核、同核缓存亲和**。

#### `need_resched`

| 概念 | 含义 |
|------|------|
| **`need_resched`** | 「该调度了」标志 |
| 谁置位 | 定时器 tick（周期份额检查）、**唤醒更「优先」任务**、显式 `set_need_resched` 等 |
| 何时真正切 | 在 **安全抢占点** 检查标志 → `schedule()` |

CFS 里两类常见触发（详见 [§4.3](./section-4.3-Linux-调度算法.md)）：

1. **周期性**：tick 更新 `vruntime`，当前任务应得份额用尽；  
2. **唤醒抢占**：新唤醒任务 `vruntime` 足够小，直接抢当前。  

**不是**「只有固定时间片耗尽才调度」。

```
中断 / 定时器 / 唤醒更高优先级
       │
       ▼
  置 need_resched
       │
  抢占点（返回用户态 / preempt_enable / 显式 schedule）
       │
       └──► schedule() ──► context_switch()
```

#### 用户抢占 vs 内核抢占

| 类型 | 时机 | 含义 |
|------|------|------|
| **用户抢占** | **从内核返回用户空间前** | 用户态任务之间换人 |
| **内核抢占**（2.6+） | 内核路径在 **可抢占** 处 | 内核态也可能被更高优先级任务换下 |

| 不可随意抢占时 | 例子 |
|----------------|------|
| 持有自旋锁 | 临界区必须原子完成 |
| `preempt_disable` / 中断关闭 | 保护 per-CPU 或短临界区 |
| 中断/软中断上下文 | 另有规则（Ch 7–8） |

#### 抢占计数 · `preempt_count`（概念）

| 想法 | 说明 |
|------|------|
| 计数 > 0 | **禁止调度抢占**（仍可能关中断） |
| 回到 0 | 若 `need_resched` 已置，可能立刻调度 |

**HFT：**

| 抖动源 | 工程手段 |
|--------|----------|
| 内核抢占 + 长临界区 | 短锁、少关抢占、`PREEMPT_RT` 类内核（进阶） |
| 同核被无关任务打断 | **`isolcpus` / cpuset + affinity** |
| 切换过多 | 无锁队列、忙等（慎用）、批处理 |

→ **Ch 1** 宏内核+抢占 · [01 Day 16 多任务](../../../../projects/P9-os-from-scratch/thirty-days-os/day-16-multitask2/) · [4.6 RT](./section-4.6-实时调度策略.md)

### 常见陷阱

1. 混淆用户态抢占和内核态抢占——用户态在 syscall 返回/中断返回时抢占，内核态需要 CONFIG_PREEMPT
2. 以为上下文切换只是保存寄存器——还要切换 mm_struct（页表/TLB）、FPU 状态、TLS
3. 忽略 switch_mm 的开销——页表切换 + TLB 刷新是 context switch 中最昂贵的部分

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 用户态抢占和内核态抢占的触发点分别是什么？

<details><summary>答案</summary>

用户态抢占：① syscall 返回用户态时检查 need_resched。② 中断返回用户态时检查。内核态抢占（CONFIG_PREEMPT）：① 中断返回内核态时（preempt_count==0）。② preempt_enable() 时。③ 显式 schedule()。无 CONFIG_PREEMPT 时内核代码不可被抢占（除非自愿 schedule）。

</details>

**Q2.** context_switch 的两个核心步骤及各自开销？

<details><summary>答案</summary>

① switch_mm()：切换页表（写 CR3）+ TLB 刷新。开销 ~1-3us（TLB 重建最贵）。内核线程不需要（lazy TLB）。② switch_to()：保存/恢复寄存器 + FPU 状态。开销 ~100-300ns。总 context switch 延迟 ~1-5us。HFT 用绑核 + isolcpus 消除切换。

</details>

**Q3.** HFT 如何测量和消除 context switch？

<details><summary>答案</summary>

测量：① `perf stat -e context-switches`。② `/proc/[pid]/status` 的 `voluntary_ctxt_switches` / `nonvoluntary_ctxt_switches`。③ `pidstat -w -p [pid]`。消除：① 绑核 + SCHED_FIFO。② `isolcpus` 隔离。③ `mlockall` 防 swap 引起的切换。④ 无锁设计避免 mutex 阻塞。目标：0 nonvoluntary switches。

</details>

</details>


> ↔ [ULK Ch7 §2 调度策略与抢占](../../../../08-linux-kernel-deep/chapter-07-process-scheduling/notes/section-2-调度策略与抢占.md)
---
