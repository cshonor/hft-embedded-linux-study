# 06 — Busy Polling 完整机制：从 socket 选项到 NAPI defer

> **对应 Rosen:** 不存在（3.x 无 busy polling）
> **内核源码路径:** `Documentation/networking/napi.rst`、`net/core/dev.c`、`include/net/busy_poll.h`

## 文档概述

[03-napi-modern](./03-napi-modern.md) 只讲了 `SO_BUSY_POLL` 这一个 socket 选项。
但 HFT 实际用的不是它——5.11+ 引入了**不依赖 socket 的 NAPI defer 轮询机制**，
让整个网卡进入"准轮询"状态。这是**内核协议栈能把延迟压到的极限**，
再往下只有旁路（XDP/AF_XDP/DPDK）。

---

## 核心内容

### 三代 busy polling 的演进

| 代 | 内核 | 机制 | 粒度 | 局限 |
|----|------|------|------|------|
| 一代 | 3.11+ | `SO_BUSY_POLL` | 单 socket | 只有调用 `recvmsg` 时才轮询；socket 一多开销失控 |
| 二代 | 5.11+ | `SO_PREFER_BUSY_POLL` + NAPI defer | 单 socket 但用全局轮询线程 | 仍需有 socket 在等 |
| 三代 | 5.11+ | `napi_defer_hard_irqs` + `gro_flush_timeout` | **整个网卡队列** | 无需任何 socket，纯 sysfs 配置 |

**关键区分：** 一代/二代是"应用去问内核要不要数据"；三代是"内核自己一直盯着网卡"。

---

### 三代机制：NAPI defer（HFT 真正用的）

传统 NAPI：收完一批包 → 重开中断 → 睡眠等下一个中断（中断唤醒代价 ~1-5 μs）。

NAPI defer：收完包后**不立即重开中断**，而是继续轮询一段时间。

```bash
# 核心两个参数（按队列/设备配置）
echo 2       > /sys/class/net/eth0/napi_defer_hard_irqs   # 允许连续推迟硬中断的次数
echo 200000  > /sys/class/net/eth0/gro_flush_timeout      # 无包后继续轮询多久（纳秒 = 200μs）

# 配套全局默认值（微秒）
sysctl -w net.core.busy_read=50      # SO_BUSY_POLL 的默认值
sysctl -w net.core.busy_poll=50      # epoll/io_uring 无 socket 时的默认 busy poll 时长
```

| 参数 | 单位 | 作用 |
|------|------|------|
| `napi_defer_hard_irqs` | 次 | NAPI 轮询结束后，推迟重新使能硬中断的最大次数 |
| `gro_flush_timeout` | **纳秒** | 无新包时继续保持轮询的时长（超时才退出并开中断） |
| `net.core.busy_read` | 微秒 | `SO_BUSY_POLL` 的系统级默认值 |
| `net.core.busy_poll` | 微秒 | epoll/io_uring 层 busy poll 的默认时长 |

> ⚠️ `gro_flush_timeout` 单位是**纳秒**，不是微秒。写成 `200` 只有 0.2μs，等于没配。
> 常见值：200000（200μs）；要求极致延迟配 1000000（1ms），代价是 CPU 长期 100%。

**效果：** 队列进入持续轮询，硬中断几乎不再触发，
延迟从"中断路径的 5-10μs"压到"纯轮询的 1-3μs"。

---

### 配套 socket 选项（二代，与 defer 叠加）

```c
/* 5.11+：告诉内核优先用 busy poll 而不是等中断 */
int prefer = 1;
setsockopt(fd, SOL_SOCKET, SO_PREFER_BUSY_POLL, &prefer, sizeof(prefer));

/* busy poll 时长（微秒）；0 = 用 net.core.busy_read 默认值 */
int usec = 50;
setsockopt(fd, SOL_SOCKET, SO_BUSY_POLL, &usec, sizeof(usec));

/* 6.1+：一次 busy poll 最多处理多少包（控制单次调用的延迟上限） */
int budget = 64;
setsockopt(fd, SOL_SOCKET, SO_BUSY_POLL_BUDGET, &budget, sizeof(budget));
```

| 选项 | 内核 | 说明 |
|------|------|------|
| `SO_BUSY_POLL` | 3.11+ | 本 socket 的 busy poll 时长（μs） |
| `SO_PREFER_BUSY_POLL` | 5.11+ | 优先 busy poll，压过中断路径 |
| `SO_BUSY_POLL_BUDGET` | 6.1+ | 单次调用处理包数上限，**牺牲吞吐换尾延迟** |

---

### epoll / io_uring 侧的纳秒超时

5.11+ 的 `epoll_pwait2()` 支持**纳秒级超时**，配合 busy poll 用：

```c
struct timespec ts = { .tv_sec = 0, .tv_nsec = 100000 };  /* 100μs */
epoll_pwait2(epfd, events, maxevents, &ts, NULL);
```

io_uring 侧对应的是 SQPOLL 模式 + `IORING_SETUP_COOP_TASKRUN`（6.x 默认行为），
内核线程替你轮询，省掉一次系统调用。→ 详见 [12-io-uring-net](../../chapter-12-io-uring-net/)

---

### 代价与配套配置

Busy polling 是**拿 CPU 换延迟**，配套必须做全，否则收益被调度抖动吃掉：

```bash
# 1. 隔离 CPU（ isolcpus + nohz_full + rcu_nocbs ）
#    内核启动参数：
#    isolcpus=domain,managed_irq,2-7 nohz_full=2-7 rcu_nocbs=2-7
#    managed_irq 很关键：把受管中断移出隔离核，否则仍会被打断

# 2. 关闭该核的 watchdog / timer 迁移
sysctl -w kernel.watchdog=0

# 3. 进程绑核
taskset -c 3 ./feed_handler

# 4. 关 GRO（聚合会等窗口，直接加延迟）
ethtool -K eth0 gro off

# 5. 中断合并归零
ethtool -C eth0 rx-usecs 0 rx-frames 0
```

| 代价 | 说明 |
|------|------|
| CPU 100% | 一个核全程被轮询占死，一核一队列 |
| 功耗 | 满载，不适合笔记本/云按量实例 |
| 调度敏感 | 必须 isolcpus + nohz_full，否则上下文切换会毁掉收益 |
| 多队列 | N 个队列要 N 个核，核数直接决定能收多少行情流 |

---

### 内核栈的极限在哪（为什么还要 DPDK）

即使把上面全部配到位，包仍然要走完这些步骤：

```
NIC → page_pool → sk_buff 分配 → GRO 判定 → 路由查找
   → UDP 层 socket 匹配 → 组播复制 → socket 接收队列
   → 唤醒/epoll 返回 → recvmsg() 拷贝到用户态  ← 又一次拷贝
```

**省不掉的：** sk_buff 分配、协议栈遍历、socket 查找、**内核→用户态的数据拷贝**。

| 方案 | 典型 tick-to-trade | 省掉了什么 |
|------|-------------------|-----------|
| 默认中断 | 10-50 μs | — |
| NAPI defer + busy poll | 2-5 μs | 中断唤醒、GRO 等待 |
| AF_XDP zero-copy | 0.5-2 μs | sk_buff、协议栈、拷贝 |
| DPDK | 0.3-1 μs | 再加内核 syscall 与队列管理 |

**结论：** NAPI defer 是**不旁路时的最优解**，投入产出比极高（几行 sysctl，无需改代码）。
要再往下走，必须放弃内核协议栈，代价是牺牲 TCP/路由/SSH 等一切内核功能。

→ 决策树见 [07-xdp-vs-dpdk](../../chapter-07-xdp-redirect-dpdk/notes/02-xdp-vs-dpdk.md)

---

## HFT 要点

- **先配 NAPI defer 再谈 DPDK。** 它是零代码成本的，且是内核栈的天花板
- `gro_flush_timeout` 单位是**纳秒**，写成 200 等于没配（最常见错误）
- 必须配 `isolcpus=...,managed_irq`，否则中断还是会打到你的轮询核
- `SO_BUSY_POLL_BUDGET` 是调尾延迟的旋钮：budget 小 → p999 好，吞吐差
- 一核一队列，核数直接决定能并行收多少条行情流——**这是容量规划的硬约束**
- 判断标准：如果 busy poll 后 p99 仍不达标，才值得付出旁路的复杂度

## 与 Rosen 3.x 的差异

| 维度 | Rosen（3.x） | 现代（5.x/6.x） |
|------|-------------|----------------|
| busy polling | 不存在 | 三代机制（socket / prefer / NAPI defer） |
| NAPI 收尾行为 | poll 完立即重开中断 | 可推迟中断持续轮询（defer） |
| epoll 超时精度 | 毫秒 | 纳秒（`epoll_pwait2`） |
| Rx buffer | 动态分配 | page_pool 复用，配合轮询无分配热点 |
