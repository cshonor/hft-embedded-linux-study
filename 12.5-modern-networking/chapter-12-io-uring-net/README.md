# Chapter 12: io_uring 网络收发

> 来源：kernel-docs（io_uring net）+ LWN（io_uring net + vs epoll）+ **v6.6 源码逐行核对**
> 对标：Rosen（无 io_uring，3.x 仅 epoll）
> 内核版本：以 **v6.6** 为准（v6.6 起 io_uring 代码已从 `fs/io_uring.c` 迁入 `io_uring/`，
> 网络操作在 `io_uring/net.c` 1522 行）；multishot 5.19+，SEND_ZC 6.0+
> （`io_uring/io_uring.c`、`io_uring/net.c`、`io_uring/notif.c`、
> `net/ipv4/{tcp,udp}.c`、`include/uapi/linux/io_uring.h`）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [io-uring-net](notes/01-io-uring-net.md) | **机制基础**：SQ/CQ 双环结构（SQ 是索引环不是队列）、setup flags 全表（SQPOLL/DEFER_TASKRUN/SINGLE_ISSUER 的取舍）、网络 opcode 全表（socket→收发→shutdown 全生命周期）、IOSQE 链接模式编排「收→算→发」、registered 三体系辨析 |
| 2 | [io-uring-net-lwn](notes/02-io-uring-net-lwn.md) | **v6.6 深水区**：multishot recv 逐行（`MORE`/`SOCK_NONEMPTY` 双标志语义）、provided buffers 生命周期、SEND_ZC 双 CQE 协议（NOTIF 为 buffer 解锁界）、`SOCK_SUPPORT_ZC` 无条件置位、REPORT_USAGE 监控假零拷贝、收方向零拷贝在 v6.6 不存在 |
| 3 | [io-uring-vs-epoll](notes/03-io-uring-vs-epoll.md) | **选型对比**：事件模型 vs 完成模型的成本账单（syscall 数/唤醒/拷贝三轴）、等待机制三档、多连接扩展性、决策表（小报文 ZC 是负优化）、五阶段迁移路径 |

## 本篇的核心结论

1. **⭐ io_uring 不是更快的数据路径**——协议栈和拷贝走的是同一份内核代码。
   它消除的是**外围成本**：syscall（epoll 每包 2+ 次 vs SQPOLL 0 次）、
   LT/ET 语义坑（`SOCK_NONEMPTY` 让内核自决策继续收）、每包重提交（multishot 常驻）。

2. **⭐ multishot recv 是网络 io_uring 的分水岭**（5.19+）：一次提交常驻，
   `IORING_CQE_F_MORE` 承诺后续、消失即请求已死（多半 buffer 池干涸）；
   `IORING_CQE_F_SOCK_NONEMPTY`（来自 msg_inq）驱动内核批内连收。

3. **⭐ SEND_ZC 的 buffer 生命周期以 NOTIF CQE 为界**：第一个 CQE 只是「数据进了
   协议栈」，页仍被 pin；带 `IORING_CQE_F_NOTIF` 的第二个 CQE（`io_tx_ubuf_callback`
   析构链）才是「buffer 自由」。提前复用 = 静默数据损坏。

4. **⭐ 小报文 ZC 是负优化**：64B 拷贝 ~几 ns，页 pin + notif 管理 ~百 ns 级固定
   成本——盈亏点在 KB 级。交易报文用普通 SEND 的同步快路径更短更稳。

5. **⭐ `IOSQE_ASYNC` 是热路径毒药**：强制走 io-wq（线程池唤醒几十 µs 级）；
   数据就绪时默认同步快路径才是低延迟正道。

6. **⭐ DEFER_TASKRUN 买的是 cache 局部性**：完成回调从中断上下文推迟到
   提交线程 enter 时执行（同核同上下文），要求 SINGLE_ISSUER。

## HFT 关联

- **行情收发标准形态**：SQPOLL（sq_thread_idle=∞ 永不睡）+ multishot recv +
  ring-mapped provided buffers + registered files + DEFER_TASKRUN——一次提交常驻、
  零 syscall、批内全收
- **等待三档按延迟预算选**：阻塞（2-10µs 唤醒）/自旋（亚 µs）/全 busy poll
  （SQPOLL + NAPI busy poll 把「收包到应用」钉死单核，epoll 时代做不到的完整方案）
- **收方向零拷贝 v6.6 不存在**（后续内核的 zcrx 才有）：一次 1500B 拷贝 ~100ns，
  成为新瓶颈时上 AF_XDP，而不是一开始就上
- **CQ 溢出是右尾延迟的隐形来源**：监控 enter 的 EBUSY，CQSIZE 宁大勿小
- **迁移看瓶颈**：syscall 占比 >10-15% 才值得迁；瓶颈在拷贝/协议栈时先解决那头

## 交叉引用

- `12.5-modern-networking/chapter-13-zerocopy-highperf/`：MSG_ZEROCOPY 体系（SEND_ZC 的前身）与 SO_REUSEPORT
- `12.5-modern-networking/chapter-06-af-xdp/`：收方向零拷贝的真答案
- `12.5-modern-networking/chapter-15-debugging-perf-tuning/`：延迟测量（迁移决策的依据）
- `04-cpp/M2-cpp-network-programming/`：epoll/socket 基础
