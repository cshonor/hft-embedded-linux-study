## 6. 进程的创建与销毁

---

### 一、创建：一条底层路径

用户态 API 不同，内核底层汇聚：

```
fork() / vfork() / clone()
        ↓
    do_fork()
        ↓
   copy_process()
```

| 调用 | 典型用途 |
|------|----------|
| `fork()` | 复制进程（现代多由 `clone` 实现） |
| `vfork()` | 子进程先跑，共享地址空间（已较少用） |
| `clone()` | 精细控制共享项 — **线程创建**靠它 |

→ 系统调用路径：[Ch 10](../../chapter-10-system-calls.md)

---

### 二、写时复制（Copy-On-Write, COW）

`fork` 时**不立刻**复制父进程所有物理页：

1. 父子 **共享** 相同物理页（只读映射）
2. **任一方写入** → 缺页 / 保护异常 → 内核分配新物理页并复制

极大加速进程创建 — 依赖 [Ch 2 分页](../../chapter-02-memory-addressing/) · [Ch 9 VMA](../../chapter-09-process-address-space/)

---

### 三、内核线程（Kernel Threads）

内核自己创建的后台执行流，例如：

- `kswapd` — 内存回收  
- `pdflush` — 写回脏页（2.6 时代名称）

特点：

- **仅内核态**运行  
- 只使用 **3 GB 以上**的内核线性地址空间（2.6 模型）  
- 不参与用户态地址空间

---

### 四、销毁：`exit` 与僵尸

| 阶段 | 说明 |
|------|------|
| `_exit()` → `do_exit()` | 释放内存、文件、信号量等；向父进程发信号 |
| `EXIT_ZOMBIE` | 进程已死，**进程描述符**仍留，等父进程 `wait()` |
| 父进程 `wait()` | 彻底回收，变为 `EXIT_DEAD` |

Ch 1 提到的 `init` 收养孤儿进程 — 避免僵尸泄漏。

→ 程序加载：[Ch 20 程序执行](../../chapter-20-program-execution.md) · [01 CSAPP](../../../02-computer-systems/) Ch 8

---

### 五、后续章节索引

| Ch 3 主题 | 继续读 |
|-----------|--------|
| 谁下一个运行 | [Ch 7 进程调度](../../chapter-07-process-scheduling.md) 🔴 |
| COW、VMA、页表 | [Ch 9 进程地址空间](../../chapter-09-process-address-space/) 🔴 |
| 中断打断执行流 | [Ch 4 中断与异常](../../chapter-04-interrupts-and-exceptions.md) 🔴 |
| 睡眠、锁、唤醒 | [Ch 5 内核同步](../../chapter-05-kernel-synchronization.md) 🔴 |
| fork/exit/wait 入口 | [Ch 10 系统调用](../../chapter-10-system-calls.md) 🔴 |
| 退出信号 | [Ch 11 信号](../../chapter-11-signals.md) 🟡 |

### 常见陷阱

1. 以为 `fork()` 立即复制内存——COW 机制下只复制页表（PTE 设为只读），物理页延迟到第一次写时才复制
2. 混淆 `exit()` 和 `_exit()`——`exit()` 是 glibc 包装（跑 atexit handler + flush stdio），`_exit()`/`sys_exit_group()` 是内核直接终止
3. 以为僵尸进程是 bug——这是正常状态，父进程还没 `wait()` 回收子进程的退出状态

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** `fork()` 中 COW 的具体流程是什么？

<details><summary>答案</summary>

① `dup_mm()` 复制 `mm_struct` 和页表（PTE），所有 PTE 设为只读。② 物理页不复制，引用计数 +1。③ 子进程写某页 → page fault → `do_wp_page()` → 分配新物理页，复制内容，PTE 改为可写，旧页引用计数 -1。COW 省内存但首次写有 fault 开销。

</details>

**Q2.** 进程退出时内核做了哪些清理？

<details><summary>答案</summary>

① `do_exit()`：释放 `mm_struct`（如引用计数归零）、关闭 fd、释放信号队列、从 PID 哈希/任务链表移除。② 状态设为 `EXIT_ZOMBIE`，保留 `task_struct`（含退出码 `exit_code`）等待父进程 `wait()`。③ 父进程 `wait()` → `release_task()` 释放 `task_struct`。孤儿进程由 `init`（PID 1）自动回收。

</details>

**Q3.** HFT 中为什么要避免在热路径上 `fork()`/`exec()`？

<details><summary>答案</summary>

`fork()` 复制页表的开销与进程地址空间大小成正比（大程序可达毫秒级）。`exec()` 更昂贵：丢弃页表 + 加载 ELF + 重新初始化地址空间。HFT 进程应在启动时 `fork` + `exec` 所有 worker，之后不再创建新进程。用 `posix_spawn()` 或 `vfork()`（不复制页表）可减小 `fork` 开销。

</details>

</details>

---

← [5. 进程切换](./section-5-进程切换.md) · 下一章 [Ch 4 中断与异常](../../chapter-04-interrupts-and-exceptions.md)
