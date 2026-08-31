# 03 — io_uring vs epoll：事件模型与完成模型的账

> **对应 Rosen:** 无（epoll 见 Ch4 的 select/poll/epoll 演进）
> **内核版本：** epoll 2.6+；io_uring 5.1+，对比基于 **v6.6**（两侧机制见 [01](01-io-uring-net.md)/[02](02-io-uring-net-lwn.md)）
> **方法论：** 对比的不是 benchmark 数字，是**每条路径的固定成本结构**——数字随硬件变，结构不变

## 文档概述

本篇把「收一个行情包从到达 socket 到应用拿到数据」拆成两边各自的**完整成本账单**，然后给选型结论。

---

## 1. 两种模型的本质差异

```
epoll（事件模型 / readiness）：
  epoll_wait() ─→ 「fd 42 可读了」 ─→ 应用调 recvmsg() ─→ 数据到手
     系统调用 #1      只是通知                系统调用 #2      真正干活

io_uring（完成模型 / completion）：
  提交 RECV SQE（带 buffer） ─→ 内核异步收 ─→ CQE（数据已在你 buffer 里）
     0 次系统调用（SQPOLL）                     应用只是消费结果
```

**本质**：epoll 把「等待」和「操作」分成两个系统调用，每次都要进出内核；io_uring 把**操作本身**（不只是等待）搬进共享内存的提交/完成协议，让「进出内核」变成可选的优化项。

### 1.1 一次收包的完整账单（单连接，v6.6）

| 成本项 | epoll + recvmsg | io_uring（SQPOLL + multishot） |
|---|---|---|
| 唤醒等待 | `epoll_wait()` 阻塞/唤醒：1 次 syscall，进出内核 | 自旋读 CQ 内存：0 次（或等待时 `enter` 1 次） |
| 事件通知 | 每次 epoll_wait 返回（批量可摊） | 每个 CQE 一次（CQ 批量读取可摊） |
| 读数据 | `recvmsg()`：1 次 syscall + 数据拷贝 | 已在 provided buffer：只剩数据拷贝（同） |
| 提交下一个读 | 无（LT 模式自动再通知） | **无**（multishot 常驻，`MORE` 标志） |
| LT/ET 语义坑 | 有（busy loop 或漏读，见 [02](02-io-uring-net-lwn.md) Q4） | 无（`SOCK_NONEMPTY` 内核自决策） |
| buffer 交接 | recvmsg 参数里给 | provided 池自动 + CQE 报 id |

**净差**：epoll 路径每包（至少）2 次 syscall ≈ 2 × ~200-500ns（含 cache 污染更贵）；io_uring 热路径 **0 次**。注意这只是 syscall 差额——数据拷贝、协议栈成本两边相同。

**量级直觉**（不是结论，是校准）：syscall 进出 ~200-500ns，一次 1500B 拷贝 ~100ns，跨核 IPI ~1-2µs，线程唤醒 ~2-10µs。**io_uring 省的是第一项；若你的瓶颈在唤醒或拷贝，io_uring 帮不了**——先用 perf 看清再选。

---

## 2. 等待机制的三档

| 档位 | epoll | io_uring | 延迟 | CPU |
|---|---|---|---|---|
| **阻塞等待** | `epoll_wait` 睡眠 | `io_uring_enter(GETEVENTS)` 睡眠 | 唤醒延迟 2-10µs | ~0 |
| **中断 + 自旋混合** | `epoll_wait(timeout=0)` 轮询 | CQ tail 原子读自旋 | 亚 µs | 中 |
| **全 busy poll** | `SO_BUSY_POLL` + 空转 epoll_wait | SQPOLL + CQ 自旋 | 最低 | 100%×核数 |

**重要**：busy polling 的收益不只在 syscall——`SO_BUSY_POLL` 类机制让**协议栈处理也在收包的同一核上立刻发生**（避免 IPI 到别的核再处理）。io_uring 的 SQPOLL + NAPI busy poll（`io_uring_register_napi`，6.6 支持）组合是把「收包处理+完成+应用」钉在一个核上的完整方案，这是 epoll 时代做不到的。

---

## 3. 多连接扩展性

| 维度 | epoll | io_uring |
|---|---|---|
| 每连接状态 | epoll_item + 应用侧 fd map | SQE 常驻（multishot）+ user_data |
| 万级连接的就绪扫描 | O(就绪数)（红黑树+就绪链表，本身不慢） | O(完成数)（CQ 消费） |
| 批量操作 | 一次 epoll_wait 返回 N 个事件，但**每个还是要逐个 recv** | 一次提交 N 个操作 / 一次收 N 个 CQE，**操作本身批量化** |
| fd 开销 | 每连接一个 fd（epoll 本身 + 1） | registered files 索引化，fd 压力更小 |

**连接数 <100 的 HFT 行情机**（典型形态）：两边的「扩展性」差异不重要——每连接几十 µs 级的调度抖动才是重点，io_uring 的确定性（无 syscall、无 LT/ET 语义、可 busy poll）是主要收益。

**万级连接的网关形态**：io_uring 的批量提交/完成（一次 enter 处理一批）省 syscall 摊销；epoll 的「N 事件 + N 次 recv」syscall 数随事件数线性涨。

---

## 4. 决策表（HFT 视角）

| 场景 | 推荐 | 决定性理由 |
|---|---|---|
| 行情接收（组播/TCP，10-1000 连接，延迟敏感） | **io_uring**：SQPOLL + multishot recv + provided buffers + DEFER_TASKRUN | 零 syscall + 批内全收 + 确定性调度 |
| 交易发送（小报文，微秒级） | **io_uring**：SEND（不必 ZC——小报文 pin 页不划算）+ 链接模式 | 提交路径最短；ZC 只对大报文有意义 |
| 大报文发送（策略数据同步、快照推送） | **io_uring SEND_ZC** 或 MSG_ZEROCOPY | 拷贝成本超过 pin 管理成本 |
| 管理通道（SSH/监控/日志，几百连接） | **epoll**（或阻塞 IO） | 延迟不敏感，epoll 生态成熟、代码简单 |
| 与遗留框架共存 | epoll | io_uring 重构是伤筋动骨（buffer 生命周期、错误处理全变） |
| 内核 < 5.19 | epoll 或降级 io_uring（无 multishot） | multishot/provided-ring 是 io_uring 网络真正好用的分水岭 |

**反直觉的两条**：

1. **小报文别用 SEND_ZC**：一次 64B 报文的拷贝 ~几 ns，页 pin/unpin + notif 管理 ~百 ns 级——**零拷贝比拷贝贵**。ZC 的盈亏点在 KB 级以上
2. **epoll 不会消失**：io_uring 的复杂度（buffer 池管理、CQE 语义、setup flags 组合）是真实成本；延迟不敏感的代码用 epoll 写出来又快又稳

---

## 5. 迁移路径（epoll → io_uring）

```
阶段 0：perf 确认瓶颈在 syscall/唤醒（而不是拷贝/协议栈）——否则迁了白迁
阶段 1：io_uring 默认模式替换 epoll_wait+recv
        （行为等价：enter 提交+等待，完成=数据到手）—— 验证正确性
阶段 2：multishot recv + provided buffer ring
        （消掉每包重提交）—— 收包路径定型
阶段 3：SQPOLL + DEFER_TASKRUN + registered files/buffers
        （消掉 syscall + cache 局部性）—— 热路径定型
阶段 4（可选）：SEND_ZC、链接模式、NAPI busy poll
```

每阶段都有明确的收益预期和验证手段（`strace -c` 数 syscall、`perf stat` 看 cache-miss、延迟直方图）。

---

## 6. HFT 要点

1. **对比的坐标系是成本结构**：syscall 数、唤醒次数、拷贝字节数、cache 迁移距离——benchmark 数字会骗人（测试方法不同结果翻倍），结构不会
2. **io_uring 的核心收益 = 消除 syscall + 消除语义坑**（LT/ET、每包重提交），不是「更快的数据路径」——数据路径（协议栈+拷贝）两边一模一样
3. **busy poll 是延迟的最后一级**，io_uring 的 SQPOLL+NAPI 组合把「收包到应用」钉死单核，这是它对 epoll 的结构性优势
4. **小报文 ZC 是负优化**，盈亏点 KB 级
5. **迁移看瓶颈**：syscall 不占大头时，先解决拷贝（AF_XDP）或调度（绑核/隔离）

---

## 7. 代码自测

<details>
<summary>Q1：epoll LT 模式下收 10 个包（同时到达），最少几次系统调用？ET 模式呢？io_uring multishot 呢？</summary>

假设一次 epoll_wait 一次返回全部就绪事件（理想情况）：

- **epoll LT**：1 次 epoll_wait（返回 1 个 fd 可读）+ 10 次 recvmsg（LT 下必须读到空，否则 epoll_wait 立即再返回）= **11 次**；若 epoll_wait 每次只报一次就绪，还会更多
- **epoll ET**：1 次 epoll_wait + **循环 recvmsg 直到 EAGAIN** = 10 + 1 次（EAGAIN 那次）= 11-12 次，且漏读风险在自己
- **io_uring multishot（SQPOLL）**：**0 次系统调用**——10 个 CQE 依次出现在 CQ ring（SOCK_NONEMPTY 驱动内核连收），应用自旋消费

差异的本质：epoll 的「通知」和「读取」是两个 syscall；io_uring 的完成**自带数据**。
</details>

<details>
<summary>Q2：为什么说「io_uring 不是更快的数据路径」？</summary>

收包的协议栈成本（IP 解析 → UDP 查找 → socket 入队）和数据拷贝（skb → 用户 buffer），epoll 和 io_uring **走的是完全相同的内核代码**（`udp_recvmsg` / `tcp_recvmsg`）。

io_uring 改变的是**外围**：怎么知道有数据（CQE vs epoll 事件）、怎么把数据交给应用（provided buffer vs recvmsg 参数）、要几次 syscall（0 vs 2+）。

所以：**syscall 占大头的场景**（小包、高频、热 buffer 命中）io_uring 收益显著；**拷贝或协议栈占大头的场景**（大报文）收益有限，该去上 ZC/AF_XDP。
</details>

<details>
<summary>Q3：SO_BUSY_POLL 和 io_uring 的 SQPOLL 是一回事吗？</summary>

**不是，层次不同**：

- `SO_BUSY_POLL`（socket 级）：让**阻塞读/epoll** 在队列空时代价高昂地轮询协议栈（避免中断驱动的唤醒延迟）
- `SQPOLL`（ring 级）：让**内核线程**轮询 SQ ring 看有没有新提交（消除提交 syscall）

一个管「等数据」，一个管「交作业」。HFT 的完整组合是两者叠加：SQPOLL 消提交 syscall + NAPI/io_uring busy poll 消收包唤醒延迟（`io_uring_register_napi`）——全部代价是烧 CPU 换延迟确定性。
</details>

<details>
<summary>Q4：64 字节的交易报文，用 SEND 还是 SEND_ZC？</summary>

**SEND（普通）**。

成本对比（64B 报文）：
- 拷贝 64B：~几 ns（一条 cache line 都不到）
- ZC 路径：页 pin、`ubuf_info` 管理、notif 请求分配、第二个 CQE 的排队与消费、页解 pin——**百 ns 级固定成本**

ZC 的盈亏平衡在**KB 级**报文（拷贝成本线性涨，ZC 固定成本不涨）。64B 场景强行 ZC 是负优化——而且交易报文最重要的是**发送路径的确定性**，普通 SEND 的同步快路径（数据就绪即完成）恰恰最短最稳。
</details>

<details>
<summary>Q5：什么信号说明该从 epoll 迁 io_uring？什么信号说明不该迁？</summary>

**该迁**（满足任一）：
- `strace -c` 显示 syscall 时间占比 >10-15%（epoll_wait+recv 的进出内核成为大头）
- 延迟直方图右尾由唤醒抖动主导（跨核 IPI、调度延迟——DEFER_TASKRUN + busy poll 能压）
- 需要批内聚合语义（一批到达的包一次处理），epoll LT 的 busy 风险和 ET 的漏读风险都不可接受

**不该迁**：
- 瓶颈在数据拷贝（大报文）——去解决拷贝（ZC/AF_XDP），迁 io_uring 是白折腾
- 管理面代码（延迟不敏感）——epoll 生态成熟、可维护性好
- 团队没有带宽驾驭 buffer 生命周期管理（provided 池、NOTIF 语义）——写错的 io_uring 比写对的 epoll 慢
- 内核 < 5.19——没有 multishot/provided-ring 的 io_uring 网络是残血版
</details>

---

## 导航

- **上一篇：** [02-io-uring-net-lwn.md](02-io-uring-net-lwn.md) — multishot / provided buffers / SEND_ZC 的机制细节
- **相关：** [chapter-13-zerocopy-highperf/](../../chapter-13-zerocopy-highperf/) 零拷贝体系（ZC 盈亏点的另一侧） · [chapter-06-af-xdp/](../../chapter-06-af-xdp/) 收方向零拷贝 · [chapter-15-debugging-perf-tuning/](../../chapter-15-debugging-perf-tuning/) 延迟测量方法
- **章节主页：** [README](../README.md)
