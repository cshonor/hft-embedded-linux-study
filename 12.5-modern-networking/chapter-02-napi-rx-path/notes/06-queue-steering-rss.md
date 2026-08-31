# 07 — 多队列、RSS 与行情流定向

> **对应 Rosen:** Ch14（RPS/RFS，软分发）
> **内核源码路径:** `Documentation/networking/scaling.rst`

## 文档概述

现代网卡有多个 RX/TX 队列，每个队列对应一个 NAPI 实例和一个中断。
**包落到哪个队列，决定了它在哪个 CPU 上被处理**——这一步直接决定延迟与抖动。
本笔记讲如何用硬件（RSS / ntuple）把行情组播流钉到独占队列。

---

## 核心内容

### 队列 → NAPI → 中断 → CPU 的绑定关系

```
                ┌─ RX queue 0 ─→ napi 0 ─→ IRQ_A ─→ CPU 0
  NIC 多队列 ───┼─ RX queue 1 ─→ napi 1 ─→ IRQ_B ─→ CPU 1
   (RSS hash)   ├─ RX queue 2 ─→ napi 2 ─→ IRQ_C ─→ CPU 2
                └─ RX queue 3 ─→ napi 3 ─→ IRQ_D ─→ CPU 3
```

**一核一队列**是 HFT 的标准部署：队列 N 的中断绑到 CPU N，收包进程也绑到 CPU N，
包从进队列到被处理的全过程不跨核、不跨 NUMA。

---

### RSS：硬件决定包去哪个队列

RSS（Receive Side Scaling）用硬件 hash 分发：

```
hash( src_ip, dst_ip, src_port, dst_port )  →  indirection table  →  queue id
```

```bash
# 查看 indirection table（每队列权重）
ethtool -x eth0

# 把全部流量集中到队列 2
ethtool -X eth0 weight 0 0 1 0

# 自定义 hash 字段（按驱动支持）
ethtool -N eth0 rx-flow-hash udp4 sd   # s=src port, d=dst port
```

**行情组播的 RSS 特性（重要）：**

组播流的四元组（交易所 src IP、组播 dst IP、两个端口）**全部固定**
→ hash 恒定 → **永远落同一个队列**。

| 后果 | 含义 |
|------|------|
| 好的一面 | 完全确定，可预测，能精确规划 |
| 坏的一面 | 该队列成为唯一热点；若上面还跑别的流量 → 队头阻塞，延迟抖动 |
| 更坏的 | **多条行情流可能 hash 到同一队列**，互相抢 |
| 解法 | 不用 RSS 默认 hash，改用 ntuple 显式指定队列 |

---

### ntuple / Flow Steering：显式钉队列

绕过 hash，直接下规则："这个组播组 + 这个端口 → 队列 3"。

```bash
# 把行情组播流钉到队列 3（Intel 叫 Flow Director，mlx5 叫 Flow Steering）
ethtool -U eth0 flow-type udp4 dst-ip 224.1.2.3 dst-port 12345 action 3

# 查看已下发的规则
ethtool -u eth0

# 删除规则
ethtool -U eth0 delete 40      # 40 = 规则 location
```

| 厂商 | 名称 | 备注 |
|------|------|------|
| Intel (ice/i40e/ixgbe) | Flow Director (fdir) | 规则数有限（数千） |
| Mellanox (mlx5) | Flow Steering | 规则数大，支持复杂匹配 |
| Broadcom (bnxt) | NTuple filter | 依网卡型号 |

**典型分流设计（HFT 标准做法）：**

```
队列 0            → SSH / 监控 / 管理流量        → 内核协议栈
队列 1, 2         → 行情组播流 A, B              → AF_XDP / DPDK 独占
队列 3            → 订单链路（TCP/FIX）          → 内核协议栈（要 TCP）
```

关键点：**被 AF_XDP/DPDK 接管的队列，流量不再进内核栈。**
所以必须先把行情流和管理流分开，否则你旁路掉自己 SSH 的同时也旁路了心跳包。

---

### RPS / RFS：HFT 应该关掉

| 机制 | 位置 | HFT 取舍 |
|------|------|---------|
| RSS | 网卡硬件 | ✅ 用，零 CPU 成本 |
| ntuple | 网卡硬件 | ✅ 用，精确可控 |
| **RPS** | 软件（收包后跨 CPU 分发） | ❌ 关：引入 IPI + 排队，只加延迟 |
| **RFS** | 软件（按应用所在 CPU 分发） | ❌ 关：同上，且收益在吞吐不在延迟 |

RPS/RFS 是为"多核吞吐"设计的，HFT 要的是"单核确定性"。
已有硬件队列时开 RPS 是纯负收益。

```bash
# 确认已关闭
cat /sys/class/net/eth0/queues/rx-*/rps_cpus    # 应为 0
```

---

### 中断绑定：irqbalance 是敌人

```bash
# ① 必须关掉 irqbalance —— 它会动态迁移中断，毁掉你的绑核规划
systemctl stop irqbalance
systemctl disable irqbalance

# ② 查看每个队列的中断号
grep -E 'eth0|TxRx|rx-' /proc/interrupts

# ③ 绑到指定 CPU（bitmask，bit3 = CPU3 → 0x8）
echo 8 > /proc/irq/<irq>/smp_affinity

# ④ 或用 smp_affinity_list 更直观
echo 3 > /proc/irq/<irq>/smp_affinity_list
```

**`managed_irq` 坑：** 内核启动参数若不加 `managed_irq`，
受管中断（managed interrupt）仍可能落到你的隔离核上：

```
isolcpus=domain,managed_irq,2-7 nohz_full=2-7 rcu_nocbs=2-7
              ↑ 这个必须加
```

---

### 队列数与 CPU 拓扑

| 约束 | 说明 |
|------|------|
| 队列数 ≤ 隔离核数 | 每队列要一个独占核轮询，核不够就开不了那么多队列 |
| 队列与核同 NUMA | 跨节点访存 +100ns 起，且不可预测 |
| 超线程 | HFT 通常关掉或只用一个兄弟核，避免 SMT 争抢 |

```bash
# 调队列数（需驱动支持，且会短暂断流）
ethtool -l eth0                 # 查看当前/最大
ethtool -L eth0 combined 4      # 设为 4 队列

# NUMA 归属检查
cat /sys/class/net/eth0/device/numa_node
lscpu | grep NUMA
numactl --cpunodebind=0 --membind=0 ./feed_handler
```

---

### 验证：流量到底落在哪个队列

```bash
# 每队列收包计数（确认分流是否生效）
ethtool -S eth0 | grep -E 'rx_queue_[0-9]+_(packets|bytes)'

# 或看软中断分布（每 CPU 的 NET_RX）
watch -n1 'cat /proc/softirqs | grep NET_RX'

# 确认没有丢包
ethtool -S eth0 | grep -Ei 'miss|no_buf|drop'
```

**排障口诀：** 行情收不到 → 先看 `ethtool -S` 里队列 N 的 packets 是否增长
→ 不增长说明流没进这个队列（RSS/ntuple 问题，或交换机 IGMP 问题）
→ 增长但应用收不到，说明是用户态消费慢（ring/buffer 满）。

---

## HFT 要点

- **行情流必须用 ntuple 钉队列**，不能指望默认 RSS hash 帮你分好
- **关掉 irqbalance**，它是绑核规划的头号破坏者
- **关掉 RPS/RFS**，硬件队列够用，软件分发只加抖动
- `isolcpus` 要带 `managed_irq`，否则中断仍会打进隔离核
- 队列规划要预留：行情走旁路队列，SSH/监控/订单链路留内核队列
- **一核一队列**是容量规划的硬约束——核数决定能并行处理多少条行情流

## 与 Rosen 3.x 的差异

| 维度 | Rosen（3.x） | 现代（5.x/6.x） |
|------|-------------|----------------|
| 分发机制 | RPS/RFS（软）为主 | RSS + ntuple 硬件分发为主，RPS 降级为兜底 |
| 队列数 | 单/双队列常见 | 数十队列常态，配合 RSS indirection table |
| 中断绑定 | 手工 `/proc/irq` | 同，但多了 `managed_irq` 与 irqbalance 的冲突 |
| 旁路共存 | 无此概念 | 需按队列划分"内核管"与"旁路独占" |
