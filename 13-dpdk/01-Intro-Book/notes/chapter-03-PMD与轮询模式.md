# Ch 3 PMD 与轮询模式 · PMD & Poll Mode

> **01-Intro-Book** · 官方 Programmer's Guide · **精读**

> **实体书：** [chapter-07-nic-performance-optimization](../chapter-07-nic-performance-optimization/) §2 精讲轮询/混合中断原理与 UIO/VFIO；[chapter-06-pcie-packet-io](../chapter-06-pcie-packet-io/) §3 描述符环与 MMIO。

> **本篇分工：** 实体书讲**为什么轮询比中断快**；本篇是**实验向**——
> `rx_burst` 的真实语义、BURST_SIZE 的吞吐/延迟权衡怎么**实测**、队列与 lcore 怎么绑。

> **实验：** [code/mcast-minimal/src/main.c](../code/mcast-minimal/src/main.c)
> 里的 `hist_burst` 就是为量化本篇第二节的权衡而写的。

---

## 一、`rte_eth_rx_burst()` 的真实语义

```c
uint16_t nb_rx = rte_eth_rx_burst(port, queue, bufs, BURST_SIZE);
```

新手最容易误解的三件事：

| 误解 | 事实 |
|---|---|
| 返回 0 是异常 | **正常**。轮询就是"问一次有没有"，没有就返回 0，继续问 |
| 会等到凑满 BURST_SIZE | **不会**。尽力而为，有多少给多少，可能只给 1 个 |
| 它只是"取包" | 它还**顺手重填描述符**——这是理解 mempool 大小的关键 |

一次调用内部大致做了：

```
1. 从描述符环当前位置起，检查 DD（Descriptor Done）位
2. 有 DD 的 → 取出对应 mbuf 指针，放进 bufs[]
3. 遇到 DD 为 0 就停（或取满 BURST_SIZE）
4. 更新环尾指针
5. 若累计已取走的 mbuf ≥ rx_free_thresh（默认 0 = 由 PMD 决定，常见 32）
      → 批量从 mempool 取新 mbuf，把描述符填回去
6. 返回本次取到的包数
```

**第 5 步是分批做的，不是每包一次**——这就是 `rx_burst` 能做到
"取 1 个包和取 32 个包单次调用成本差不多"的原因，
也是为什么 pool 大小必须包含 `Σ RX_DESC`（描述符环那批 mbuf 长期押在网卡手里）。

→ 详见 [chapter-02-mbuf与内存池.md](./chapter-02-mbuf与内存池.md) 第四节。

---

## 二、BURST_SIZE：吞吐与尾延迟的直接兑换

这是本篇唯一一个**必须自己实测**才能定的参数，任何"经验值"都只能当起点。

### 大 burst 的好处（换吞吐）

- 摊薄 `rx_burst` 函数调用成本（几十 cycles/包 → 几 cycles/包）
- 描述符 refill 更成批，mempool 访问更连续
- PCIe 上更连续的读序，TLP 效率更高

### 大 burst 的代价（换尾延迟）——队头等待

一次 burst 里，第 30 个包必须等前 29 个处理完才被"看到"：

```
t_burst0                                                    t_end
   │                                                          │
   ├─ 包1 parse ─┤                                            │  等待 ≈ 0
   │             ├─ 包2 parse ─┤                              │  等待 ≈ 1×parse
   │                           ├─ 包3 parse ─┤                │  等待 ≈ 2×parse
   │                                         ├─ ...           │
   │                                                          └─ 包30 等待 ≈ 29×parse
```

粗估：**批次末尾的额外等待 ≈ (批次内位置) × 单包处理时间**。
parse 花 100ns 时，32 burst 的末包就背了约 3.1μs ——
已经超过 DPDK 本身的收包延迟，等于白优化。

### 怎么实测

`mcast-minimal` 里两个直方图就是干这个的：

| 直方图 | 含义 | 随 BURST_SIZE 变化 |
|---|---|---|
| `hist_parse` | 纯解析代码耗时 | 基本不变，是基线 |
| `hist_burst` | 从 burst 开始到本包解析完 | **随 BURST_SIZE 增大而明显右移** |

做法：把 `BURST_SIZE` 依次改成 1 / 8 / 16 / 32 / 64，各跑一轮，
记录 `hist_burst` 的 **p50 和 p999**：

```
BURST=1   :  p50 ≈ parse        p999 ≈ parse           ← 最确定，吞吐最低
BURST=32  :  p50 ≈ parse×16     p999 ≈ parse×31        ← 吞吐高，尾延迟差
```

**HFT 怎么选：**

- 纯行情解码（收进来 → 解码 → 喂策略）：常用 **8~32**，吞吐优先
- "收一个就要立刻决策"的场景：用 **1**，放弃摊销换确定性
- 别拿默认值当结论 —— 这个数必须由**你自己的 parse 成本**算出来

---

## 三、队列与 lcore 绑定

无锁的前提是**一核一队列**：

```
队列 0 ──→ lcore 2 ──→ 独占，无锁
队列 1 ──→ lcore 3 ──→ 独占，无锁
```

多线程收同一个队列，即便能做到无锁，也会带来两个问题：
① 同一描述符环所在 cache line 的争用；② **包乱序**（行情序列号会立刻暴露）。

### 骨架

```c
static int lcore_rx(void *arg)
{
    unsigned q = (unsigned)(uintptr_t)arg;
    struct rte_mbuf *bufs[BURST_SIZE];

    while (!force_quit) {
        uint16_t n = rte_eth_rx_burst(port, q, bufs, BURST_SIZE);
        for (uint16_t i = 0; i < n; i++) {
            /* ... */
            rte_pktmbuf_free(bufs[i]);
        }
    }
    return 0;
}

unsigned q = 0;
RTE_LCORE_FOREACH_WORKER(lcore_id) {
    rte_eal_remote_launch(lcore_rx, (void *)(uintptr_t)q++, lcore_id);
}
```

### 配套（缺一项基本白忙）

| 项 | 为什么 |
|---|---|
| `-l` 明确指定 lcore | 别让 EAL 猜 |
| lcore 与网卡**同 NUMA** | 跨节点收包 = 每包一次远端内存访问 |
| `--socket-mem` 按节点分配大页 | pool 建在哪就要哪边有大页 |
| `isolcpus` + `nohz_full` + `rcu_nocbs` | **把核从调度器手里拿走**，见下节 |
| 关 `irqbalance`、中断绑走 | 见 [12.5/06 队列定向](../../../12.5-modern-networking/chapter-02-napi-rx-path/notes/06-queue-steering-rss.md) |
| `watchdog=0` / `nmi_watchdog=0` | 避免周期性 NMI/perf 打断 |

### 验证绑对了没

```bash
# 每队列收包数 —— 确认流量确实分到了预期队列
dpdk-procinfo -- --xstats | grep 'rx_q.*packets'

# lcore 所在 CPU
taskset -cp <pid>
```

---

## 四、轮询的代价：100% CPU，以及被抢占的尖峰

轮询核永远 100% 占用——这是**设计选择**，不是缺陷：
用一整颗核换"延迟不抖动"。但它有个致命前提：**这颗核不能被任何人抢走**。

**被抢占一次 = 一次几十 μs 到 ms 级的延迟尖峰**，在 p999 上会看得清清楚楚。
典型来源：

| 来源 | 症状 | 对策 |
|---|---|---|
| 其他进程被调度上来 | 周期性尖峰 | `isolcpus`，不与业务混部 |
| 时钟 tick | 每 1/10/250 ms 一次 | `nohz_full=<核列表>` |
| RCU 回调 | 周期性 | `rcu_nocbs=<核列表>` |
| 设备中断 | 随机尖峰 | 中断绑到别的核 |
| watchdog / NMI | 罕见但致命 | `watchdog=0`，`nmi_watchdog=0` |
| C-state 休眠 | 唤醒延迟 | `intel_idle.max_cstate=0`、`processor.max_cstate=1` |

**怎么确认是被抢占？** 量两次 `rx_burst` 之间的间隔：

```c
uint64_t t_prev = rte_rdtsc();
while (!force_quit) {
    uint16_t n = rte_eth_rx_burst(port, q, bufs, BURST_SIZE);
    uint64_t t = rte_rdtsc();
    hist_record(&hist_gap, t - t_prev);   /* 间隔分布 */
    t_prev = t;
}
```

如果 `hist_gap` 的 p999 出现几百 μs 的**孤立尖峰**、而 p50 正常，
**几乎一定是调度/中断问题，不是你的代码问题**。
（方法论见 [12.5/延迟测量](../../../12.5-modern-networking/chapter-15-debugging-perf-tuning/notes/03-latency-measurement.md)）

---

## 五、混合中断模式：什么时候该用

实体书 §2 讲了 `l3fwd-power` 的状态机。本篇只给**结论**：

| 场景 | 建议 |
|---|---|
| 行情核（热路径） | **不用**。休眠换来的省电，代价是首包 10–50μs 的唤醒延迟，HFT 承受不起 |
| 管理面 / 备用链路 | 可用：`rte_eth_dev_rx_intr_enable()` + `rte_epoll_wait()` 阻塞 |
| 流量有明显潮汐 | 可用，但要加大环深度，避免唤醒前溢出丢包 |

切换 API：

```c
rte_eth_dev_rx_intr_enable(port, queue);    /* 进休眠前开中断 */
/* ... epoll 阻塞在 eventfd 上 ... */
rte_eth_dev_rx_intr_disable(port, queue);   /* 唤醒后关中断，回到纯轮询 */
```

---

## 六、观测清单

```c
/* 标准统计：丢包定位的主力 */
struct rte_eth_stats st;
rte_eth_stats_get(port, &st);
st.ipackets;    /* 收包总数 */
st.imissed;     /* 网卡侧收不进来 */
st.rx_nombuf;   /* mempool 耗尽 */

/* 扩展统计：每队列细分、驱动私有项 */
int len = rte_eth_xstats_get_names(port, NULL, 0);
struct rte_eth_xstat_name *names = calloc(len, sizeof(*names));
uint64_t *values = calloc(len, sizeof(*values));
rte_eth_xstats_get_names(port, names, len);
rte_eth_xstats_get(port, values, len);
```

| 计数器涨了 | 说明 | 处理 |
|---|---|---|
| `imissed` | 网卡收不进来：描述符环太小 / PCIe 跟不上 | 加大 `RX_RING_SIZE` |
| `rx_nombuf` | mempool 耗尽：应用消费慢或漏 free | 见 [ch02](./chapter-02-mbuf与内存池.md) |
| `ierrors` | CRC / 帧错误：物理层问题 | 查光模块、线缆 |

---

## 相关章节

- 实体书：[section-2-轮询与混合中断模式.md](../chapter-07-nic-performance-optimization/notes/section-2-轮询与混合中断模式.md)（原理与状态机）· [section-3-IO性能深度优化.md](../chapter-07-nic-performance-optimization/notes/section-3-IO性能深度优化.md)
- 上一章：[chapter-02-Cache与内存.md](./chapter-02-Cache与内存.md) · [chapter-02-mbuf与内存池.md](./chapter-02-mbuf与内存池.md)
- 下一章：[chapter-04-零拷贝与用户态旁路.md](./chapter-04-零拷贝与用户态旁路.md)
- 队列定向与 RSS：[12.5/chapter-02/notes/06-queue-steering-rss](../../../12.5-modern-networking/chapter-02-napi-rx-path/notes/06-queue-steering-rss.md)
- 延迟测量方法论：[12.5/chapter-15/notes/03-latency-measurement](../../../12.5-modern-networking/chapter-15-debugging-perf-tuning/notes/03-latency-measurement.md)
- 流分类：[chapter-08-流分类与多队列.md](./chapter-08-流分类与多队列.md)
- 实验：[code/mcast-minimal/](../code/mcast-minimal/)
