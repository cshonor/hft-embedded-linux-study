# 跨模块联动指南

> 本仓库 **技术板块 `00`–`16` + 外部 C++ 索引 `17`**。  
> **推荐阅读顺序** → [LEARNING-CHAIN.md](./LEARNING-CHAIN.md)

---

## 一、仓库板块总览

| 板块 | 文件夹 | 维度 |
|------|--------|------|
| 交易金融理论 | `00` | 业务 / LOB |
| **程序与硬件** | **`01` CSAPP → `02` Hennessy** | 知其所以然 |
| **性能（后置）** | **`03` SysPerf → `04` BPF** | 12 DPDK 之后、15 HFT 之前 · 文件夹编号不变 |
| Linux 内核 | **`05` LKD → `06` ULK → `07` Gorman** | 调度 / 源码 / VM |
| **Linux 用户态** | `08` TLPI | syscall · epoll · mmap · 线程 |
| **系统底层动手** | `09` | **01 MikanOS**（主线）· 02 30天（可选） |
| **C++ 语言** | **`17`** [cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes) | Modern C++ · 并发 · 对象模型（**09 后、10 前**） |
| **C++ 网络实战** | `10` PNP / muduo | Reactor 实验骨架 |
| **网络完整栈** | `11`–`14` | UNP → TCP/IP → Rosen → DPDK |
| **低延迟工程** | `15` HFT Practice | C++ 整机实践 |
| **Rust 量化** | `16` Rust Guide | Rust 工程 |

### `14` 与网络板块（`10`–`13`）的分界

| | `10`–`13` 网络技术栈 | `15` HFT 工程实践 |
|---|--------------------------|-------------------|
| **关注点** | 报文收发、协议、网卡、内存收发模型 | 裸机调优、对接规范、系统架构、延迟压测 |
| **产出** | 理解/实现网络路径 | 把技术栈落到交易系统工程 |
| **关系** | 底层能力 | 业务侧整合 |

---

## 二、网络学习链（推荐顺序）

```
03 LKD → 04 ULK → 05 Gorman → 06 TLPI（epoll / mmap / 调度）
    ↓
07/01 MikanOS（HFT 主线）
    ↓
17 C++（至少 M1 Modern C++）
    ↓
10 陈硕 PNP / muduo
    ↓
11 UNP + CSAPP Ch10–11
    ↓
10 TCP/IP → 11 Rosen → 12 DPDK
    ↓
13 SysPerf → 14 BPF（后置 · 观测落地）
```

| 轨道 | 外部仓库 | 本仓库索引 |
|------|----------|------------|
| **C++ 9 书 + 可选** | [cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes) | [17-cpp-learning-notes/](./17-cpp-learning-notes/) |
| **PNP 实战** | [Computer-Networking/PNP](https://github.com/cshonor/Computer-Networking/tree/main/PNP) | [08-Practical-Network-Programming/](./08-Practical-Network-Programming/) |
| **UNP** | [UNP_Vol1](https://github.com/cshonor/Computer-Networking/tree/main/UNP_Vol1) | [09-UNP-Vol1/](./09-UNP-Vol1/) |

---

## 三、内核网络栈 vs 用户态旁路

```
  11 UNP + CSAPP Ch10–11  │  socket → epoll（标准内核路径）
       ↓
  13 Rosen                │  sk_buff / NAPI / softirq
       ‖ 对照
  14 DPDK                 │  PMD 轮询 / mbuf / 绕过 socket
```

| 对比项 | 内核栈（11 UNP / 13 Rosen） | 用户态旁路（14 DPDK） |
|--------|----------------------------|----------------------|
| 收包触发 | 中断 + NAPI 软中断 | 用户态 busy-poll |
| 缓冲结构 | `sk_buff` | `rte_mbuf` |
| 系统调用 | `recvfrom` / `epoll_wait` | 无（UIO/VFIO） |

**阅读顺序：** `03` LKD → `04` ULK → `05` Gorman → `06` TLPI → `07` 自制 → **`17` C++** → `08` PNP → `11` UNP → `12`–`14` 网络纵深 → `15` HFT → `16` Rust。

---

## 四、DPDK ↔ UNP Socket 模型

| UNP / CSAPP 概念 | DPDK 对应 | 关键差异 |
|------------------|-----------|----------|
| `socket()` | `rte_eth_dev_configure()` + `rte_eth_rx_queue_setup()` | DPDK 直接绑网卡队列 |
| `epoll_wait()` | `while(1) { rx_burst; process; }` | 事件驱动 → 忙等 |
| `recvfrom()` | `rte_eth_rx_burst()` | 批量收包；无 syscall |

→ UNP：[09-UNP-Vol1](./09-UNP-Vol1/) · PNP：[08-Practical-Network-Programming](./08-Practical-Network-Programming/)  
→ DPDK：[12-DPDK-Low-Latency-Network](./12-DPDK-Low-Latency-Network/)

---

## 五、DPDK ↔ CSAPP 缓存与内存

| CSAPP / Hennessy 概念 | DPDK 落地 | HFT 要点 |
|----------------------|-----------|----------|
| 局部性、Cache line | mbuf 连续布局、mempool 同 NUMA | 避免跨 NUMA 取 mbuf |
| TLB / 大页 | EAL `-huge` | DPDK 强制依赖大页 |

→ [12-DPDK chapter-02-mbuf](./12-DPDK-Low-Latency-Network/01-Intro-Book/notes/chapter-02-mbuf与内存池.md)

---

## 六、待填充的动手实验

| 实验 | 目录 | 关联模块 |
|------|------|----------|
| CSAPP Lab | `01-CSAPP-3rd/code/` | Ch3–12 |
| PNP 网络实验 | [外部 PNP/code](https://github.com/cshonor/Computer-Networking/tree/main/PNP/code) | 对照 11 UNP |
| 自制 OS/CPU | `07-system-low-level-hands-on/code/` | 中断、页表 |
| DPDK 组播最小工程 | `12-DPDK/.../mcast-minimal/` | Rosen 组播 |

---

## 七、OpenOnload / RDMA

详见 [note-openonload-rdma对比](./12-DPDK-Low-Latency-Network/02-Advanced-Book/notes/note-openonload-rdma对比.md)
