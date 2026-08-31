# 03 — 6.x 的 NAPI 改动：threaded NAPI、软中断演进与 GRO 批量化

> **对应 Rosen:** Ch1（NAPI 基础）+ Ch14（高级主题 RPS/RFS）
> **内核版本:** NAPI 原始设计 2.5+；threaded NAPI **5.11+**；`netif_receive_skb_list()` 批量分发 5.x；
> `gro_flush_timeout` 复用为 NAPI defer **5.11+**
> **内核源码路径:** `net/core/dev.c`、`net/core/gro.c`、`kernel/softirq.c`

## 文档概述

原笔记把 threaded NAPI、busy polling、budget 揉在一篇里，且 busy polling 只讲了
`SO_BUSY_POLL` 一个 socket 选项 —— 那其实是**最老、最不推荐**的一代。

**本篇与 [05-busy-poll-mechanism](./05-busy-poll-mechanism.md) 的分工：**

| 篇 | 讲什么 |
|----|--------|
| **本篇** | NAPI 自身的演进：threaded NAPI、软中断上下文的局限、GRO 批量化 |
| [05](./05-busy-poll-mechanism.md) | **轮询的驱动者是谁**：三代 busy polling 与 NAPI defer |

简单说：本篇讲"**NAPI 在哪儿跑**"，05 讲"**谁去按 NAPI 的按钮**"。

---

## 一、为什么软中断是个问题

传统 NAPI 的 poll 跑在 `NET_RX_SOFTIRQ` 软中断上下文。它的三个固有缺陷：

| 缺陷 | 后果 | 6.x 的对策 |
|------|------|-----------|
| 没有独立调度实体 | 与同核所有软中断共享 ksoftirqd，无法单独绑核/设优先级 | threaded NAPI |
| 不能睡眠（原子上下文） | poll 里不能做会阻塞的事，驱动受限 | threaded NAPI（线程可睡眠） |
| 时间预算一到就被赶下 CPU | `netdev_budget_usecs` 超时 → 收一半被迫退出，尾延迟尖刺 | 调大预算 / busy polling |

栈回溯里看到这样，就是在软中断上下文里收包：

```
[<...>] net_rx_action+0x...
[<...>] __do_softirq+0x...
[<...>] do_softirq+0x...
```

而 threaded NAPI 会变成：

```
[<...>] napi_threaded_poll+0x...
[<...>] kthread+0x...
```

---

## 二、threaded NAPI（5.11+）

### 开启与验证

```bash
# 开启（每设备）
echo 1 > /sys/class/net/eth0/threaded

# 验证：线程名格式为 napi/<dev>-<queue>
ps -eo pid,comm,psr,pri | grep napi
#   882  napi/eth0-0    7   20

# 确认对应的是哪个队列（IRQ 亲和性应与之对齐）
cat /proc/irq/*/affinity_list 2>/dev/null | head
```

### 内核里的实现要点

```c
/* net/core/dev.c */
static int napi_threaded_poll(void *data)
{
    struct napi_struct *napi = data;

    while (!napi_thread_wait(napi)) {      /* 等 NAPI_STATE_SCHED 置位 */
        /* 主动调 poll */
        napi_poll(napi, napi->weight);
    }
    return 0;
}
```

要点：

- 线程通过 **`NAPI_STATE_SCHED` 位**被唤醒，而不是信号/队列 —— 与软中断路径共用同一个状态位
- `napi_thread_wait()` 返回 true 时线程退出（设备 down / 关闭 threaded）
- **硬中断仍然会发生**：线程等待期间是"关了队列中断 + 等调度位"，
  中断 handler 仍然执行 `napi_schedule()` 把它唤醒
  → 想连中断都省掉，那是 busy polling 的事（[05](./05-busy-poll-mechanism.md)）

### 绑核与优先级

```bash
PID=$(pgrep -f 'napi/eth0-0')
taskset -cp 7 $PID          # 绑到隔离核 7
chrt -f -p 50 $PID          # SCHED_FIFO，优先级 50
```

⚠️ **顺序很重要**：必须先 `echo 1 > threaded` 让线程出生，再绑核。
重启网卡（`ip link set eth0 down/up`）会**重新创建线程**，绑核设置丢失 ——
生产环境要写成 udev 规则或启动脚本，不能手工敲一次了事。

### 与软中断 NAPI 的实测差异（典型值，需自测）

| 指标 | 软中断 NAPI | threaded NAPI |
|------|------------|---------------|
| p50 收包延迟 | 略低（无线程切换） | 略高 |
| p99 / p999 | **抖动较大** | **明显更稳** ⭐ |
| 吞吐上限 | 高 | 略低 |
| 能否独占核 | 否 | ✅ |

**结论：threaded NAPI 换的是确定性，不是绝对速度。**
如果你的问题是"p50 很好但 p999 偶发尖刺"，它是对症的；
如果你的问题是"p50 就不够快"，它帮不上忙。

---

## 三、软中断侧的其它演进

| 改动 | 版本 | 内容 |
|------|------|------|
| `netdev_budget_usecs` | 4.x | 给软中断加时间预算，防止单核被收包独占 |
| softirq 可抢占讨论 | 长期 | 社区多次尝试，主线仍保持"软中断不可抢占" |
| `ksoftirqd` 优先级 | — | 始终是普通优先级，这是抖动的来源之一 |
| RPS/RFS | 2.6.35+ | 把包转发到别的核处理，**HFT 一般应关闭**（多一跳） |

**HFT 上 RPS 该不该开？**

```
RPS: 硬中断在 CPU0 收包 → enqueue_to_backlog() 塞到 CPU7 的 backlog
     → 给 CPU7 发 IPI → CPU7 的软中断再处理
```

看起来能让"业务核"收包，但代价是**一次跨核 IPI + 排队**，
对尾延迟是纯负担。HFT 的正确做法是用**硬件多队列 + RSS/ntuple** 让包
直接落到目标核，而不是靠 RPS 软件转发 → [06-queue-steering-rss](./06-queue-steering-rss.md)

---

## 四、GRO 的现代化

### 批量上送

5.x 引入 `netif_receive_skb_list()`：一次 poll 攒一批 skb，**一次调用**分发给协议栈，
而不是每个 skb 各跑一遍 `__netif_receive_skb_core()`。

```
旧：for each skb → __netif_receive_skb_core()   （重复跑 ptype 查找、tc ingress）
新：napi_gro_flush() → netif_receive_skb_list()  （批量分发，减少重复查找）
```

收益是**降低 CPU 占用**（吞吐导向），对单包延迟没有直接帮助。

### `gro_flush_timeout` 被复用成 NAPI defer 的旋钮

这是 5.11 一个容易困惑的设计：`gro_flush_timeout` 原本是"GRO 等多久就冲掉"，
现在被复用为"**NAPI 收完包后继续轮询多久**"。

```bash
# 单位是纳秒！不是微秒
echo 2      > /sys/class/net/eth0/napi_defer_hard_irqs
echo 200000 > /sys/class/net/eth0/gro_flush_timeout     # 200μs
```

> ⚠️ 写成 `200` 只有 0.2μs，等于没配 —— 这是最常见的配置错误。

→ 完整机制见 [05-busy-poll-mechanism](./05-busy-poll-mechanism.md)

---

## 五、观测清单

```bash
# 1. 软中断在每个核上的分布（看是否集中、是否有热核）
cat /proc/softirqs | head -3

# 2. 软中断是否被挤压
cat /proc/net/softnet_stat
# processed | time_squeeze | received_rps | flow_limit_count

# 3. 每次 NAPI poll 处理多少包（判断是否 budget 打满）
bpftrace -e 'tracepoint:net:napi_poll { @[comm] = hist(args->work); }'

# 4. threaded NAPI 线程跑在哪个核、有没有被抢占
ps -eo pid,comm,psr,pri,rtprio | grep napi

# 5. 软中断处理时长分布（>1ms 要警惕）
bpftrace -e '
  tracepoint:irq:softirq_entry /args->vec==3/ { @s[tid]=nsecs; }
  tracepoint:irq:softirq_exit  /args->vec==3/ {
      if (@s[tid]) { @us=hist((nsecs-@s[tid])/1000); delete(@s[tid]); } }'
```

---

## HFT 要点

- **threaded NAPI 治抖动，不治绝对延迟**。p50 已经很好但 p999 难看时用它
- **重启网卡会重建 napi 线程**，绑核/优先级设置会丢 —— 必须脚本化
- **RPS 对 HFT 是负优化**，用硬件队列 + RSS/ntuple 直落目标核
- **批量化（`skb_list`）是吞吐优化**，别指望它降延迟
- 想知道"包到底在哪一跳慢"，用上面的 bpftrace 逐跳打时间戳，
  而不是凭感觉调 sysctl

## 与 Rosen 3.x 的差异

| 维度 | Rosen（3.x） | 现代（5.x/6.x） |
|------|-------------|----------------|
| NAPI 上下文 | 仅软中断 | 软中断 / **threaded 线程** / busy poll |
| poll 能否睡眠 | 不能 | threaded 下可以 |
| 能否绑核 | 不能 | ✅（threaded） |
| 上送方式 | 逐 skb | `netif_receive_skb_list()` 批量 |
| 收完包的行为 | 立刻重开中断 | 可 defer 继续轮询（`napi_defer_hard_irqs`） |
| 跨核转发 | RPS 是新生事物 | RPS 成熟，但 HFT 应关闭 |

## 代码自测

<details>
<summary>Q1：threaded NAPI 开启后，硬中断还会发生吗？</summary>

会。threaded NAPI 只改变了"轮询在哪个上下文执行"，没改变唤醒方式。
线程平时阻塞在 `napi_thread_wait()` 等 `NAPI_STATE_SCHED` 位；
包到达时硬中断仍然触发，handler 里仍然 `napi_schedule()` 置位唤醒线程。
想连中断都省掉，需要 busy polling（应用/内核主动 poll，不依赖中断唤醒）
→ [05-busy-poll-mechanism](./05-busy-poll-mechanism.md)
</details>

<details>
<summary>Q2：`/proc/softirqs` 里 NET_RX 几乎全在 CPU0，业务核空闲，正常吗？</summary>

要分两种情况看：

- **中断亲和性没配**：所有队列的中断都打到 CPU0 → 不正常，要配
  `/proc/irq/<n>/smp_affinity_list`，或用 `irqbalance` 的 `banned_cpus` 把业务核排除
- **只有 RSS 哈希落在 CPU0 的那部分流**：说明流的四元组分布不均，
  要用 ntuple/flow-type 把行情流显式钉到目标队列 → [06](./06-queue-steering-rss.md)

单纯"CPU0 忙"不是问题，"**目标核没收到该收的包**"才是问题。
</details>

<details>
<summary>Q3：为什么我把 `netdev_budget` 调大后延迟反而变差了？</summary>

因为 `netdev_budget` 是"一轮软中断累计处理多少包"的上限。
调大意味着软中断一次占用 CPU 更久，期间同核的其它任务（包括你的收包进程）
被推迟 —— 平均吞吐上去，尾延迟变差。

低延迟场景应该反过来：**小 budget + busy polling**，让每次 poll 快速返回，
由应用主动、高频地去取。
</details>
