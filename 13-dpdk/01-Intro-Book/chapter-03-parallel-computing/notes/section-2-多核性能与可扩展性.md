## 2. 多核性能与可扩展性

---

### 一、Amdahl vs Gustafson

| 定律 | 关注点 | 结论 |
|------|--------|------|
| **Amdahl** | **时延 / 加速比** | 串行部分 **封顶** 加速 — 优化 **关键路径** |
| **Gustafson** | **吞吐量** | 核数 ↑ 时 **放大并行部分** — 包处理 **更适用** |

DPDK 目标：**吞吐随核数线性增长** — 靠 **资源局部化、少跨核共享、小临界区**。

**HFT 视角：** Amdahl 更适用于 HFT — tick-to-trade 延迟取决于 **串行关键路径**，加核不能缩短串行部分。但吞吐侧（多策略并行、多行情源）可从 Gustafson 受益。

→ [Ch1 水平扩展](../../chapter-01-dpdk-intro/notes/section-4-底层方法论.md)

---

### 二、DPDK 多核架构

```
┌─────────────────────────────────────────┐
│            DPDK 应用进程                  │
│  ┌──────┐  ┌──────┐  ┌──────┐          │
│  │lcore0│  │lcore1│  │lcore2│  ...      │
│  │ RX   │  │ RX   │  │worker│           │
│  │ TX   │  │ TX   │  │      │           │
│  └──┬───┘  └──┬───┘  └──┬───┘          │
│     │         │         │               │
│  ┌──┴─────────┴─────────┴──┐           │
│  │    rte_ring (无锁队列)    │           │
│  │  producer → consumer     │           │
│  └─────────────────────────┘           │
├─────────────────────────────────────────┤
│  ┌─────┐  ┌─────┐  ┌─────┐             │
│  │RXq0 │  │RXq1 │  │RXq2 │  网卡多队列  │
│  └─────┘  └─────┘  └─────┘             │
└─────────────────────────────────────────┘
```

**RSS（Receive Side Scaling）** — 网卡硬件将流入包按 hash（五元组）分发到不同 RX 队列，每个 lcore 轮询自己的队列，无锁无共享：

```c
/* 配置 RSS — 网卡硬件分发 */
struct rte_eth_rss_conf rss_conf = {
    .rss_key = NULL,                    /* 使用默认 RSS key */
    .rss_key_len = 0,
    .rss_hf = ETH_RSS_IP | ETH_RSS_UDP, /* 按 IP+UDP 五元组 hash */
};
port_conf.rx_adv_conf.rss_conf = rss_conf;

/* 每个 lcore 绑定一个 RX 队列 */
unsigned lcore_id = rte_lcore_id();
unsigned queue_id = lcore_id % nb_queues;
rte_eth_rx_queue_setup(port_id, queue_id, RX_DESC, socket_id, NULL, pool);
```

---

### 三、超线程 (Hyper-Threading)

| 特点 | 对 DPDK 的含义 |
|------|----------------|
| 1 物理核 → 2 **逻辑线程**，共享流水线与 Cache | I/O 密集、**IPC 要求低于计算密集** |
| 轮询 + 等内存时，另一逻辑线程可利用 **空闲流水线** | 可能 **小幅** 提升；也可能 **争用** 执行单元 — **需实测** |
| L1/L2 cache 被两个线程平分 | 实际可用 cache 减半 |

**HFT 常见做法：** 热路径 **独占物理核**（`isolcpus`、关 HT 绑核）— 求 **确定性** 而非峰值吞吐。超线程的 cache 争用会引入 **尾延迟尖刺**。

```bash
# 内核参数：隔离热路径核 + 关闭超线程（BIOS）
isolcpus=2,3,4,5 nohz_full=2,3,4,5 rcu_nocbs=2,3,4,5

# 或保留 HT 但 DPDK 只绑物理核
./my_app -l 2,4,6,8   # 只绑偶数核（每核的第一个 HT 线程）
```

---

### 四、cgroup 与 pthread

- DPDK lcore 底层是 **普通 pthread** — `rte_eal_remote_launch()` 内部调 `pthread_create()`
- 可用 **cgroup** 限制/分配 **CPU 配额** — 改善 I/O 核 **闲置** 与混部场景资源隔离

生产共置：DPDK 核 **cpuset 隔离** + cgroup **防 noisy neighbor**。

```bash
# 将 DPDK 进程固定到特定核
taskset -c 2,3,4,5 ./my_app -l 2,3,4,5 ...

# cgroup v2 限制
echo "+cpuset" > /sys/fs/cgroup/cgroup.subtree_control
mkdir /sys/fs/cgroup/dpdk
echo "2,3,4,5" > /sys/fs/cgroup/dpdk/cpuset.cpus
echo $(pidof my_app) > /sys/fs/cgroup/dpdk/cgroup.procs
```

 [ULK Ch7 调度](../../../../16-linux-kernel-deep/chapter-07-process-scheduling/)

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. ILP](./section-3-指令级并发.md)
