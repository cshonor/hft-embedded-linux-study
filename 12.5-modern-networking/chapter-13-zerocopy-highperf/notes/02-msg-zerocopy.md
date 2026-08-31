# 13-02 — MSG_ZEROCOPY：用户态发送零拷贝（内核文档 + errqueue 协议）

> **对应 Rosen:** Ch11（sendmsg 拷贝模式）
> **内核源码路径:** `Documentation/networking/msg_zerocopy.rst`、`net/core/skbuff.c`、`net/ipv4/tcp.c`

## 章节导航

| 上一篇 | 本篇 | 下一篇 |
|---|---|---|
| [13-01 scaling](01-scaling.md) | **13-02 MSG_ZEROCOPY** | [13-03 LWN 深入](03-msg-zerocopy-lwn.md) |

## 本节讲什么

传统发送路径 `sendmsg()` 把用户态 buffer `copy_from_user()` 进内核 skb——每字节一次拷贝。MSG_ZEROCOPY（4.14+）把这个逐字节成本替换为**页级 pin + 完成通知**：内核把用户态 page 直接挂进 frag list，NIC DMA 直接读用户态内存。本篇讲清楚**用户态可见的完整协议**：如何启用、buffer 何时能改、完成通知怎么收、什么条件下内核会"偷偷退回拷贝模式"。

```
传统：  user buf ──copy_from_user──► skb linear data ──DMA──► NIC
ZC：    user page ──pin───────────► skb frag list ───DMA──► NIC
                                          │
                                          ▼ refcount 归零
                              errqueue 完成通知（ee_info..ee_data 区间）
```

## 要点（先记住结论）

1. **buffer 的释放边界从"sendmsg 返回"推迟到"errqueue 通知到达"**——sendmsg 返回只代表数据已提交给栈，page 还被 DMA 引用着。提前改写 = 发出去的是脏数据。
2. **完成通知走 `MSG_ERRQUEUE`**，不是普通数据队列：`recvmsg(fd, &msg, MSG_ERRQUEUE)` 读出 `sock_extended_err`，`ee_origin == SO_EE_ORIGIN_ZEROCOPY`、`ee_errno == 0`、区间为 `[ee_info, ee_data]`（按 send 调用序号编号，`uarg->id` 起始）。
3. **完成 ≠ 发送成功**：通知只表示"数据已离开 DMA 引用"。部分场景（网卡不支持 SG、retransmit）内核退回拷贝，此时通知带 `SO_EE_CODE_ZEROCOPY_COPIED` 标志——**数据是拷贝出去的，但 buffer 同样可以释放**。
4. **TCP 只有 `sk->sk_route_caps & NETIF_F_SG` 才走真 ZC**（tcp.c:1054）；不满足时 `uarg_to_msgzc(uarg)->zerocopy = 0`（tcp.c:1066）降级为拷贝+通知协议。
5. **区间合并**：相邻完成的 [lo, hi] 区间会在 errqueue 尾部合并成一个 skb（`skb_zerocopy_notify_extend`），64KB 通知开销摊到多个 send 上——这是 ZC 对小包不划算之外的第二层批量机制。
6. **收益拐点约 10KB**：官方基准里 1KB 时 ZC 慢 ~50%，64KB 快 3x，1MB 快 25x。HFT 交易报文（几百字节）**不要用**。

## 一、启用方式

两种，二选一：

```c
/* 方式 A：socket 级开关（之后所有 send 零拷贝） */
int opt = 1;
setsockopt(sockfd, SOL_SOCKET, SO_ZEROCOPY, &opt, sizeof(opt));

/* 方式 B：调用级标志（更精细） */
sendmsg(sockfd, &msg, MSG_ZEROCOPY);
```

注意 SO_ZEROCOPY 需要 `CAP_NET_ADMIN`（版本早期）或受 `optmem` 限额约束——因为 ZC 要 pin 用户 page，占用 socket 的 optmem 预算（RLIMIT_MEMLOCK 相关），超过限额 sendmsg 返回 `-ENOBUFS` 或静默退回拷贝模式。

## 二、完成通知协议（errqueue）

### 接收端样板代码

```c
int wait_zc_completion(int fd)
{
	struct msghdr msg = { .msg_flags = MSG_ERRQUEUE };
	char control[CMSG_SPACE(sizeof(struct sock_extended_err))];
	struct cmsghdr *cm;
	struct sock_extended_err *serr;

	msg.msg_control = control;
	msg.msg_controllen = sizeof(control);

	if (recvmsg(fd, &msg, MSG_ERRQUEUE) < 0)
		return -1;                          /* 队列空 */

	cm = CMSG_FIRSTHDR(&msg);
	if (!cm)
		return -1;
	serr = (void *)CMSG_DATA(cm);
	if (serr->ee_origin != SO_EE_ORIGIN_ZEROCOPY)
		return -1;                          /* 其他 errqueue 事件（如 ICMP） */

	printf("completed: %u..%u%s\n",
	       serr->ee_info, serr->ee_data,    /* [lo, hi] 闭区间 */
	       (serr->ee_code & SO_EE_CODE_ZEROCOPY_COPIED) ? " (copied)" : "");
	return 0;
}
```

每个 send 调用分配一个**单调递增序号**（`sk_zckey`，见 `msg_zerocopy_realloc` 的 `atomic_read(&sk->sk_zckey)`）。通知给出闭区间 `[ee_info, ee_data]`：区间内所有序号的 buffer 都已释放。应用侧维护一个 ring buffer 记录序号→buffer 指针的映射即可。

### 区间合并的内核实现

```c
// skbuff.c:1647 __msg_zerocopy_callback()（节选）
q = &sk->sk_error_queue;
spin_lock_irqsave(&q->lock, flags);
tail = skb_peek_tail(q);
if (!tail || SKB_EXT_ERR(tail)->ee.ee_origin != SO_EE_ORIGIN_ZEROCOPY ||
    !skb_zerocopy_notify_extend(tail, lo, len)) {
	__skb_queue_tail(q, skb);        // 合并失败 → 新通知 skb
	skb = NULL;
}
spin_unlock_irqrestore(&q->lock, flags);
sk_error_report(sk);                // → 唤醒等待者（poll EPOLLERR）
```

新通知先尝试**扩展 errqueue 尾部那条通知的 hi**（前提：`lo == 旧 hi + 1` 严格相邻），失败才入队新的。等待方式：`poll/epoll` 监听 `EPOLLERR`（`sk_error_report` 触发），而不是阻塞 recvmsg。

### "copied" 降级标志

`ee_code & SO_EE_CODE_ZEROCOPY_COPIED` 置位时表示这段数据实际是**拷贝后发送**的（网卡路径不支持 SG、或 TCP 重传路径重建 skb 等）。语义契约不变——buffer 依然可以释放——只是告知性能特征。**应用不应把它当错误处理**。

## 三、TCP 路径的准入判断（tcp.c 源码）

```c
// tcp.c:1054（tcp_sendmsg_locked 节选）
if (sk->sk_route_caps & NETIF_F_SG)
	uarg = msg_zerocopy_realloc(sk, size, skb_zcopy(skb));
...
if (!(sk->sk_route_caps & NETIF_F_SG))    // tcp.c:1063（发送包时二次确认）
	...
	uarg_to_msgzc(uarg)->zerocopy = 0;    // tcp.c:1066：标记"实为拷贝"
```

`sk_route_caps` 在路由查找时由网卡 feature 与路径协商得出。**loopback、不支持 SG 的虚拟设备**上 MSG_ZEROCOPY 永远走"拷贝+copied 通知"模式——测试时在 lo 上测不到真 ZC 收益，这是常见踩坑点。

数据入 skb 用的是 `skb_zerocopy_iter_stream()`（tcp.c:1233）：把用户 iov 逐段 `pin_user_pages` 后填进 skb 的 frag list，不是拷贝进 linear 区。

## 四、uarg 的聚合与 512KB 上限

`msg_zerocopy_realloc()`（skbuff.c:1577）允许**一个 uarg 服务多个 skb**（TCP 流式发送时同一 TSO 聚合内的多个分段共享一个 ubuf_info）：

```c
const u32 byte_limit = 1 << 19;       /* limit to a few TSO */
...
if (uarg_zc->len == USHRT_MAX - 1 || bytelen > byte_limit) {
	if (sk->sk_type == SOCK_STREAM)
		goto new_alloc;       // TCP：开新 uarg，旧的自然完成
	return NULL;             // UDP cork：放弃扩展
}
```

- `bytelen > 512KB`（约几个 TSO 大小）就切新 uarg——控制单条通知的区间粒度，也限制 pin 的 page 集合大小；
- `len` 以 **send 调用次数**计（USHRT_MAX=65535 个序号上限）；
- 序号连续性用 `sk_zckey` 校验：`(uarg_zc->id + uarg_zc->len) == next` 才扩展，否则新分配（中途有别的调用插队会断开区间）。

## 五、官方基准与适用条件

| 数据量 | 拷贝模式 | 零拷贝 | 收益 |
|--------|---------|--------|------|
| 1 KB | ~1 μs | ~1.5 μs | **-50%（更慢）** |
| 4 KB | ~1.5 μs | ~1.2 μs | +20% |
| 64 KB | ~5 μs | ~1.5 μs | +230% |
| 1 MB | ~80 μs | ~3 μs | +2500% |

| 条件 | 要求 |
|------|------|
| 网卡 | checksum offload + scatter-gather（NETIF_F_SG） |
| 协议 | TCP（4.14+）；UDP 需 4.14+ 且 cork 场景才有意义 |
| 数据量 | **> ~10KB 才正收益** |
| 内存 | optmem 限额内（pin page 计账），超限退回拷贝 |

小包变慢的三个来源：page pin/unpin 的固定成本（每次 send 数百 ns）、errqueue 通知的 skb 分配、应用侧收通知的额外 syscall。

## HFT 关联

| 场景 | 判定 |
|------|------|
| 交易报文（< 1KB） | **不用**——拷贝几百字节只要几十 ns，pin+通知的固定开销是它的 10 倍 |
| 行情转发/回放松重放（大 payload） | 值得——1MB 级 payload 25x 提升 |
| 与 io_uring 组合 | `IORING_OP_SEND_ZC`（6.0+）是同一机制的异步化，通知从 errqueue 变成第二个 CQE（`IORING_CQE_F_NOTIF`），批量场景比裸 MSG_ZEROCOPY 更优（见 [12-io-uring-net](../../chapter-12-io-uring-net/README.md)） |

## 衔接

本篇讲完用户态协议。下一篇 [13-03 LWN 深入](03-msg-zerocopy-lwn.md) 下沉到实现层：`ubuf_info_msgzc` 藏在 skb->cb 里的免分配技巧、page pin 的 refcount 生命周期、destructor 回调链如何驱动 errqueue，以及 MSG_ZEROCOPY 与 splice/sendfile/mmap+write 三条替代路线的横向对比。

## 代码自测

<details>
<summary>Q1：sendmsg(MSG_ZEROCOPY) 返回后立刻 memset 用户 buffer，会发生什么？</summary>

数据损坏。sendmsg 返回 ≠ DMA 完成——NIC 可能还在从你的 page 里读数据。正确做法：等 errqueue 上 `[ee_info, ee_data]` 覆盖该 send 序号的通知到达后才能复用/释放 buffer。ZC 把"buffer 生命周期管理"从同步语义变成了异步协议，这是它复杂度的根源。
</details>

<details>
<summary>Q2：收到 ee_code 带 SO_EE_CODE_ZEROCOPY_COPIED 的通知，应用该怎么处理？</summary>

正常处理，不当错误。它只是"性能公告"：这段数据实际是拷贝发送的（路由不支持 SG / TCP 重传重建了 skb）。语义上 buffer 照样可以释放——协议契约不变。只有想统计真实 ZC 命中率时才需要区分（io_uring 的 IORING_NOTIF_SQ_POLL 类似物是 REPORT_USAGE 选项）。
</details>

<details>
<summary>Q3：为什么在 loopback 上压测 MSG_ZEROCOPY 看不到收益？</summary>

`sk->sk_route_caps & NETIF_F_SG` 为假（lo 设备不支持 SG scatter-gather），tcp.c:1054 的准入判断不过，内核直接走拷贝路径 + `zerocopy = 0` 标记，最后给你一条 COPIED 通知。必须两台物理机、支持 SG 的网卡间测。
</details>

<details>
<summary>Q4：512KB 的 byte_limit 为什么注释写 "limit to a few TSO"?</summary>

TSO 单 skb 最大 64KB（GSO_MAX_SIZE 相关），512KB ≈ 8 个 TSO skb。一个 uarg 覆盖多个 skb 意味着这些 skb 的生命周期被绑在一起（refcount 共享）——全部发完才能通知。限制在"几个 TSO"粒度是为了：①单条通知覆盖的区间不至太大（失败重传影响面可控）；②pin 的 page 集合有上界（optmem 记账可控）。
</details>

<details>
<summary>Q5：通知为什么用 errqueue 而不是设计一个新的数据队列？</summary>

复用现有基础设施：①errqueue 的 skb 承载机制（cmsg + sock_extended_err）天然适合"带元数据的带外事件"；②poll/epoll 已监听 EPOLLERR，等待路径零新增；③errqueue 已有排队/唤醒逻辑（sk_error_report）。一个 socket 只有一条 errqueue 也没关系——区间合并（notify_extend）把同源通知压缩成少量 skb。
</details>
