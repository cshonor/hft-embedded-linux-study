# OpenOnload / RDMA 与 DPDK 对比

> **02-Advanced-Book** · 《Linux 高性能网络详解》配套 · **选读**

## 路线概览

| 路线 | 典型产品/技术 | API 语义 | 旁路程度 | 端到端延迟量级 |
|------|--------------|----------|----------|----------------|
| 标准内核栈 | UNP socket + Rosen 内核栈 | `socket` / `epoll` | 无 | 10–50μs（默认）→ 2–5μs（NAPI defer） |
| 内核旁路 + Socket 兼容 | **OpenOnload** | 保留 BSD socket API | 部分旁路 | ~1–5μs |
| 内 hook + 用户态收包 | **AF_XDP（zero-copy）** | ring + UMEM | 按流旁路 | ~0.5–2μs |
| 用户态旁路 | **DPDK** | `rte_eth_*` / mbuf | 完全旁路 | ~0.3–1μs |
| 硬件 RDMA | **RoCE / InfiniBand** | ibverbs / rdma_cm | 内核/用户态可选 | ~0.5–2μs（单边 ~1μs 内） |

延迟量级只是**同一台机器上可互相比较的相对刻度**，不是绝对值：
取决于网卡、CPU 代际、内核版本、是否 isolcpus、包大小。
**真正可信的数字只有你自己按
[延迟测量方法](../../../12.5-modern-networking/chapter-15-debugging-perf-tuning/notes/03-latency-measurement.md)
实测出来的那一组。**

## 何时选什么

| 场景 | 倾向 | 理由 |
|------|------|------|
| 开发迭代、TCP 订单通道 | 内核栈（05 UNP）或 OpenOnload | 要 socket 语义和 TCP 状态机 |
| 已有系统渐进提速 | **AF_XDP（zc）** | 可只旁路一条流，可回退，不动基础设施 |
| UDP 组播行情、微秒级 | DPDK（本文件夹） | 要最后那点确定性 |
| 共置/托管、纳秒级共址 | RDMA/RoCE | 传输层由硬件完成，主机侧完全不过网络栈 |

**OpenOnload 的定位值得单独说**：它把旁路藏在了 BSD socket API 后面，
老代码不用改就能提速。代价是**你失去了对底层的可见性**——
出问题只能靠厂商工具，而且它主要服务 Solarflare / Xilinx 网卡。
HFT 里它的典型位置是**订单通道**（要 TCP），而不是行情通道（要 UDP 组播）。

## 官方参考

- OpenOnload：https://www.openonload.org/
- RDMA 规范：https://www.infinibandta.org/
- Linux RDMA：https://www.kernel.org/doc/html/latest/infiniband/

## 相关章节

- [note-DPDK实体书递进](../../01-Intro-Book/notes/note-DPDK实体书递进.md) — ② 本书与 DPDK/RDMA/XDP 全书地图
- 上一梯度：[01-Intro chapter-05](../../01-Intro-Book/notes/chapter-05-组播行情接入.md)
- XDP 对照：[note-XDP与DPDK对照](./note-XDP与DPDK对照.md)
- 跨模块：[README 跨模块对照](../../../README.md)
