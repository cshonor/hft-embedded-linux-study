# 跨模块联动指南

> 本仓库 **技术板块已封顶**（`00`–`12`），不再新增顶层编号文件夹。  
> 剩余工作是在各目录内填充笔记、代码实验，以及通过本文档打通模块边界。

---

## 一、仓库板块总览

| 板块 | 文件夹 | 维度 |
|------|--------|------|
| 交易金融理论 | `00` Trading and Exchanges | 业务 / LOB / 市场结构 |
| **硬件 CPU 层** | **`07` Hennessy · `08` CSAPP** | **缓存 / 进程 / VM / 锁 — 读 SysPerf 前的地基** |
| Linux 系统层 | `01` SysPerf · **`02` LKD（课+书）** · `03` Gorman | 性能方法论 / 调度 / 虚拟内存（内核视角） |
| **网络完整栈** | `04` TCP/IP · `05` UNP · `06` Rosen · **`12` DPDK** | 协议 → Socket → 内核栈 → 用户态旁路 |
| 性能观测 | `09` BPF Tools | eBPF / XDP 生产排障 |
| **低延迟工程落地** | `10` HFT Practice | 整机调优 / 交易所对接 / 交易系统架构 / 压测 |
| Rust 开发栈 | `11` Rust Quant Guide | 量化工程 / 无锁 / 订单簿 |

> **阅读顺序 ≠ 文件夹序号。** HFT 推荐：`07`+`08` 地基 → `01` SysPerf → **`09` BPF（紧接）** → `02`/`03` → 网络栈。详见 [HFT-READING-ROADMAP.md](./HFT-READING-ROADMAP.md)。

### `10` 与网络板块（`04/05/06/12`）的分界

| | `04/05/06/12` 网络技术栈 | `10` HFT 工程实践 |
|---|--------------------------|-------------------|
| **关注点** | 报文收发、协议、网卡、内存收发模型 | 裸机调优、对接规范、系统架构、延迟压测 |
| **产出** | 理解/实现网络路径 | 把技术栈落到交易系统工程 |
| **关系** | 底层能力 | 业务侧整合；内含 C++ HFT 案例仅为工程参考 |

二者**互不冲突**，不存在「为 10 再补网络书」的问题。

---

## 二、内核网络栈 vs 用户态旁路

两条**并行路线**，互为补充，不是重复内容。

```
                    ┌─────────────────────────────────────┐
  04 TCP/IP         │  协议语义：UDP/TCP/IGMP/组播        │
       ↓            └─────────────────────────────────────┘
                    ┌─────────────────────────────────────┐
  05 UNP + CSAPP11  │  标准内核路径：socket → epoll       │
       ↓            │  系统调用、内核协议栈、NAPI/softirq   │
  06 Rosen          └─────────────────────────────────────┘
                           ‖ 对照
                    ┌─────────────────────────────────────┐
  12 DPDK           │  用户态旁路：PMD 轮询、mbuf、mempool │
                    │  绕过 sk_buff / socket 层           │
                    └─────────────────────────────────────┘
```

| 对比项 | 内核栈（05 UNP / 06 Rosen） | 用户态旁路（12 DPDK） |
|--------|----------------------------|----------------------|
| 收包触发 | 中断 + NAPI 软中断 | 用户态 busy-poll |
| 缓冲结构 | `sk_buff` | `rte_mbuf` |
| 内存分配 | 内核 slab / 页分配 | 预分配 mempool（大页） |
| 系统调用 | `recvfrom` / `epoll_wait` | 无（映射 UIO/VFIO） |
| 适用场景 | 通用、开发快、TCP 友好 | 极致延迟、UDP 组播行情 |
| 观测工具 | **09 BPF Ch10** + XDP note、SysPerf Ch10 | testpmd、BPF（对比用） |

**阅读顺序：** 先走通 04 → 05 → 06，再读 12 理解「绕过了什么」。

---

## 三、DPDK ↔ UNP Socket 模型

| UNP / CSAPP 概念 | DPDK 对应 | 关键差异 |
|------------------|-----------|----------|
| `socket()` | `rte_eth_dev_configure()` + `rte_eth_rx_queue_setup()` | DPDK 直接绑网卡队列，不经 socket 层 |
| `bind()` / `connect()` | 无；L2/L3 解析在用户态完成 | 行情组播用 `rte_eth_dev` + 自解析 UDP |
| `recvfrom()` | `rte_eth_rx_burst()` | 批量收包；无 syscall |
| `epoll_wait()` | 轮询循环 `while(1) { rx_burst; process; }` | 事件驱动 → 忙等 |
| `setsockopt(SO_RCVBUF)` | mempool 大小、`RX_RING_SIZE` | 预分配、无动态扩缩 |
| `sendto()` | `rte_eth_tx_burst()` | 同理批量发包 |

→ UNP 细节：[05-UNP-Vol1](./05-UNP-Vol1/) · CSAPP：[chapter-11](./08-CSAPP-3rd/chapter-11-网络编程.md)  
→ DPDK 细节：[12-DPDK-Low-Latency-Network](./12-DPDK-Low-Latency-Network/)

---

## 四、DPDK ↔ CSAPP 缓存与内存

| CSAPP / Hennessy 概念 | DPDK 落地 | HFT 要点 |
|----------------------|-----------|----------|
| 局部性、Cache line（CSAPP Ch6） | mbuf 连续布局、mempool 同 NUMA 节点 | 避免跨 NUMA 取 mbuf |
| 伪共享（Hennessy Ch2） | 每核独立 RX queue、mempool 按 lcore 分 | 多队列网卡 + 绑核 |
| TLB / 大页（CSAPP Ch9、Gorman） | EAL `-huge`、1GB/2MB 大页 | DPDK 强制依赖大页 |
| 预分配 vs malloc | mempool 启动时一次性分配 | 热路径零分配 |
| Roofline（Hennessy Ch1） | `rte_eth_rx_burst` 批量大小调优 | 平衡 cache 与延迟 |

→ CSAPP：[chapter-06](./08-CSAPP-3rd/chapter-06-存储器层次结构.md)、[chapter-09](./08-CSAPP-3rd/chapter-09-虚拟内存.md)  
→ DPDK：[chapter-02-mbuf与内存池](./12-DPDK-Low-Latency-Network/chapter-02-mbuf与内存池.md)

---

## 五、待填充的动手实验（均在现有目录内）

| 实验 | 目录 | 关联模块 |
|------|------|----------|
| CSAPP 五大 Lab | `08-CSAPP-3rd/code/` | Ch3–12 程序员视角 |
| UNP Socket Demo | `05-UNP-Vol1/code/` 或外部仓库 | epoll / 非阻塞 / UDP |
| Rust 无锁订单簿 | `11-Rust-Quant-Trading-Guide/code/` | Hennessy Ch5 + CSAPP Ch12 |
| DPDK 组播最小工程 | `12-DPDK-Low-Latency-Network/code/mcast-minimal/` | Rosen 组播 + DPDK Ch5 |

---

## 六、OpenOnload / RDMA（轻量化，不建新文件夹）

详见 [12-DPDK/note-openonload-rdma对比.md](./12-DPDK-Low-Latency-Network/note-openonload-rdma对比.md)：

- **OpenOnload：** 内核旁路但保留 Socket API 语义 — 介于 UNP 与 DPDK 之间
- **RDMA/RoCE：** 共置/托管超低延迟 — 与 DPDK 场景部分重叠，部署模型不同

官方参考：[RDMA 规范](https://www.infinibandta.org/) · [Linux RDMA 文档](https://www.kernel.org/doc/html/latest/infiniband/)
