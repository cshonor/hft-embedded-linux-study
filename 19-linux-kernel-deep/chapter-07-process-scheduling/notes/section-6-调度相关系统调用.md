## 6. 与调度相关的系统调用

---

### 一、普通进程优先级

| 调用 | 作用 |
|------|------|
| **`nice()`** | 修改静态优先级（nice 值） |
| **`getpriority()`** | 获取优先级 |
| **`setpriority()`** | 设置进程/进程组优先级 |

→ 实现路径：[Ch 10](../chapter-10-system-calls.md) · 用户态：[08 TLPI](../../../03-linux-userspace-api/)

---

### 二、CPU 亲和性

| 调用 | 作用 |
|------|------|
| **`sched_getaffinity()`** | 获取 **CPU affinity mask** |
| **`sched_setaffinity()`** | 限制进程 **只在指定 CPU** 上运行 |

HFT 标配：**交易线程绑 isolated CPU**，与 housekeeping 核分离。

→ [16 HFT 内核调优](../../../17-hft-engineering/)

---

### 三、实时调度

| 调用 | 作用 |
|------|------|
| **`sched_setscheduler()`** | 改为 **FIFO / RR** 等策略 |
| **`sched_setparam()`** | 设置 **实时优先级** |

需要特权：**`CAP_SYS_NICE`**（或 root）。

---

### 四、后续章节索引

| Ch 7 主题 | 继续读 |
|-----------|--------|
| 进程、切换 | [Ch 3 进程](../chapter-03-processes/) 🔴 |
| tick、时间片 | [Ch 6 定时测量](../chapter-06-timing/) 🟡 |
| 抢占、锁 | [Ch 5 内核同步](../chapter-05-kernel-synchronization/) 🔴 |
| syscall 路径 | [Ch 10 系统调用](../chapter-10-system-calls.md) 🔴 |
| 内存、COW | [Ch 8 内存管理](../chapter-08-memory-management.md) 🔴 |
| Modern CFS | [05 LKD Ch 4](../../../05-linux-kernel/) |
| HFT 绑核/FIFO | [16 HFT 工程](../../../17-hft-engineering/) |

### 常见陷阱

1. 混淆 `nice()` 和 `setpriority()`——`nice()` 只能相对当前值调整，`setpriority()` 可直接设置绝对值
2. 以为 `sched_setscheduler()` 需要 root——现代内核需要 `CAP_SYS_NICE` capability，非 root 也可
3. 在 RT 策略下调用 `sched_yield()`——FIFO 策略下 yield 会把线程移到同优先级队列末尾，可能不立即重新运行

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** `nice(inc)` / `setpriority(PRIO_PROCESS, pid, prio)` / `sched_setscheduler()` 的区别？

<details><summary>答案</summary>

`nice(inc)`：当前进程 nice += inc（受 RLIMIT_NICE 限制），返回新 nice 值。`setpriority(PRIO_PROCESS, pid, prio)`：设置指定进程的 nice 值（绝对值）。`sched_setscheduler(pid, SCHED_FIFO, &param)`：切换调度策略和 RT 优先级。HFT 用 `sched_setscheduler(SCHED_FIFO, .sched_priority=99)` 设置最高 RT 优先级。

</details>

**Q2.** `SCHED_FIFO` 下 `sched_yield()` 的行为是什么？

<details><summary>答案</summary>

FIFO 策略：yield 把当前线程移到同优先级队列末尾。如果队列中没有其他同优先级线程，yield 立即返回（线程继续运行）。如果有同优先级线程，下一个线程运行。yield 不会让出给低优先级线程。HFT 不应在 RT 热路径上用 yield——如果需要让出 CPU 说明设计有问题。

</details>

**Q3.** HFT 如何用 `sched_setaffinity()` + `mlockall()` 建立确定性环境？

<details><summary>答案</summary>

```c
// 1. 绑核
cpu_set_t cpuset;
CPU_ZERO(&cpuset); CPU_SET(2, &cpuset);  // 绑到 2 号核
sched_setaffinity(0, sizeof(cpuset), &cpuset);
// 2. RT 优先级
struct sched_param sp = { .sched_priority = 99 };
sched_setscheduler(0, SCHED_FIFO, &sp);
// 3. 锁内存（防 swap/page fault）
mlockall(MCL_CURRENT | MCL_FUTURE);
// 4. 预分配 + 大页
void *p = mmap(NULL, size, PROT_READ|PROT_WRITE,
               MAP_PRIVATE|MAP_ANONYMOUS|MAP_HUGETLB|MAP_POPULATE,
               -1, 0);
```

</details>

</details>

---

← [5. SMP 平衡](./section-5-SMP运行队列平衡.md) · 下一章 [Ch 8 内存管理](../chapter-08-memory-management.md)
> ↔ [LKD Ch04 §4.7 与调度相关的系统调用](../../../05-linux-kernel/00_Book_3rd_Notes/chapter-04-process-scheduling/notes/section-4.7-与调度相关的系统调用.md)
