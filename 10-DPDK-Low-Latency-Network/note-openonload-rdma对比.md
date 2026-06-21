# OpenOnload / RDMA 与 DPDK 对比

> **10-DPDK** · 轻量化配套 · **选读**

<!-- 笔记待补充：三种低延迟网络路线的取舍，不引入新实体书、不建新文件夹 -->

## 路线概览

| 路线 | 典型产品/技术 | API 语义 | 旁路程度 |
|------|--------------|----------|----------|
| 标准内核栈 | UNP socket + Rosen 内核栈 | `socket` / `epoll` | 无 |
| 内核旁路 + Socket 兼容 | **OpenOnload** | 保留 BSD socket API | 部分旁路 |
| 用户态旁路 | **DPDK** | `rte_eth_*` / mbuf | 完全旁路 |
| 硬件 RDMA | **RoCE / InfiniBand** | ibverbs / rdma_cm | 内核/用户态可选 |

## 何时选什么

| 场景 | 倾向 |
|------|------|
| 开发迭代、TCP 订单通道 | 内核栈（05 UNP）或 OpenOnload |
| UDP 组播行情、微秒级 | DPDK（本文件夹） |
| 共置/托管、纳秒级共址 | RDMA/RoCE（官方规范） |

## 官方参考

- OpenOnload：https://www.openonload.org/
- RDMA 规范：https://www.infinibandta.org/
- Linux RDMA：https://www.kernel.org/doc/html/latest/infiniband/

## 相关章节

- 上一章：[chapter-05-组播行情接入.md](./chapter-05-组播行情接入.md)
- 跨模块：[CROSS-MODULE-GUIDE §六](../CROSS-MODULE-GUIDE.md#六openonload--rdma轻量化不建新文件夹)
