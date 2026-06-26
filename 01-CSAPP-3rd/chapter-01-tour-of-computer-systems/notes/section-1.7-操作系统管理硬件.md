## 1.7 操作系统管理硬件

OS 在 **应用程序与硬件之间** 提供统一抽象，防止进程互相踩踏，并 **更高效** 地使用资源。

### 1.7.1 进程 (Processes)

- **进程 = 程序的一次运行实例** — 独占（逻辑上）虚拟地址空间、文件描述符、权限上下文
- **上下文切换 (context switch)** — CPU 从进程 A 换到 B：保存/恢复寄存器、切换页表、刷新 TLB（有成本）
- **并发假象** — 单核上多进程靠 **时间片轮转**；多核上真并行

**HFT：**

- 热路径进程 **固定 CPU affinity**，减少迁移与 cache 冷启动
- 与 housekeeping（日志、监控）**分进程/分核**（→ [12-HFT ch05](../../../15-HFT-Low-Latency-Practice/chapter-05-操作系统内核极致调优/)）

### 1.7.2 线程 (Threads)

- **线程** — 同一进程内多个执行流，**共享** 地址空间与文件描述符
- 创建/切换通常比进程轻，但引入 **竞态、锁、伪共享**
- 现代 Linux：**N:M 线程模型**（用户线程 ↔ 内核线程）

**HFT：**

- 常见模型：**单进程多线程** — 收包线程 / 策略线程 / 发单线程，用 **无锁队列** 或 **单 writer 原则** 分隔
- 深入 → [Ch 12 并发编程](../../chapter-12-concurrent-programming/)

### 1.7.3 虚拟内存 (Virtual Memory)

- 每个进程看到 **连续的虚拟地址**；MMU + 页表映射到 **物理页**
- 好处：**隔离**、**简化链接**、**比物理 RAM 更大的逻辑空间**（换页到磁盘）
- **页 fault** — 访问未映射或未驻留页触发内核处理（慢）

**HFT：**

- 热路径数据 **mlock / 大页 / 预 fault**，避免 tick 上 page fault
- **NUMA：** 分配与访问同节点（→ [Ch 9](../../chapter-09-virtual-memory/)、[07-Gorman](../../../07-Linux-Virtual-Memory-Manager/)）

### 1.7.4 文件 (Files)

- **Unix 哲学：一切皆文件** — 普通文件、目录、设备、socket 都用 **字节 I/O 接口**（`read`/`write`）
- 内核 **VFS** 统一不同文件系统与设备

**HFT：**

- 行情/订单：**socket 也是文件描述符** — `epoll` 等多路复用（→ [Ch 10–11](../../chapter-10-system-io/)、[08-UNP](../../../11-UNP-Vol1/)）
- 配置文件：启动时读一次，不在热路径 `open`

→ OS 专章：[03-SysPerf Ch 3](../../../03-Systems-Performance-2nd/chapter-03-operating-systems/)

---

← [本章导读](../README.md)
