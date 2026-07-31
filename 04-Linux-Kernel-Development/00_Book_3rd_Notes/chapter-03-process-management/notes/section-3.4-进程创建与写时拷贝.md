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

→ [§3.5 clone/线程](./section-3.5-Linux-的线程实现.md) · [Ch 15 地址空间](../../chapter-15-process-address-space/notes/section-15.1-地址空间.md) · [07 TLPI Ch24/27 fork/exec](../../../../07-The-Linux-Programming-Interface/chapter-24-process-creation/notes.md) · [01 CSAPP Ch9 COW](../../../../01-CSAPP-3rd/chapter-09-virtual-memory/)

---
