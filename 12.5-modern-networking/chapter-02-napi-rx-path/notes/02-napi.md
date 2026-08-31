# 02 — NAPI：状态机、budget 与驱动 API

> **对应 Rosen:** Ch1（NAPI 基础）/ Ch14（高级主题 RPS/RFS）
> **内核源码路径:** `Documentation/networking/napi.rst`、`net/core/dev.c`、`include/linux/netdevice.h`

## 文档概述

内核官方 `napi.rst` 描述 NAPI 的完整工作流与驱动 API。原笔记只列了 6 个 API 和
threaded NAPI 的开关命令，本篇补上三件原笔记没讲但调优时一定会撞上事：

1. **状态位** —— NAPI 是怎么在"中断"和"轮询"之间切换的，卡住时看哪个标志位
2. **budget 的两层含义** —— 驱动的 `weight` 和全局的 `netdev_budget` 不是一回事
3. **`napi_complete_done()` 的返回值** —— 它返回 false 意味着什么

---

## 状态机

```
                    napi_enable()
                          │
                          ↓
              ┌───────────────────────┐
              │      ENABLED          │  中断开着，等着包来
              └───────────┬───────────┘
                          │ 包到达 → 硬中断 → napi_schedule()
                          ↓  NAPI_STATE_SCHED 置位，加入 sd->poll_list
              ┌───────────────────────┐
              │      SCHED / POLL     │  软中断 net_rx_action() 调用 poll
              └───────────┬───────────┘  NAPI_STATE_NPSVC 置位（正在 poll）
                          │
              ┌───────────┴───────────┐
              │                       │
   收完了(work < weight)     没收完(work == weight)
              │                       │
              ↓                       │ 不调用 napi_complete()，
   napi_complete_done()               │ 保持 SCHED，软中断再跑一轮
   清 NAPI_STATE_SCHED                │
   重开中断                            │
              │                       │
              └─────────→ ENABLED ←───┘
```

**关键状态位**（`include/linux/netdevice.h` 的 `napi_struct.state`）：

| 标志 | 含义 | 卡住时的诊断价值 |
|------|------|-----------------|
| `NAPI_STATE_SCHED` | 已挂入 poll_list，等待/正在被轮询 | 长期置位 = 软中断没跑或跑不完 |
| `NAPI_STATE_NPSVC` | 正在 poll 回调里 | 配合 SCHED 判断是否卡在驱动 |
| `NAPI_STATE_MISSED` | poll 期间又来了新包 | 正常现象，说明是忙的 |
| `NAPI_STATE_DISABLE` | 已禁用（`napi_disable()` 后） | ifdown / 驱动重置时会短暂出现 |
| `NAPI_STATE_THREADED` | 走内核线程而非软中断 | 判断是否真的开了 threaded |
| `NAPI_STATE_IN_BUSY_POLL` | 正被用户态 busy poll 占用 | 若长期置位 → 你的进程在死循环 poll |
| `NAPI_STATE_PREFER_BUSY_POLL` | 用了 `SO_PREFER_BUSY_POLL` | → [05](./05-busy-poll-mechanism.md) |

---

## 关键 API

| API | 作用 | 调用上下文 |
|-----|------|-----------|
| `netif_napi_add(ndev, napi, poll, weight)` | 注册 NAPI 实例，绑定 poll 回调与权重 | 驱动 probe/初始化 |
| `netif_napi_add_weight()` | 同上，显式指定 weight | 驱动初始化 |
| `napi_enable(napi)` | 启用，清 `NAPI_STATE_DISABLE` | 网卡 up |
| `napi_disable(napi)` | 禁用，**会睡眠等待** poll 退出 | 网卡 down / 重置 |
| `__napi_schedule(napi)` | 挂入 poll_list 并 raise 软中断 | 硬中断 handler（不可睡眠） |
| `napi_schedule_irqoff()` | 已知中断关闭时的优化版本 | 硬中断 handler |
| `napi_complete_done(napi, work)` | 报告完成，返回 bool | poll 回调末尾 |
| `napi_hash_add()` | 加入 napi_hash，供 busy polling 按 id 查找 | 驱动初始化 |

### `napi_complete_done()` 的返回值（原笔记没讲）

```c
/**
 * 返回值：
 *   false —— 没能停下来（poll 期间又有新包进来，得继续轮询）
 *   true  —— 确实收尾了，中断已重开
 */
bool napi_complete_done(struct napi_struct *n, int work_done);
```

驱动的典型写法：

```c
static int my_poll(struct napi_struct *napi, int budget)
{
    int work = 0;
    while (work < budget) {
        if (!rx_pkts_available())
            break;
        work += clean_rx(rx_ring, budget - work);
    }

    /* 收完了（不足 budget）→ 尝试收尾、重开中断 */
    if (work < budget && napi_complete_done(napi, work))
        reenable_irq();      /* 驱动私有的重开中断动作 */

    return work;   /* 必须返回实际处理的包数，budget 用尽时返回 budget */
}
```

> **返回值必须 ≤ budget**。返回 budget 表示"还没干完"，软中断会再安排一轮；
> 这是 `softnet_stat` 里 `time_squeeze` 上涨的直接原因。

---

## budget 的两层含义（最容易搞混）

| 层 | 变量 | 默认 | 控制什么 |
|----|------|------|---------|
| 每 NAPI 实例 | `napi->weight` | 64 | **单次 poll 回调**最多处理多少包 |
| 每软中断轮次 | `net.core.netdev_budget` | 300 | **一轮 NET_RX_SOFTIRQ** 累计最多处理多少包（所有 NAPI 合计） |
| 时间预算 | `net.core.netdev_budget_usecs` | 2000 | 一轮软中断最多跑多久（μs），超时强行退出 |

```bash
# 查看与调整
sysctl net.core.netdev_budget          # 300
sysctl net.core.netdev_budget_usecs    # 2000
sysctl -w net.core.netdev_budget=1000  # 吞吐导向
```

**HFT 的取向相反**：低延迟要的是"小批量、快返回"，
所以**不应该**盲目加大 budget —— 加大只是让单次 poll 干得更久，
尾延迟反而变差。小 budget + busy polling 才是低延迟组合。

观察是否够用：

```bash
cat /proc/net/softnet_stat
# 字段：processed  time_squeeze  received_rps  flow_limit_count ...
# time_squeeze 持续增长 = 时间预算被压爆，包进得比处理得快
```

---

## threaded NAPI（5.11+）

传统 NAPI 在 `NET_RX_SOFTIRQ` 软中断上下文执行：
与 ksoftirqd 共享 CPU、可能被抢占、**无法单独绑核**。

Threaded NAPI 把 poll 搬到一个独立的内核线程：

```bash
# 启用
echo 1 > /sys/class/net/eth0/threaded

# 确认线程出现
ps -eo pid,comm,psr | grep napi
#   1234 napi/eth0-0    5      ← 第 4 列是它当前跑在哪个核上

# 绑核 + 提优先级（RT 调度，避免被普通进程抢占）
taskset -cp 5 1234
chrt -f -p 50 1234
```

| 维度 | 软中断 NAPI | threaded NAPI |
|------|------------|---------------|
| 调度实体 | ksoftirqd/N（共享） | 独立的 `napi/eth0-N` 线程 |
| 能否绑核 | 否 | ✅ 可以 |
| 能否设 RT 优先级 | 否 | ✅ 可以 |
| 额外开销 | 无 | 一次线程调度（百 ns 级） |
| 适用 | 吞吐 | **延迟确定性** |

> 注意：threaded NAPI 和 busy polling 是**两种思路**。
> threaded 是"让轮询有确定的调度实体"；busy polling 是"让应用自己轮询"。
> 不要同时开 —— 见 [05](./05-busy-poll-mechanism.md)。

---

## GRO 与 NAPI 的关系

```
驱动 poll
   │
   ├── napi_gro_receive(napi, skb)      ← 交给 GRO 排队
   │        └─ dev_gro_receive() 尝试与已有的 flow 合并
   │              ├─ 能合并 → skb 挂到 napi->gro_hash 链表，暂不上送
   │              └─ 不能合并 → 立刻上送 netif_receive_skb()
   │
   └── napi_gro_flush(napi, flush_old)  ← poll 结束时冲刷
            └─ 把还挂在链表上没凑满的包全部上送
```

**这解释了 GRO 为什么会引入延迟**：包进来后要先在 `gro_hash` 里等着，
看后面有没有同流的包能凑一起。等的过程就是延迟。

- `flush_old=1`：连"还没到期的"一起冲，用于 `napi_complete()` 前必须清空
- 关掉 GRO：`ethtool -K eth0 gro off` → 驱动改走 `netif_receive_skb()` 直上
- **XDP 不经过 GRO**：XDP 在 GRO 之前，这是它能保住低延迟的原因之一

→ 详见 [04-gro-gso](./04-gro-gso.md)

---

## busy polling 与 `napi_id`

`SO_BUSY_POLL` 要求驱动支持 NAPI ID：

```c
/* 接收时把 napi_id 记到 skb 上 */
skb->napi_id = napi->napi_id;          /* 驱动或 napi_gro_receive 里 */

/* socket 侧记住自己最近一次收包的 napi_id */
sk->sk_napi_id = skb->napi_id;
```

用户态 `recvmsg()` 时，内核拿着 `sk->sk_napi_id` 去 `napi_hash` 里找到
对应的 `napi_struct`，然后**直接调用它的 poll** —— 完全跳过中断和软中断。

```bash
# 确认驱动支持（napi_id 非 0）
bpftrace -e 'tracepoint:net:netif_receive_skb { @[args->napi_id] = count(); }'
```

- `napi_id == 0` → 该驱动没接 NAPI ID，`SO_BUSY_POLL` 会静默退化成普通阻塞收包
  **（这是最容易被忽略的坑：配了 busy poll 却毫无效果）**
- 完整机制与三代演进 → [05-busy-poll-mechanism](./05-busy-poll-mechanism.md)

---

## HFT 要点

- **先确认 `napi_id` 非 0**，否则所有 busy polling 配置都是白配
- **budget 不是越大越好**：低延迟要小批量快返回，大 budget 换的是吞吐
- `softnet_stat` 的 `time_squeeze` 是第一观测点，涨了说明包进得比处理快
- threaded NAPI 的价值是**可绑核 + 可设 RT**，不只是"换个上下文"
- `napi_disable()` 会睡眠，不能在中断或持有自旋锁时调用

## 与 Rosen 3.x 的差异

| 维度 | Rosen（3.x） | 现代（5.x/6.x） |
|------|-------------|----------------|
| poll 上下文 | 仅软中断 | 软中断 / **threaded 线程** / busy poll |
| budget | 固定 64 | `weight` + `netdev_budget` + 时间预算三层 |
| NAPI ID | 无 | 有（`napi_id`），busy polling 的基础 |
| GRO 集成 | 有但简单 | `gro_hash` 按流合并、`netif_receive_skb_list()` 批量上送 |
| 缓冲区 | 驱动自管 | page_pool 复用 → [chapter-04](../../chapter-04-page-pool/) |

## 代码自测

<details>
<summary>Q1：驱动的 poll 回调返回 budget 意味着什么？内核会怎么处理？</summary>

意味着"这一轮没干完，还有包"。
`net_rx_action()` 看到返回值 == weight（budget）时，
**不会**调用 `napi_complete()`，NAPI 保持 `NAPI_STATE_SCHED`，
软中断退出前会重新 raise 自己再跑一轮。
代价是这一轮软中断占用了更长的 CPU 时间，
可能挤掉同核上的其它软中断或导致 `netdev_budget_usecs` 超时。
</details>

<details>
<summary>Q2：开了 threaded NAPI 之后还能用 busy polling 吗？</summary>

技术上可以，但**没有意义，而且有害**。
threaded NAPI 是把轮询交给一个内核线程；busy polling 是让应用线程自己轮询。
两者都要"独占"这个 NAPI 实例，同时开会互相抢，
表现为延迟抖动变大而吞吐没提升。
选一个：要调度确定性用 threaded，要最低延迟用 busy poll。
</details>

<details>
<summary>Q3：`napi_disable()` 为什么可能睡眠？驱动里要注意什么？</summary>

它要等待正在进行的 poll 回调退出（等待 `NAPI_STATE_NPSVC` 清零），
所以内部有同步等待，不能睡眠的上下文（自旋锁内、中断上下文）调用会死锁/告警。
驱动正确的顺序是：先 `napi_disable()` 停轮询 → 再停硬件队列、释放 ring →
最后 `netif_napi_del()` 注销。
</details>
