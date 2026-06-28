# 第5章 操作系统内核极致调优

> **Ch4 原理的 Linux 落地** · CPU 隔离 · Kernel Bypass · Huge Pages

← 原理：[chapter-04 硬件到 OS](./chapter-04-硬件选型与服务器配置.md) · 总览：[chapter-01 §2](./chapter-01-高频交易基础与生态.md#2-硬件与操作系统优化)

---

## 1. CPU Pinning 与隔离

**上下文切换**对 HFT 是致命延迟源。

| 手段 | 说明 |
|------|------|
| **`taskset` / `pthread_setaffinity`** | 线程绑 **物理核** |
| **`isolcpus=`**（内核参数） | 指定核 **不参与通用调度** |
| **`IRQ affinity`** | 网卡中断与 **收包线程同 NUMA、异核** 或 **轮询模式** |

**实践：** Gateway / Book / Strategy **各占独占核**；避免超线程 **兄弟核** 共享执行单元。

→ [08-TLPI 调度](../08-The-Linux-Programming-Interface/)

---

## 2. BIOS / 电源：降低 Jitter

| 禁用 | 原因 |
|------|------|
| **Hyper-Threading** | 两线程争用，延迟波动 |
| **C-states / P-states** | 唤醒 **μs 级** 惩罚 |
| **Intel Turbo Boost** | 频率跳变 → **非确定性** |

---

## 3. Kernel Bypass

传统路径：`recv()` → 内核协议栈 → 拷贝 → 用户缓冲。

| 技术 | 效果 |
|------|------|
| **OpenOnload**（Solarflare） | 用户态 **直接轮询 NIC ring** |
| **DPDK** | PMD 轮询 · **零拷贝 mbuf** |
| **延迟量级** | 约 **1.5–10 μs → 0.5–2 μs**（视配置） |

→ 原理：[chapter-06 动态网络 §5](./chapter-06-低延迟网络与协议优化.md#5-数据包生命周期kernel-路径) · [14-DPDK](../14-DPDK-Low-Latency-Network/) · [chapter-06](./chapter-06-低延迟网络与协议优化.md)

---

## 4. 内存：Pool + Huge Pages

### Memory Pool

- 启动时 **预分配** 订单、Book level、消息节点
- 热点 **allocate/free** 改为 **index stack** 或 **bump pointer**

### Huge Pages

```bash
# 示例：挂载 hugetlbfs，应用 mmap MAP_HUGETLB
echo 1024 > /proc/sys/vm/nr_hugepages
```

| 收益 | 大页减少 **TLB miss** — 对 **随机访 Book** 显著 |

→ [chapter-07 内存布局](./chapter-07-无锁数据结构与内存布局.md) · [09 MikanOS Ch27 Demand/CoW](../09-system-low-level-hands-on/02-mikan-os/chapter-27-app-memory/)
