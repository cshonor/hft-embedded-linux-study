# Ch 14 §3 忙轮询与收包路径 · Busy Poll & RX Scaling

> **Linux Kernel Networking** · Rami Rosen · **精读 🔴**

### 3. 忙轮询套接字 (Busy Poll Sockets)

**Linux 3.11+** — **`SO_BUSY_POLL` / `SO_BUSY_POLL_BUDGET`**：有数据待收时，用户线程 **spin 轮询 NIC 驱动**，**绕过中断唤醒延迟** — **以 CPU 换 latency**。

---

## 传统 vs Busy Poll

```
传统:
  包到达 → 硬中断 → NAPI/softirq → sk_data_ready → epoll 唤醒 → read

Busy Poll:
  epoll_wait 返回后 → sock busy poll → 驱动 poll 一次 → 直接 copy 数据
  （减少: 中断 + softirq + schedule 延迟）
```

| 调优 | 说明 |
|------|------|
| **`net.core.busy_poll`** | 全局默认 **微秒预算** |
| **`net.core.busy_read`** | low latency read |
| **`SO_BUSY_POLL`** | per-socket |
| **需驱动支持** | `poll` napi 或 **ndo_busy_poll** |

**HFT：** 与 **kernel bypass (DPDK)**、**Onload** 同谱系 — **共置仍用内核栈时** 值得 A/B。

---

## NAPI 与 softirq（收包主干）

> 详 [Ch 1 §2 net_device](../../chapter-01-introduction/notes/section-2-网络设备-net_device.md)

```
硬中断 (minimal)
  → napi_schedule
  → NET_RX_SOFTIRQ: net_rx_action
       → 驱动 napi->poll() 批量 dequeue ring
       → netif_receive_skb → GRO → L3/L4
```

| 参数 | 影响 |
|------|------|
| **`netdev_budget` / `netdev_budget_usecs`** | 单次 softirq **处理包数/时间** — 过大 **饿死用户态** |
| **`netdev_max_backlog`** | 输入队列满 → **drop** |

---

## 多队列扩展：RSS / RPS / RFS / XPS

| 技术 | 层级 | 作用 |
|------|------|------|
| **RSS** | **硬件** | 哈希 **五元组 → 多 RX 队列** — 多核并行 |
| **RPS** | **软件** | 单队列 NIC：**软分发** 到 CPU |
| **RFS** | **软件** | RPS + **跟踪应用 CPU** — 包到 **处理它的核** |
| **XPS** | **发送** | **指定 CPU → TX 队列** — 减 **跨核锁** |

```bash
# 概念示例
echo f > /sys/class/net/eth0/queues/rx-0/rps_cpus
ethtool -X eth0 equal 4   # RSS 队列
```

**HFT checklist：**

| 项 | 目标 |
|----|------|
| **IRQ affinity** | RX queue **绑 tick 核** |
| **RFS + SO_REUSEPORT** | [Ch 11 §1](../../chapter-11-layer-4-protocols/notes/section-1-套接字.md) 多 fd |
| **XPS** | 发单线程 **绑 TX queue** |
| **disable GRO/LRO** | 极端 latency 测试 |

---

## 延迟工具箱对照

| 手段 | CPU | 典型场景 |
|------|-----|----------|
| **中断 + NAPI** | 低 | 通用 |
| **SO_BUSY_POLL** | 高 | 内核栈 **极致 latency** |
| **DPDK poll mode** | 高 | **旁路内核** |
| **RDMA poll CQ** | 高 | [Ch 13](../chapter-13-infiniband/) |

---

← [2. Cgroups](./section-2-控制组Cgroups.md) · [Ch 14](../README.md) · 下一节 [4. 蓝牙](./section-4-Linux蓝牙子系统.md)
