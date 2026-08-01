# 跨模块联动指南

> 本仓库 **[hft-embedded-linux-study](https://github.com/cshonor/hft-embedded-linux-study)** · 技术板块 **`00`–`23`** · **编号 = 读序**。  
> **推荐阅读顺序** → [LEARNING-CHAIN.md](./LEARNING-CHAIN.md) · [LEARNING-PATH-LOCKED.md](./LEARNING-PATH-LOCKED.md)

---

## 一、仓库板块总览

| 板块 | 文件夹 | 维度 |
|------|--------|------|
| **硬件底层** | `00` digital-logic-cpu | 组合/时序/CPU 词汇 |
| **语言与系统** | `01` C → `02` computer-systems（`03` 体系结构可后读） | 母语 + 程序=机器 |
| **Linux 用户态** | `04` userspace-api · `05` os-from-scratch · `06` cpp | syscall / 自制 OS / C++ |
| **Linux 内核** | `07` kernel · `09` mm（`08` deep 拓展） | 调度 / VM |
| **嵌入式** | `10`–`14` ARM → 构建 → 驱动/DT → 实战 → 飞控 | 第二职业退路 |
| **网络栈** | `15`–`18` sockets → TCP/IP → 内核网 → DPDK | 报文路径 |
| **性能工具** | `19`–`20` systems-performance → BPF | 观测落地 |
| **HFT 上层** | `21`–`23` hft-engineering · rust-quant · markets | 工程 + 业务 |

### `18` DPDK 与 `21` HFT 的分界

| | `15`–`18` 网络技术栈 | `21` HFT 工程实践 |
|---|----------------------|-------------------|
| **关注点** | 报文收发、协议、网卡、内存收发模型 | 裸机调优、对接规范、系统架构、延迟压测 |
| **产出** | 理解/实现网络路径 | 把技术栈落到交易系统工程 |
| **关系** | 底层能力 | 业务侧整合 |

---

## 二、网络学习链（推荐顺序）

```
00 数字逻辑 → 01 C → 02 计算机系统
    ↓
04 用户态 API → 05 自制 OS → 06 C++
    ↓
07 内核 + 09 MM
    ↓
15 sockets → 16 TCP/IP → 17 内核网络 → 18 DPDK
    ↓
19 SysPerf → 20 BPF → 21 HFT
```

| 轨道 | 外部仓库 | 本仓库索引 |
|------|----------|------------|
| **本手册** | [hft-embedded-linux-study](https://github.com/cshonor/hft-embedded-linux-study) | 读序 · OUTLINE · `00`–`23` |
| **C 语言** | [cpp-learning-notes / C](https://github.com/cshonor/cpp-learning-notes) | [01-c-language/](./01-c-language/) |
| **C++** | [cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes) | [06-cpp/](./06-cpp/) |
| **Socket 实战** | [Computer-Networking](https://github.com/cshonor/Computer-Networking) | [15-network-sockets/](./15-network-sockets/) |

---

## 三、内核网络栈 vs 用户态旁路

```
  15 sockets + 02 Ch10–11  │  socket → epoll（标准内核路径）
       ↓
  17 kernel-networking    │  sk_buff / NAPI / softirq
       ‖ 对照
  18 dpdk                 │  PMD 轮询 / mbuf / 绕过 socket
```

| 对比项 | 内核栈（15 / 17） | 用户态旁路（18 DPDK） |
|--------|-------------------|----------------------|
| 收包触发 | 中断 + NAPI 软中断 | 用户态 busy-poll |
| 缓冲结构 | `sk_buff` | `rte_mbuf` |
| 系统调用 | `recvfrom` / `epoll_wait` | 无（UIO/VFIO） |

---

## 四、嵌入式支线（`10`–`14`）

与 HFT 主线在 Phase4 后分叉；细节见 [HFT-READING-ROADMAP §嵌入式](./HFT-READING-ROADMAP.md#六嵌入式-linux-支线10–14)。
