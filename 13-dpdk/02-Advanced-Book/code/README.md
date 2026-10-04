# 02-Advanced-Book · code · 进阶对照实验

> **目标：** 同一台机器、同一份 UDP 组播流量、同一套延迟直方图，
> 只换"收包路线"这一个变量，把 [notes](../README.md) 里的延迟量级表变成**自己的实测数据**。

---

## 同口径测法（关键约束）

| 约束 | 原因 |
|---|---|
| 同一台机器、同一张网卡、同一队列 | 路线间数字才可比 |
| 同一份组播源（发送端固定速率/包长） | 排除流量差异 |
| 同一直方图口径：`hist_parse` / `hist_burst` / `hist_gap` | 复用 [mcast-minimal](../../01-Intro-Book/code/mcast-minimal/) 的定义，见 [chapter-03-PMD与轮询模式](../../01-Intro-Book/notes/chapter-03-PMD与轮询模式.md) 第二节 |
| 同样的核隔离（isolcpus/nohz_full/rcu_nocbs） | 内核栈路线也要给最好条件，结论才公平 |
| 记录 p50 / p99 / p999 | 均值无意义，看分布 |

---

## 实验矩阵

| # | 实验 | 路线 | 状态 |
|---|------|------|------|
| E0 | [mcast-minimal](../../01-Intro-Book/code/mcast-minimal/)（DPDK 版，已有） | DPDK | ✅ 已存在 |
| E1 | [mcast-minimal/src/mcast_socket_ref.c](../../01-Intro-Book/code/mcast-minimal/) — 同口径内核栈对照版（已有） | 内核栈 | ✅ 已存在 |
| E2 | `afxdp-recv/` — XDP_REDIRECT → AF_XDP（zc）收同款流 | AF_XDP | ⬜ 待补充 |
| E3 | Onload 版（`onload ./E1`，代码零改动） | OpenOnload | ⬜ 待补充（需 Solarflare 系网卡） |
| E4 | RDMA WRITE 推送（需 RoCE 环境） | RDMA | ⬜ 待补充 |

E1 是基线：先跑出内核栈的 p999，后面每个实验都是"相对 E1 快了多少"。
E3/E4 依赖硬件，没有对应网卡时标注 N/A，不要用软件模拟凑数。

---

## 结果回填

每完成一个实验，把 p50/p999 连同**测量环境**（CPU 型号/频率、网卡、内核版本、
isolcpus 配置、BURST_SIZE）记录在下表，并回填到
[note-openonload-rdma对比](../notes/note-openonload-rdma对比.md) 第一节的概览表旁：

| 路线 | p50 | p999 | 环境 | 日期 |
|------|-----|------|------|------|
| 内核栈（E1） | 2624–3136ns（recvmmsg/burst，v=1） | 6.4μs（v=1）| Core 7 240H / lo 自发自收 / 无隔离（[R1](../../01-Intro-Book/code/mcast-minimal/RESULTS.md)） | 2026-10-04 |
| AF_XDP zc（E2） | — | — | — | — |
| DPDK（E0） | — | — | — | — |
| Onload（E3） | — | — | — | — |

> R1 是在 loopback 上跑的**下界数据**（无 NIC/PCIe 路径），发送端 ~128k pps 未压满批次；
> 物理网卡 + pktgen 环境复测后应更新本行而非追加。

> 没有环境标注的数字等于没测——CPU 差一代，绝对值可以差一倍。
