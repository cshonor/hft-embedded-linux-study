# DPDK — 主题目录（基于官方 Programmer's Guide）

> 无实体书；章节按 DPDK 官方文档主题拆分，序号仅作本仓库阅读顺序。

| 标签 | HFT 读法 |
|------|----------|
| 🔴 | 精读 |
| 🟡 | 选读 |
| ⚪ | 跳过 |

## 核心笔记

| # | 主题 | 笔记 | HFT |
|---|------|------|-----|
| 1 | EAL、环境初始化、大页、NUMA | [chapter-01](./chapter-01-DPDK架构与EAL.md) | 🔴 |
| 2 | mbuf、mempool、ring | [chapter-02](./chapter-02-mbuf与内存池.md) | 🔴 |
| 3 | PMD、poll mode、队列绑定 | [chapter-03](./chapter-03-PMD与轮询模式.md) | 🔴 |
| 4 | 零拷贝、UIO/VFIO、旁路原理 | [chapter-04](./chapter-04-零拷贝与用户态旁路.md) | 🔴 |
| 5 | UDP 组播、行情接入模式 | [chapter-05](./chapter-05-组播行情接入.md) | 🔴 |

## 轻量化配套（不另建文件夹）

| 主题 | 笔记 | HFT |
|------|------|-----|
| OpenOnload / RDMA / RoCE 与 DPDK 取舍 | [note-openonload-rdma对比](./note-openonload-rdma对比.md) | 🟡 |
| **实体书：** 深入浅出 DPDK → Linux 高性能网络详解 | [note-DPDK实体书递进](./note-DPDK实体书递进.md) | 🟡 |

## 官方文档选读（无单独笔记文件）

| DPDK 官方主题 | 标签 | 说明 |
|---------------|------|------|
| l2fwd / testpmd 示例 | 🔴 | 基准与转发参考 |
| rte_ethdev / rte_mbuf API | 🔴 | 收发包核心 API |
| Multi-process / hotplug | ⚪ | 除非多进程架构 |
| Crypto / Eventdev | ⚪ | 非行情热路径 |

## 建议阅读顺序

```
09 Rosen（内核栈收包路径）
    ↓
① 《深入浅出 DPDK》 ∥  chapter-01–04 + 官方 Programmer's Guide
    ↓
chapter-05 + code/mcast-minimal（组播落地）
    ↓
② 《Linux 高性能网络详解》 ∥  note-openonload-rdma对比 + 03-BPF XDP note
```

> **何时开读：** `01` CSAPP + `02` SysPerf 打底，且 perf 已能定位「网络收发是瓶颈」后再上。详见 [note-DPDK实体书递进](./note-DPDK实体书递进.md)。

跨模块对照 → [CROSS-MODULE-GUIDE.md](../CROSS-MODULE-GUIDE.md)
