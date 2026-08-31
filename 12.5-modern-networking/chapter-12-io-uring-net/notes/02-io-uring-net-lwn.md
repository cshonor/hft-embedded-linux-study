# 02 — io_uring 网络深水区：multishot、provided buffers 与 SEND_ZC

> **来源：** LWN io_uring 系列 + **v6.6 源码逐行核对**
> （`io_uring/net.c` 1522 行、`io_uring/notif.c`、`io_uring/io_uring.c`、
> `net/ipv4/tcp.c`、`net/ipv4/udp.c`、`include/uapi/linux/io_uring.h`）
> **对应 Rosen:** 无
> **内核版本：** multishot 5.19+，SEND_ZC 6.0+，机制细节以 **v6.6** 为准

## 文档概述

本篇拆 v6.6 网络收发的三个核心机制：**multishot recv**（一次提交管到底）、**provided buffers**（内核侧 buffer 池）、**SEND_ZC**（零拷贝发送 + 完成通知）。这三者组合是「行情收发」在 io_uring 上的标准形态。

姊妹篇分工：

| 文件 | 主题 | 与本篇的关系 |
|------|------|-------------|
| [01-io-uring-net.md](01-io-uring-net.md) | ring 结构、opcode 全景、setup flags | 01 是地基，本篇是三个上层机制 |
| [03-io-uring-vs-epoll.md](03-io-uring-vs-epoll.md) | 与 epoll 的选型 | 本篇的机制优势是 03 对比论的弹药 |

---

## 1. multishot recv：一次提交，持续完成

**传统模式的痛点**：收一个包 = 提交一个 RECV SQE + 消费一个 CQE + **再提交一个 RECV**……每个包两次 SQE 生命周期操作。

**multishot（5.19+）**：提交一次 RECV，内核**持续**完成它——每个到达的包产生一个 CQE（带 `IORING_CQE_F_MORE` 标志表示「还有更多」），请求本身长期存活：

```c
/* io_uring/net.c:590 —— prep 阶段 */
if (sr->flags & IORING_RECV_MULTISHOT) {
	...
	req->flags |= REQ_F_APOLL_MULTISHOT;
	/* Store the buffer group ... separately */
}
```

### 1.1 完成逻辑逐行：`io_recv_finish()`（net.c:632）

```c
static inline bool io_recv_finish(struct io_kiocb *req, int *ret,
				  struct msghdr *msg, bool mshot_finished, ...)
{
	cflags = io_put_kbuf(req, issue_flags);        /* 归还 provided buffer */

	if (msg->msg_inq && msg->msg_inq != -1)
		cflags |= IORING_CQE_F_SOCK_NONEMPTY;  /* ⭐ 队列里还有数据 */

	if (!(req->flags & REQ_F_APOLL_MULTISHOT)) {   /* 普通 recv：一次完成 */
		io_req_set_res(req, *ret, cflags);
		return true;
	}

	if (!mshot_finished) {                         /* multishot：还有后续 */
		if (io_fill_cqe_req_aux(req, ..., *ret,
					cflags | IORING_CQE_F_MORE)) {   /* ⭐ MORE 标志 */
			io_recv_prep_retry(req);           /* 重置请求准备下一次 */
			/* Known not-empty or unknown state, retry —— 立即重试 */
			if (cflags & IORING_CQE_F_SOCK_NONEMPTY || msg->msg_inq == -1)
				return false;              /* 不结束请求，继续收 */
			...
		}
	}
	/* buffer 池耗尽等异常 → 结束 multishot */
	...
}
```

**三个精妙设计**（全部 v6.6 源码可考）：

1. **`IORING_CQE_F_MORE`**：CQE flags 里的「后面还有」承诺。应用看到 MORE 就知道**不需要再提交**；没看到 MORE 就知道 multishot 结束了（比如 buffer 池干了），要重新提交
2. **`IORING_CQE_F_SOCK_NONEMPTY`**：来自 `msg_inq`——recvmsg 内核侧回填的「**接收队列剩余字节数**」。`io_recvmsg`/`io_recv` 都设 `msg.msg_get_inq = 1`（net.c:812/892）请求这个信息。**队列还有数据就不回去等事件，直接连着收**（上例 `return false` 的分支）——这是「批内聚合」的关键：同一批到达的 N 个包，1 次事件唤醒全收完
3. **`io_recvmsg_multishot_overflow()`**：收到的包比 buffer 大（`len < buflen` 且非 MSG_TRUNC）时 multishot 自毁——**provided buffer 模式下收不了超长包**，回退普通模式处理

### 1.2 与 epoll 循环的对照

```
epoll 一次唤醒：epoll_wait(返回 1 个事件) → recvmsg → [队列还有？epoll 不会再通知] → 睡
                                         └── 想再收要再等下一次 epoll_wait（LT/ET 语义坑）

multishot 一次唤醒：CQE(包1, MORE, NONEMPTY) → CQE(包2, MORE, NONEMPTY) → CQE(包3)
                    └────────── 同一批全收完才停 ──────────┘
```

**水平触发 vs 边缘触发**的经典难题（epoll LT 的 busy loop、ET 的漏读），在 multishot 里被 `SOCK_NONEMPTY` 信息直接消解——**内核告诉你要不要继续**，而不是让你猜。

---

## 2. provided buffers：`IOSQE_BUFFER_SELECT`

**问题**：multishot recv 的 buffer 谁出？应用不知道会收到多少包、每个多大，没法在 SQE 里写死 buffer 地址。

**答案（5.1+）**：应用预先把一池 buffer 交给内核（`IORING_OP_PROVIDE_BUFFERS`），SQE 设 `IOSQE_BUFFER_SELECT` 并标 buffer group id（`buf_group` 字段）：

```
应用：PROVIDE_BUFFERS(bgid=1, buf[0..N])        → 内核持有一池 buffer
应用：RECV + BUFFER_SELECT + buf_group=1        → 提交收包（不带地址！）
内核：挑 buf[i] ← 数据 → CQE(flags = BUFFER|<<16 编号i>>)  → 应用知道数据在哪个 buf
应用：处理完 → PROVIDE_BUFFERS 归还 buf[i]      → 循环
```

**CQE 里的 buffer id**：`IORING_CQE_F_BUFFER` 置位时，flags 高 16 位是 buffer 编号（`IORING_CQE_BUFFER_SHIFT`）。

**HFT 实践要点**：

1. **池大小 = 最大突发深度**。池干涸时 multishot 自毁（上节 overflow 逻辑），应用要能感知重建
2. **buffer 归还是显式的**——处理完必须还，否则池单调缩减
3. **与 registered buffers 是两套体系**（见 [01](01-io-uring-net.md) Q4）：provided 走「内核挑选」，registered 走「应用指定」
4. ring-mapped provided buffers（`IORING_REGISTER_PBUF_RING`，5.19+）是升级版：归还走用户态 ring 无系统调用，热路径推荐

---

## 3. 收方向零拷贝的现状：v6.6 是「半成品」

v6.6 上**没有**收方向的零拷贝（zerocopy receive，`io_uring/zcrx.c` 在后续内核才出现）。现状：

- multishot + provided buffers 收包，数据仍有一次「内核 skb → 用户 buffer」的拷贝
- 想消除这次拷贝，v6.6 的路径只有 **AF_XDP**（见 [chapter-06](../../chapter-06-af-xdp/)）——那是彻底的另一种 socket 模型

**选型直觉**：一次 1500 字节内的拷贝约 100ns 量级；若收包路径的其他开销（唤醒、系统调用）已被 io_uring 压掉，这次拷贝会变成新瓶颈——那时再考虑 AF_XDP，而不是一开始就上。

---

## 4. DEFER_TASKRUN：完成的「同核同上下文」执行

（setup flag 语义见 [01](01-io-uring-net.md) §2，这里补数据路径视角）

```
无 DEFER_TASKRUN：
  网卡中断（核 A）→ 软中断里跑完成回调 → CQE 写入
  用户线程（核 B）自旋看到 CQE → 处理
  问题：完成逻辑在核 A 污染的 cache 里跑；CQE 写入跨核可见性延迟

有 DEFER_TASKRUN：
  网卡中断（核 A）→ 只把 task_work 挂队列
  用户线程（核 B）调 io_uring_enter() → 依次执行挂着的完成回调 → CQE 写入
  效果：完成回调、CQE 写入、用户处理 全在核 B 的热 cache 里
```

v6.6 的实现锚点：`io_uring/io_uring.c` 里 `IORING_SETUP_DEFER_TASKRUN` 出现 10+ 处，核心是 `io_allowed_defer_tw_run()` 校验「当前线程 == 提交线程」+ `io_req_task_work_run()` 在 enter 上下文执行。

**HFT 推荐组合**：`SQPOLL + SINGLE_ISSUER + DEFER_TASKRUN` 中按场景取舍——单核全包（sqpoll 线程既提交又跑完成）最简；多核分离（用户线程 enter 跑完成）cache 更优。**两种都要实测延迟分布**，别信纸面。

---

## 5. SEND_ZC：零拷贝发送的 io_uring 形态（6.0+）

### 5.1 与 MSG_ZEROCOPY 的关系

`SO_ZEROCOPY` + `MSG_ZEROCOPY`（4.14+，见 [chapter-13](../../chapter-13-zerocopy-highperf/)）是 sendmsg 层的零拷贝：数据 pin 住页直接做 skb frag，**完成通知走 error queue**（`MSG_ERRQUEUE` 的 `SO_EE_ORIGIN_ZEROCOPY`），应用要另收一个 socket 事件——**通知机制和主路径割裂**。

`IORING_OP_SEND_ZC` 把同样的零拷贝数据路径装进 io_uring：**通知就是 ring 里的第二个 CQE**。

### 5.2 两次 CQE 的语义（v6.6 源码核对）

```c
/* io_uring/net.c:978 —— io_send_zc_prep() */
notif = zc->notif = io_alloc_notif(ctx);
notif->cqe.user_data = req->cqe.user_data;    /* 同 user_data！ */
notif->cqe.flags = IORING_CQE_F_NOTIF;        /* ⭐ 区分标志 */
```

- **CQE #1（立即）**：`res = 已提交到内核的字节数`，`flags` 无 NOTIF——「数据已交给协议栈」
- **CQE #2（延迟，带 `IORING_CQE_F_NOTIF`）**：`res = 0 或错误码`——「数据已真正离开」（skb 释放，页解 pin）

**应用侧规则**：同 `user_data` 收到 NOTIF CQE 前，**发送 buffer 不可复用/释放**。

### 5.3 完成通知的内核侧链路（`io_uring/notif.c`）

```c
static void io_tx_ubuf_callback(struct sk_buff *skb, struct ubuf_info *uarg, bool success)
{
	if (refcount_dec_and_test(&uarg->refcnt))
		__io_req_task_work_add(notif, IOU_F_TWQ_LAZY_WAKE);   /* 挂 task_work */
}
```

数据路径：`io_send_zc()`（net.c:1110 附近）→ 检查 `SOCK_SUPPORT_ZC` → `io_sg_from_iter()` 把用户页直接填进 `skb_shinfo()` 的 frags（**零拷贝**，`SKBFL_MANAGED_FRAG_REFS` 管理引用）→ skb 被网卡发出并释放时，`ubuf_info` 的析构回调（`io_tx_ubuf_callback`）触发 → task_work 投递 NOTIF CQE。

**协议支持**（v6.6 核对）：`SOCK_SUPPORT_ZC` 在 `tcp_init_sock()`（tcp.c:462）和 `udp_init_sock()`（udp.c:1582）**无条件置位**——TCP/UDP 都支持。UDP 的零拷贝有额外约束（页必须能作为 frag 组装，且历史上要求 MSG_MORE 场景才划算），TCP 是主场。

### 5.4 `IORING_SEND_ZC_REPORT_USAGE`：识别「假零拷贝」

```c
/* notif.c —— ext 回调里记账 */
if (success && !nd->zc_used)  WRITE_ONCE(nd->zc_used, true);
else if (!success && !nd->zc_copied) WRITE_ONCE(nd->zc_copied, true);
/* 完成 CQE 里：IORING_NOTIF_USAGE_ZC_COPIED 标志 = 内核拷贝了数据 */
```

内核某些路径（如 TCP 重传时数据已变化、fastopen）会**退化成拷贝**。带 `REPORT_USAGE` flag 后，NOTIF CQE 会告诉你这批数据是否真的零拷贝了——**监控零拷贝真实命中率的开关**。

---

## 6. 完整示例：multishot 收 + ZC 发的骨架

```c
/* ── 初始化 ── */
ring = io_uring_queue_init(4096, &p /* SQPOLL|DEFER_TASKRUN|... */);
io_uring_register_files(&ring, fds, NFD);
io_uring_register_buf_ring(&ring, &br, bgid=1);        /* ring-mapped 池 */

/* ── 收：一次提交，常驻 ── */
sqe = io_uring_get_sqe(&ring);
io_uring_prep_recv(sqe, md_fd, NULL, 0, 0);            /* 无 buffer 地址 */
sqe->flags |= IOSQE_BUFFER_SELECT | IOSQE_FIXED_FILE;
sqe->buf_group = 1;
io_uring_sqe_set_flags_multishot(sqe);                 /* IORING_RECV_MULTISHOT */
io_uring_submit(&ring);

/* ── 主循环 ── */
while (1) {
	io_uring_peek_batch_cqe(&ring, cqes, 64);          /* 批量收 CQE */
	for (each cqe) {
		if (cqe->flags & IORING_CQE_F_NOTIF) {         /* 发送完成通知 */
			release_tx_buffer(cqe->user_data);     /* 页可复用了 */
		} else if (cqe->flags & IORING_CQE_F_BUFFER) { /* 收到行情包 */
			buf = get_from_pool(cqe->flags >> 16);
			handle_md(buf, cqe->res);              /* 解析+决策 */
			maybe_send_order();                   /* ── 发：ZC ── */
			return_buf_to_pool(buf);              /* 还池 */
		}
	}
	io_uring_cq_advance(&ring, n);
}
```

**要点**：`res` 是实际字节数（不是 buffer 大小）；NOTIF CQE 和数据 CQE 用 flags 区分（同 user_data）；buffer 还池是显式的。

---

## 7. HFT 要点

1. **multishot + provided buffers + SOCK_NONEMPTY = 收包的标准形态**：一次提交常驻、批内全收、无需重提——比「每包一提」的 epoll/传统 io_uring 少一个数量级的 SQE 管理
2. **`IORING_CQE_F_MORE` 消失 = multishot 死了**（多半是 buffer 池干），监控这个事件并重建
3. **SEND_ZC 的 buffer 生命周期以 NOTIF CQE 为界**——提前复用 buffer = 数据损坏，这是最隐蔽的坑
4. **收方向零拷贝 v6.6 不存在**，别被「io_uring 全零拷贝」的说法带偏；真需求上 AF_XDP
5. **UDP + ZC 要实测**：fastopen/重传路径会退化拷贝，用 `REPORT_USAGE` 监控真实命中率
6. **DEFER_TASKRUN 的收益是 cache 局部性**不是系统调用数——多核部署时按「完成与处理同核」设计拓扑

---

## 8. 代码自测

<details>
<summary>Q1：multishot recv 的 CQE 里 flags 同时有 MORE 和 SOCK_NONEMPTY，分别承诺什么？</summary>

- **`IORING_CQE_F_MORE`**：这个请求还活着，**还会有后续 CQE**——应用不需要（也不应该）重新提交 RECV
- **`IORING_CQE_F_SOCK_NONEMPTY`**：socket 接收队列里**还有数据**——源自 `msg_inq`（recvmsg 的 in-queue 字节数，io_uring 侧 `msg_get_inq=1` 请求，net.c:812/892）。内核用它决定**立即重收**（net.c:654：NONEMPTY 或 inq==-1 时 `return false` 不退出收包循环）

一个面向应用（别重提），一个面向内核（接着收）。
</details>

<details>
<summary>Q2：为什么 provided buffer 池干涸会杀死 multishot？</summary>

`io_recv_finish()` 的 multishot 分支里，`io_fill_cqe_req_aux()` 失败（无法发「MORE」CQE，通常因 CQ 满或 buffer 不可用）时走到底部：`*ret = IOU_STOP_MULTISHOT`，请求终结。

原因：multishot 的持续完成依赖「每个包都有 buffer 可装 + 每个 CQE 都能发出 MORE 承诺」。池干了就装不了下一个包——与其静默丢包，**不如杀掉 multishot 让应用显式感知**（最后一个 CQE 无 MORE），走普通路径重新提交并补池。

设计哲学：异步机制的失败要**可观测**，不能「看起来在跑实际在丢」。
</details>

<details>
<summary>Q3：SEND_ZC 的两个 CQE 中，哪个之后才能复用发送 buffer？</summary>

**第二个（带 `IORING_CQE_F_NOTIF` 的）**。

- CQE #1（无 NOTIF）：只表示数据已拷贝/引用进协议栈（`res` = 提交字节数）——但零拷贝模式下数据**还在你的 buffer 里**（页被 pin 成 skb frag）
- CQE #2（NOTIF）：skb 已释放、`ubuf_info` 析构回调已跑（`io_tx_ubuf_callback`）、页解 pin——现在 buffer 真正自由了

提前复用 = 网卡 DMA 读到的是新数据 = 报文损坏，且大概率不报错。这是 SEND_ZC 第一坑。
</details>

<details>
<summary>Q4：为什么说 `IORING_CQE_F_SOCK_NONEMPTY` 化解了 epoll LT/ET 的经典难题？</summary>

epoll 的困境：

- **LT（水平触发）**：队列非空就一直通知——收慢了 busy loop，收快了每包一次 epoll_wait 返回
- **ET（边缘触发）**：只通知一次新到达——必须循环读到 EAGAIN，漏读就饿死

根源：epoll 只告诉你「事件发生了」，不告诉你「队列还剩多少」。

multishot 的答案：每个 CQE 附带 `msg_inq`（内核精确知道队列剩余字节数），并据此**内核自己决定继续收还是回等**（net.c:654 的分支）。应用的语义退化为「来一个 CQE 处理一个」——既不 busy loop 也不会漏，**状态机在内核里**。

这是「信息完备性」解决问题的教科书案例：epoll 难题不是调度问题，是信息问题。
</details>

<details>
<summary>Q5：UDP socket 用 SEND_ZC，什么情况下会退化成拷贝？怎么发现？</summary>

退化场景（skb 生命周期超出 buffer 安全期时的内核自救，如 fastopen 合并、某些分片路径）：内核把 frag 数据拷走，`ubuf_info` 提前析构。

发现手段：`IORING_SEND_ZC_REPORT_USAGE` flag（sqe->ioprio 里设置）——`io_tx_ubuf_callback_ext()` 记账 `zc_used/zc_copied`，NOTIF CQE 的 res 带 `IORING_NOTIF_USAGE_ZC_COPIED`（notif.c:18）。

**HFT 实操**：上线初期带 REPORT_USAGE 跑一段，确认 ZC 命中率；命中率低说明你的报文尺寸/协议路径不适合 ZC（小报文 ZC 收益本来就小——页 pin/管理开销可能超过一次拷贝）。
</details>

---

## 导航

- **上一篇：** [01-io-uring-net.md](01-io-uring-net.md) — ring 结构与 opcode 全景
- **下一篇：** [03-io-uring-vs-epoll.md](03-io-uring-vs-epoll.md) — 与 epoll 的选型对比
- **相关：** [chapter-13-zerocopy-highperf/](../../chapter-13-zerocopy-highperf/) MSG_ZEROCOPY/sendmsg 体系（SEND_ZC 的前身） · [chapter-06-af-xdp/](../../chapter-06-af-xdp/) 收方向零拷贝的真答案 · [chapter-12 的 01](01-io-uring-net.md) §2 DEFER_TASKRUN 的 setup 语义
- **章节主页：** [README](../README.md)
