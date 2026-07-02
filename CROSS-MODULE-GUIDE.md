# 跨模块联动指南

> 本仓库 **[hft-embedded-linux-study](https://github.com/cshonor/hft-embedded-linux-study)** · 技术板块 **`00`–`18` 主线 + 嵌入式 `19`–`24`**。  
> **推荐阅读顺序** → [LEARNING-CHAIN.md](./LEARNING-CHAIN.md)

---

## 一、仓库板块总览

| 板块 | 文件夹 | 维度 |
|------|--------|------|
| 交易金融理论 | `00` | 业务 / LOB |
| **程序与硬件** | **`01` CSAPP → `02` C → `03` Hennessy** | 知其所以然 + 系统级 C |
| Linux 内核 | **`04` LKD → `05` ULK → `06` Gorman** | 调度 / 源码 / VM |
| **Linux 用户态** | `07` TLPI | syscall · epoll · mmap · 线程 |
| **系统底层动手** | `08` | **01 MikanOS**（主线）· 02 30天（可选） |
| **C++ 语言** | **`09`** [cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes) | Modern C++ · 并发 · 对象模型（**08 OS 后、10 PNP 前**） |
| **C++ 网络实战** | `10` PNP / muduo | Reactor 实验骨架 |
| **网络完整栈** | `11`–`14` | UNP → TCP/IP → Rosen → DPDK |
| **性能（后置）** | **`15` SysPerf → `16` BPF** | 14 DPDK 之后、17 HFT 之前 |
| **低延迟工程** | `17` HFT Practice | C++ 整机实践 |
| **Rust 量化** | `18` Rust Guide | Rust 工程 |

### `14` 与网络板块（`10`–`13`）的分界

| | `10`–`13` 网络技术栈 | `17` HFT 工程实践 |
|---|--------------------------|-------------------|
| **关注点** | 报文收发、协议、网卡、内存收发模型 | 裸机调优、对接规范、系统架构、延迟压测 |
| **产出** | 理解/实现网络路径 | 把技术栈落到交易系统工程 |
| **关系** | 底层能力 | 业务侧整合 |

---

## 二、网络学习链（推荐顺序）

```
01 CSAPP → 02 C → 03 Hennessy
    ↓
04 LKD → 05 ULK → 06 Gorman → 07 TLPI（epoll / mmap / 调度）
    ↓
08/01 MikanOS（HFT 主线）
    ↓
09 C++（至少 M1 Modern C++）
    ↓
10 陈硕 PNP / muduo
    ↓
11 UNP + CSAPP Ch10–11
    ↓
12 TCP/IP → 13 Rosen → 14 DPDK
    ↓
15 SysPerf → 16 BPF（后置 · 观测落地）
```

| 轨道 | 外部仓库 | 本仓库索引 |
|------|----------|------------|
| **本手册** | [hft-embedded-linux-study](https://github.com/cshonor/hft-embedded-linux-study) | 读序 · OUTLINE · `00`–`24` scaffold |
| **C 语言 5 书** | [cpp-learning-notes / 11-C](https://github.com/cshonor/cpp-learning-notes/tree/main/11-Linux-Kernel-DPDK-Network-C) | [02-c-programming/](./02-c-programming/) |
| **C++ 9 书 + 可选** | [cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes) | [09-cpp-learning-notes/](./09-cpp-learning-notes/) |
| **PNP 实战** | [Computer-Networking/PNP](https://github.com/cshonor/Computer-Networking/tree/main/PNP) | [10-Practical-Network-Programming/](./10-Practical-Network-Programming/) |
| **UNP** | [UNP_Vol1](https://github.com/cshonor/Computer-Networking/tree/main/UNP_Vol1) | [11-UNP-Vol1/](./11-UNP-Vol1/) |

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

**阅读顺序：** `01` CSAPP → **`02` C** → `03` Hennessy → `04` LKD → `05` ULK → `06` Gorman → `07` TLPI → `08` OS → **`09` C++** → `10` PNP → `11` UNP → `12`–`14` 网络纵深 → `15`–`16` 性能 → `17` HFT → `18` Rust。

---

## 四、DPDK ↔ UNP Socket 模型

| UNP / CSAPP 概念 | DPDK 对应 | 关键差异 |
|------------------|-----------|----------|
| `socket()` | `rte_eth_dev_configure()` + `rte_eth_rx_queue_setup()` | DPDK 直接绑网卡队列 |
| `epoll_wait()` | `while(1) { rx_burst; process; }` | 事件驱动 → 忙等 |
| `recvfrom()` | `rte_eth_rx_burst()` | 批量收包；无 syscall |

→ UNP：[11-UNP-Vol1](./11-UNP-Vol1/) · PNP：[10-Practical-Network-Programming](./10-Practical-Network-Programming/)  
→ DPDK：[14-DPDK-Low-Latency-Network](./14-DPDK-Low-Latency-Network/)

---

## 五、DPDK ↔ CSAPP 缓存与内存

| CSAPP / Hennessy 概念 | DPDK 落地 | HFT 要点 |
|----------------------|-----------|----------|
| 局部性、Cache line | mbuf 连续布局、mempool 同 NUMA | 避免跨 NUMA 取 mbuf |
| TLB / 大页 | EAL `-huge` | DPDK 强制依赖大页 |

→ [13-DPDK chapter-02-mbuf](./14-DPDK-Low-Latency-Network/01-Intro-Book/notes/chapter-02-mbuf与内存池.md)

---

## 六、待填充的动手实验

| 实验 | 目录 | 关联模块 |
|------|------|----------|
| CSAPP Lab | `01-CSAPP-3rd/code/` | Ch3–12 |
| C 系统编程 | [02-c-programming/](./02-c-programming/) | K&R · Pointers on C |
| PNP 网络实验 | [外部 PNP/code](https://github.com/cshonor/Computer-Networking/tree/main/PNP/code) | 对照 11 UNP |
| 自制 OS/CPU | `08-system-low-level-hands-on/code/` | 中断、页表 |
| DPDK 组播最小工程 | `14-DPDK/.../mcast-minimal/` | Rosen 组播 |

---

## 七、OpenOnload / RDMA

详见 [note-openonload-rdma对比](./14-DPDK-Low-Latency-Network/02-Advanced-Book/notes/note-openonload-rdma对比.md)
