# Ch 1 §2 网络设备 · The Network Device (`net_device`)

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 2. 网络设备 (The Network Device — `net_device`)

`net_device` 是网络子系统 **最基础的数据结构之一**，代表 **一个网络接口**（物理网卡、bond、vlan、dummy、tun 等）。

源码入口：`include/linux/netdevice.h` · 驱动在 `net_device` 上挂 **ops**（open/stop/xmit/NAPI poll 等）。

---

## 主要字段与含义

| 类别 | 典型成员 | 作用 |
|------|----------|------|
| 标识 | `name`（如 `eth0`） | 用户态 `ip link` / `ioctl` 可见名 |
| 链路 | **MAC 地址**、**MTU**（以太网常见 **1500**） | 帧格式与最大传输单元 |
| 中断 | **IRQ** | 传统 **中断驱动收包** 的入口 |
| 多播 | 多播地址列表 | IGMP/组播过滤（→ [note-组播IGMP](../../note-组播IGMP.md)） |
| 嗅探 | **promiscuity counter** | 混杂模式引用计数；`tcpdump`/BPF 抓包时会递增 |
| 队列 | `tx_queue_len`、qdisc 绑定 |  egress 排队与 **tc** 整形 |

每个 **`net_device`** 对应 `/sys/class/net/<name>/` 与 **rtnetlink** 配置面（→ [Ch 2 Netlink](../../chapter-02-Netlink套接字.md)）。

---

## NAPI (New API)

**问题：** 纯 **中断驱动** — 每个包一次硬中断；**高 PPS** 时中断开销压过 useful work（**receive livelock**）。

**NAPI 思路：**

1. 硬中断 **只做最少工作**：关/限中断、把设备挂到 **poll 列表**、触发 **softirq**。
2. **`net_rx_action`** 在 softirq 里 **轮询 (polling)** 驱动，**批量** 从 ring 拉包进栈。
3. 流量下降后可 **重新开中断**。

```
高负载：interrupt → schedule NAPI → softirq 批量 poll（少中断、多包/次）
低负载：恢复 per-packet interrupt（低延迟唤醒）
```

| | 传统中断模式 | NAPI |
|---|-------------|------|
| 高 PPS | 中断风暴 | **批量 poll**，CPU 更可预测 |
| 低流量 | 低延迟 | 仍可走中断 |
| HFT | 易抖动 | **Ch 14** 细讲 tuning；生产常 **busy poll** / **DPDK 绕过** |

> 现代驱动几乎 **必选 NAPI**；`netif_napi_add()` / `napi_schedule()` 为常见 API（3.9 已有，后续内核接口名微调）。

---

## HFT 要点

| 主题 | 行动 |
|------|------|
| **多队列网卡** | 每 queue 可绑 **独立 NAPI** + **RPS/RSS**（Ch 14） |
| **MTU / 巨帧** | jumbo frame 减 PPS、增 per-packet  payload — 与 **行情 burst** 尺寸匹配 |
| **promisc** | 抓包开混杂；**正常交易路径应关** — 多耗 CPU |
| **旁路** | DPDK **userspace PMD** 不注册标准 `net_device` 数据路径（或 coexist via KNI/virtio） |

---

← [1. Linux 网络栈](./section-1-Linux网络栈.md) · [Ch 1](../README.md) · 下一节 [3. sk_buff](./section-3-套接字缓冲区-sk_buff.md)
