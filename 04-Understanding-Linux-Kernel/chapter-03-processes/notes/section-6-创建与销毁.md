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

→ 程序加载：[Ch 20 程序执行](../../chapter-20-program-execution.md) · [01 CSAPP](../../../01-CSAPP-3rd/) Ch 8

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

---

← [5. 进程切换](./section-5-进程切换.md) · 下一章 [Ch 4 中断与异常](../../chapter-04-interrupts-and-exceptions.md)
