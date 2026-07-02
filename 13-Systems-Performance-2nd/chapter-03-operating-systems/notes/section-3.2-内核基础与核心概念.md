## 3.2 内核基础与核心概念

### 运行模式与系统调用

处理器至少分两种特权级：

| 模式 | 谁跑 | 能做什么 |
|------|------|----------|
| **用户态（User Mode）** | 应用程序 | 不能直接碰硬件、不能改页表 |
| **内核态（Kernel Mode）** | 内核 | 特权指令、设备、内存管理 |

用户程序通过 **系统调用** 请求内核服务，例如：

| 调用 | 典型用途 | HFT 相关 |
|------|----------|----------|
| `read` / `write` | 文件/管道 I/O | 日志、配置（热路径常避免） |
| `send` / `recv` | 套接字 | 走内核协议栈时的必经路径 |
| `mmap` | 映射文件/匿名内存 | 大页、共享内存、ring buffer |
| `clone` / `fork` | 创建线程/进程 | 进程模型、线程池 |

**一次 syscall 可能涉及：**

1. **模式切换**（用户 → 内核 → 用户）
2. 若当前线程让出 CPU → **上下文切换**
3. 若触发缺页 → 额外内存管理路径

**调优直觉：** 热路径 **少 syscall、少拷贝、少阻塞**；内核旁路见 [12-DPDK](../../../12-DPDK-Low-Latency-Network/)、用户态栈。

---

### 中断机制（Interrupts）

| 类型 | 触发 | 例子 |
|------|------|------|
| **硬件中断（IRQ）** | 外设异步 | 网卡收包、磁盘完成 |
| **同步中断 / 陷阱** | 指令或 CPU 事件 | 系统调用入口、异常、**缺页 fault** |

Linux 为降低延迟影响，常把处理拆成：

```
上半部（Top half）  — 尽快 ACK、最小工作
下半部（Bottom half）— softirq、tasklet、ksoftirqd 稍后处理
```

**HFT：** 高频收包 → **硬中断 + NAPI + softirq** 占 CPU；`mpstat`/`perf` 里看到 `%soft` 高要往网络栈查。→ [06 内核网络 Ch 14 NAPI/RSS](../../../11-Linux-Kernel-Networking/)

---

### 进程与调度（Schedulers）

**进程状态（简化）：**

```
创建 → 就绪 ⇄ 运行 → 睡眠（等 I/O/锁）→ 退出 →（若父未 wait）僵尸
```

**调度器职责：** 在多 CPU 上决定**下一个跑谁**、跑多久。

| 负载类型 | 调度倾向 |
|----------|----------|
| **I/O 密集** | 阻塞时让出 CPU，唤醒后竞争时间片 |
| **CPU 密集** | 时间片轮转 / CFS 公平份额 |

**CPU 亲和性（Affinity）：** 调度器倾向把线程留在**同一 CPU**，保留 **cache warmth**（L1/L2 仍热）。

**HFT 实践：**

- **绑核（pinning）**：行情解析、策略、发单分池，避免与 OS/中断线程抢核
- **isolcpus / 专用核**： housekeeping 与 hot path 分离 → [10-HFT ch05](../../../15-HFT-Low-Latency-Practice/chapter-05-操作系统内核极致调优/)
- Linux 里程碑：**O(1) 调度器** → **CFS**（完全公平调度，默认策略）

→ 深入进程/调度：[LKD 3rd Ch 3](../../../03-Linux-Kernel-Development/00_Book_3rd_Notes/chapter-03-process-management/) · [Ch 4 调度](../../../03-Linux-Kernel-Development/00_Book_3rd_Notes/chapter-04-process-scheduling/)

---

### 内存管理

**虚拟内存（Virtual Memory）：**

- 每个进程见**独立虚拟地址空间**
- 支持 **overcommit**（承诺超过物理内存，靠按需分配 + swap 兜底）

**分页（Paging）：**

- 物理页 ↔ 虚拟页映射；压力时 **page reclaim**、**swap**（Linux 匿名页换出常称 swapping）

| 现象 | 性能含义 |
|------|----------|
| **缺页 fault（major）** | 需读盘/映射，延迟尖刺 |
| **minor fault** | 分配/零页，相对便宜 |
| **swap 活动** | 热路径灾难性 — HFT 通常 **mlock / 预留 / 禁 swap** |

→ 精读：[05-Linux-Virtual-Memory-Manager](../../../05-Linux-Virtual-Memory-Manager/) Ch 3 页表/TLB/大页；[note-透明大页 THP](../../../05-Linux-Virtual-Memory-Manager/chapter-03-page-table-management/notes/note-透明大页THP.md)

→ SysPerf 专章：[Ch 7 内存](../../chapter-07-memory/)

---

### I/O 与文件系统

**VFS（Virtual File System）：** 统一抽象，`ext4`/`xfs`/管道/套接字等走同一套接口。

**I/O 栈与缓存：**

```
应用 read/write
  → VFS
    → 页缓存（Page Cache）— 命中则免磁盘
      → 块层 → 磁盘
```

**性能要点：** 读写过文件系统 = 可能进 **page cache**；HFT 热路径多为 **内存 + 网络**，磁盘/FS 多为日志与配置（SysPerf Ch 8/9 可 ⚪）。

---

### 多处理器与资源控制

| 概念 | 含义 | HFT 相关 |
|------|------|----------|
| **SMP** | 对称多处理，多核共享内存 | NUMA 拓扑、跨 node 访问 |
| **IPI** | 核间中断 | TLB shootdown、调度迁移 |
| **Kernel preemption** | 内核可被打断 | 低延迟内核配置常细调抢占 |
| **cgroups** |  cgroup 限额 CPU/内存/IO | 容器环境；裸机共置时防邻居干扰 |

→ NUMA / CPU：[Ch 6](../../chapter-06-cpus/)；容器与云：[Ch 11](../../chapter-11-cloud-computing/)（HFT 常 ⚪）

---


---

← [本章导读](../README.md)
