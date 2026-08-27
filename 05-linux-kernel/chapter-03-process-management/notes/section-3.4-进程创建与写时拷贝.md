## ④ 进程创建与写时拷贝 · fork & Copy-on-Write

Unix 创建 = **`fork` + `exec`**。Linux 对 **`fork()`** 用 **写时拷贝（COW）** 优化，使「复制进程」在常见路径上极快。

| 阶段 | 行为 |
|------|------|
| **`fork` 瞬间** | **不**复制整个物理地址空间 |
| 初始 | 父子 **共享** 同一套物理页的 **只读映射** |
| **首次写入** | 页 fault → 内核 **复制该页** — 写者得到私有副本 |
| **`exec`** | 丢弃旧地址空间，加载新 ELF — 大量 COW 页直接释放 |

| 收益 | 说明 |
|------|------|
| **少复制** | 许多 `fork` 后立刻 `exec`，大量页从未被写 |
| **快创建** | 延续 Unix「**极快 fork**」传统 |
| **省内存** | 只读段（代码、部分 .rodata）可长期共享 |

#### COW 流程（ASCII）

```
fork 后：
  父页表 ──┬──► 物理页 P（ PTE 标记只读 / COW）
  子页表 ──┘

父或子 write 到 P
  │
  ├─ CPU 写保护 fault
  ├─ 内核分配新页 P'
  ├─ 复制 P → P'（若真正共享且有人写）
  └─ 更新 fault 方页表 → 可写映射 P'
```

#### 页表 vs 物理页（COW 的两个层级）

```
进程看到的地址（虚拟地址）          物理内存条上的真实位置
   ┌────────────┐   页表            ┌────────────┐
   │ 虚拟页 0   │──┐ PTE           │ 物理页 P   │  ← 4KB 真实存储（PFN）
   │ 虚拟页 1   │──┘                │            │
   │ ...        │                   │            │
   └────────────┘                   └────────────┘
       每进程一张自己的页表              机器上只有一份
```

- **物理页**：内存条上真实的 4KB 存储格子，编号叫 PFN。CPU 真正读写的对象。
- **页表**：一张「虚拟页号 → 物理页号」的翻译表，每个进程一份。进程各看各的虚拟地址，互不干扰。
- 同一物理页可出现在**多个进程的页表里** —— 这就是「共享」。fork 后父子页表都指向同一批物理页，于是「没真复制，只是多挂一张翻译表」。

对照三步：

| 阶段 | 页表层做了什么 | 物理页层做了什么 |
|------|----------------|------------------|
| fork 后 | 子页表=父页表拷贝；所有 PTE 标只读 + COW 位 | **不动**，父子指向同一批物理页 |
| 写触发 | fault → 改写者 PTE 指向新页、恢复可写 | **才复制**：找新物理页 P'，P→P' 拷内容 |
| 另一方 | 页表没动，仍只读指向原页 | 数据不变，继续用原物理页 |

> 「复制」复制的是**物理页**，「标记只读」标记的是**页表项**——两件事发生在不同层级，新手最易混的就是把「页表只读」当成「物理页只读」。

#### fork 复制什么？（内核路径 `_do_fork` / `copy_process`）

| 对象 | fork 默认 | 备注 |
|------|-----------|------|
| **地址空间** | 复制 `mm`（COW） | `CLONE_VM` 则共享 — [§3.5](./section-3.5-Linux-的线程实现.md) |
| **文件表** | 复制，fd 共享同一 file 结构 | 引用计数 +1 |
| **信号处理** | 复制 | 线程用 `CLONE_SIGHAND` 共享 |
| **PID** | 子获新 PID | 同 `tgid` 下为线程组 |

#### 用户态最小例

```c
pid_t c = fork();
if (c == 0) {
    /* 子：COW 堆栈 — 局部变量写触发复制 */
    x = 42;
    _exit(0);
}
wait(NULL);
```

#### vfork 与 fork 对比（历史）

| 调用 | 地址空间 | 父进程行为 | 现代建议 |
|------|----------|------------|----------|
| **`fork`** | COW 复制 | 并发 | 通用 |
| **`vfork`** | 共享，子先跑 | 父 **阻塞** 至子 exec/exit | 少用；`fork`+COW 已够快 |

#### 与内存管理章衔接

| 概念 | 所在 |
|------|------|
| **VMA** | 描述哪些虚拟区间可 COW — [Ch 15 §15.3](../../chapter-15-process-address-space/notes/section-15.3-虚拟内存区域.md) |
| **页表项** | 只读位、写 fault — [Ch 15 §15.7](../../chapter-15-process-address-space/notes/section-15.7-页表.md) |
| **缺页处理** | COW fault 在内核 mm 路径 — [Ch 15 §15.8](../../chapter-15-process-address-space/notes/section-15.8-从访问到缺页概念.md) |

**HFT：** 不要在热路径 **频繁 fork**（即使用 COW，仍有页表、调度、`task_struct` 开销）。Prefork  worker 池（如部分 Web 服务器）是 **启动时** 摊销成本；低延迟系统更常见 **单进程 + 线程 +  hugepage 预分配**。

→ [§3.5 clone/线程](./section-3.5-Linux-的线程实现.md) · [Ch 15 地址空间](../../chapter-15-process-address-space/notes/section-15.1-地址空间.md) · [07 TLPI Ch24/27 fork/exec](../../../03-linux-userspace-api/chapter-24-process-creation/notes) · [01 CSAPP Ch9 COW](../../../02-computer-systems/chapter-09-virtual-memory/)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 写时拷贝（COW）如何让 fork() 变快？什么情况下 COW 反而变慢？

<details><summary>答案</summary>

COW：fork 时只复制页表（不复制物理页），父子共享物理页标记只读。任一方写时触发 page fault → 内核分配新页 → 拷贝内容 → 改页表为可写。如果 fork 后立即 exec（典型 shell），COW 几乎零拷贝。如果 fork 后父子都大量写（如 Redis BGSAVE），COW 会频繁触发 page fault 反而慢。

</details>

**Q2.** vfork() 和 fork() 的区别？为什么 vfork 在现代代码中不推荐？

<details><summary>答案</summary>

vfork：父子共享页表（不是 COW），子进程挂起父进程直到 exec/exit。比 fork 快（零页表拷贝）但极危险：子进程不能修改任何变量（会破坏父进程）。现代 Linux 的 fork+COW 已足够快（仅复制页表 ~微秒级），vfork 的大多数用途已被 posix_spawn() 替代。

</details>

</details>
---
