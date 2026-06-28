# Ch 14 §2 控制组 Cgroups · Cgroups

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 2. 控制组 (Cgroups)

**Cgroups** — **资源限制/隔离**（CPU、内存、**网络**）；与 netns **正交** — 可组合。

---

## 网络相关 cgroup 模块

### `net_prio`

| | |
|---|---|
| **作用** | 按 cgroup 设 **SKB 优先级**（`sk->priority`） |
| **接口** | `net.prio.ifpriomap` — **接口 + 优先级映射** |
| **用途** | **相对优先** — 管理流量 vs 数据流量 |

### `cls_cgroup`

| | |
|---|---|
| **作用** | **分类器** — 给包打 **`classid`** |
| **配合** | **`tc`** 流量控制 — **HTB/FQ** 等 qdisc |
| **路径** | egress **排队/限速/标记** |

```bash
# 概念：cgroup classid → tc filter
tc filter add dev eth0 parent 1: protocol ip handle 1: cgroup
```

---

## 与 netns 对比

| | **netns** | **cgroup (网络)** |
|---|-----------|-------------------|
| 隔离对象 | **整栈副本** | **进程组资源/标记** |
| HFT 共置 | 少用 | **可选** — 防 **非交易进程** 抢带宽 |

---

## HFT

**裸金属低延迟** 常 **不用 tc  shaping**（增 **qdisc 延迟**）。  
**net_prio/cls_cgroup** 适合 **同机多服务** 时 **保护 tick 队列** — 不如 **CPU isolation + 专用 NIC queue** 直接。

→ 多队列/XPS：[§3](./section-3-忙轮询套接字与收包路径.md)

---

← [1. netns](./section-1-网络命名空间.md) · [Ch 14](../README.md) · 下一节 [3. Busy Poll](./section-3-忙轮询套接字与收包路径.md)
