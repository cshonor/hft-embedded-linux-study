# 01 — io_uring 网络：环形队列与网络 opcode 全景

> **对应 Rosen:** 无（io_uring 是 5.1+ 的产物）
> **核对源码：** v6.6 `include/uapi/linux/io_uring.h`、`io_uring/io_uring.c`、`io_uring/net.c`
> （v6.6 起 io_uring 代码已从 `fs/io_uring.c` 迁入 `io_uring/` 目录，网络操作在 `io_uring/net.c`）

## 文档概述

本篇是 io_uring 的**机制基础**：两个环形队列怎么工作、setup flags 各自改变什么、网络相关 opcode 全表、SQE 的 flags 与链接模式。收发路径的 v6.6 新机制（multishot/provided buffers/SEND_ZC）在 [02](02-io-uring-net-lwn.md)，与 epoll 的选型在 [03](03-io-uring-vs-epoll.md)。

---

## 1. 基本结构：SQ / CQ 两个环形队列

```
用户态共享内存（mmap 一次性映射，IORING_FEAT_SINGLE_MMAP）：

  SQ ring（索引数组）──→ SQE 数组（每项 64 字节，uapi_io_uring.h:93）
       │  用户写 SQE，更新 tail
       ▼
  内核消费 SQE，执行操作
       │
       ▼
  CQ ring（每项 16 字节 struct io_uring_cqe）
       内核写 CQE，更新 tail
       用户消费 CQE，更新 head
```

```c
/* include/uapi/linux/io_uring.h —— SQE 的关键字段 */
struct io_uring_sqe {
	__u8	opcode;		/* IORING_OP_* */
	__u8	flags;		/* IOSQE_* */
	__u32	fd;		/* 直接 fd 或 registered file index */
	union { __u64 off; __u64 addr; };	/* offset / 参数 */
	__u32	len;		/* buffer 长度 */
	__u64	user_data;	/* 原样回到 CQE —— 关联请求的唯一句柄 */
	...
};
```

**设计核心**：`user_data` 是请求与完成的**唯一关联**——SQE 里放什么，CQE 里就回来什么。这使批量提交 + 乱序完成成为一等公民。

**一个容易误解的点**：SQ 不是「队列」而是「索引环」。SQE 数组本身是普通数组，SQ ring 里存的是 SQE 下标；提交者写 SQE 数组 + 更新 SQ tail，消费者（内核）从 SQ head 取。这样**同一批 SQE 可以被 SQPOLL 线程和 io_uring_enter 混合消费**而无需复制。

### 1.1 两种消费模式

| 模式 | 谁消费 SQ | 提交系统调用 |
|---|---|---|
| 默认 | `io_uring_enter(ENTER)` 时同步消费 | 每批 1 次 |
| SQPOLL | 内核 sqpoll 线程持续轮询 SQ tail | **0 次**（用户写完 SQE 内存屏障即可） |

---

## 2. setup flags：v6.6 全表（`uapi_io_uring.h:140-191`）

```c
#define IORING_SETUP_IOPOLL		(1U << 0)	/* 存储 IO 轮询（网络无关） */
#define IORING_SETUP_SQPOLL		(1U << 1)	/* 内核 SQ 轮询线程 */
#define IORING_SETUP_SQ_AFF		(1U << 2)	/* sq_thread_cpu 有效 */
#define IORING_SETUP_CQSIZE		(1U << 3)	/* 应用定义 CQ 大小 */
#define IORING_SETUP_CLAMP		(1U << 4)	/* ring 大小钳制 */
#define IORING_SETUP_ATTACH_WQ		(1U << 5)	/* 共享 io-wq */
#define IORING_SETUP_R_DISABLED		(1U << 6)	/* 启动时禁用（后 enable） */
#define IORING_SETUP_SUBMIT_ALL		(1U << 7)	/* 出错也继续提交剩余 */
#define IORING_SETUP_COOP_TASKRUN	(1U << 8)	/* 协作式 task_work */
#define IORING_SETUP_TASKRUN_FLAG	(1U << 9)	/* IORING_SQ_TASKRUN 标志 */
#define IORING_SETUP_SQE128		(1U << 10)	/* SQE 扩到 128 字节 */
#define IORING_SETUP_CQE32		(1U << 11)	/* CQE 扩到 32 字节 */
#define IORING_SETUP_SINGLE_ISSUER	(1U << 12)	/* 单线程提交（优化前提） */
#define IORING_SETUP_DEFER_TASKRUN	(1U << 13)	/* ⭐ 完成在 enter 时跑 */
#define IORING_SETUP_NO_MMAP		(1U << 14)	/* 用户自管 ring 内存 */
#define IORING_SETUP_REGISTERED_FD_ONLY	(1U << 15)	/* ring fd 也注册 */
#define IORING_SETUP_NO_SQARRAY		(1U << 16)	/* 无 SQ 索引数组（SQPOLL） */
```

**HFT 关注的三个**：

1. **`SQPOLL`**：零系统调用提交。代价是独占一个 CPU（`sq_thread_idle` 毫秒后休眠，唤醒有延迟——低延迟场景设大让它永不睡，等于花钱买确定性）
2. **`DEFER_TASKRUN`**（6.1+）：完成回调（task_work）不在中断/软中断上下文跑，而是**推迟到本线程调 `io_uring_enter()` 时**执行——消除了「中断打扰 + IPI 打爆缓存」的问题，代价是要求 `SINGLE_ISSUER`。对延迟敏感路径，这是 v6.6 的推荐组合（详见 [02](02-io-uring-net-lwn.md) §4）
3. **`SINGLE_ISSUER`**（6.0+）：声明只有一个线程提交，让内核省去跨线程同步——是 DEFER_TASKRUN 的前提，独立使用也有收益

---

## 3. 网络 opcode 全表（v6.6 `uapi_io_uring.h:194-242` 核对）

| Opcode | 引入版本 | 对应系统调用 | 网络语义要点 |
|---|---|---|---|
| `IORING_OP_ACCEPT` | 5.5 | `accept4()` | 支持 multishot（5.19+） |
| `IORING_OP_CONNECT` | 5.6 | `connect()` | 异步 TCP 握手 |
| `IORING_OP_SEND` | 5.6 | `send()/sendto()` | |
| `IORING_OP_SENDMSG` | 5.6 | `sendmsg()` | |
| `IORING_OP_RECV` | 5.6 | `recv()/recvfrom()` | 支持 multishot（5.19+）+ provided buffers |
| `IORING_OP_RECVMSG` | 5.6 | `recvmsg()` | 同上 |
| `IORING_OP_SHUTDOWN` | 5.11 | `shutdown()` | |
| `IORING_OP_PROVIDE_BUFFERS` | 5.1 | — | 向内核提供 buffer 池（见 [02](02-io-uring-net-lwn.md) §3） |
| `IORING_OP_REMOVE_BUFFERS` | 5.1 | — | 回收 buffer 池 |
| `IORING_OP_SOCKET` | 5.19 | `socket()` | 连 socket 创建都能异步 |
| `IORING_OP_SEND_ZC` | 6.0 | — | **零拷贝发送**（见 [02](02-io-uring-net-lwn.md) §5） |
| `IORING_OP_SENDMSG_ZC` | 6.0 | — | sendmsg 版零拷贝 |
| `IORING_OP_MSG_RING` | 5.18 | — | ring 间发消息（跨线程唤醒） |
| `IORING_OP_URING_CMD` | 5.19 | — | 子系统自定义命令（网络侧：`SO_URING_CMD` 等） |

**能力完整性**：从 socket 创建（`SOCKET`）到关闭（`CLOSE`），连接建立（`CONNECT`/`ACCEPT`）到收发（`SEND*`/`RECV*`）到断连（`SHUTDOWN`），**全生命周期都能走 ring**——一个热路径线程可以做到与 syscall 层完全解耦。

---

## 4. SQE flags：一次提交的编排能力

```c
/* uapi_io_uring.h:123-135 */
#define IOSQE_FIXED_FILE	/* fd 字段是 registered file 索引，省 fd 查找 */
#define IOSQE_IO_DRAIN		/* 排空之前的所有请求（顺序屏障） */
#define IOSQE_IO_LINK		/* 链接下一个 SQE：成功才执行 */
#define IOSQE_IO_HARDLINK	/* 硬链接：无论如何都执行 */
#define IOSQE_ASYNC		/* 强制走异步（io-wq），不试同步快路径 */
#define IOSQE_BUFFER_SELECT	/* ⭐ 从 provided buffer 池选 buffer（网络收包核心） */
#define IOSQE_CQE_SKIP_SUCCESS	/* 成功不发 CQE（链接中间步骤） */
```

**HFT 用法示例——「收包 → 处理 → 发单」的原子链**：

```
SQE1: RECV  (IOSQE_IO_LINK)      ← 从行情 socket 收一个包
SQE2: URING_CMD/自定义处理        ← 收到后执行（链接保证顺序）
SQE3: SEND  (交易报文)            ← 处理完成自动发出
```

链接模式把「多步依赖操作」压进一次提交——**内核侧保证执行顺序，用户线程一次 enter 搞定**，比应用层状态机（回调链）省上下文切换和唤醒延迟。

注意 `IOSQE_ASYNC` 是**语义开关不是性能开关**：默认情况下 io_uring 对每个请求先试同步快路径（数据就绪就直接完成，省掉整个异步机制），`ASYNC` 强制走 io-wq 工作线程——**低延迟路径恰恰要避免它**（线程池唤醒是几十 µs 级的灾难）。

---

## 5. registered 体系：fd 与 buffer 的预注册

| 机制 | 注册什么 | 省掉什么 |
|---|---|---|
| registered files（`IORING_REGISTER_FILES`） | fd 数组 | 每个操作里的 `fget()` 引用计数原子操作 |
| registered buffers（`IORING_REGISTER_BUFFERS`） | iovec 数组（内核 pin 页） | 每次读写的 `get_user_pages()`/页表遍历 |
| provided buffers（`PROVIDE_BUFFERS` opcode） | 动态 buffer 池 | 收包时的「谁来出 buffer」问题（见 [02](02-io-uring-net-lwn.md) §3） |

**三者不是一回事**：前两个是**静态资源预注册**（省固定开销）；provided buffers 是**动态 buffer 生命周期管理**（收多少包不知道，内核从池里挑，用完还回来）。HFT 行情收包的正确组合是 `registered files + provided buffers`（+ 可选 fixed buffers 用于 SEND_ZC）。

---

## 6. HFT 配置模板（v6.6）

```c
struct io_uring_params p = {
	.flags = IORING_SETUP_SQPOLL		// 零系统调用提交
	       | IORING_SETUP_SQ_AFF		// 绑核（与行情线程错开）
	       | IORING_SETUP_SINGLE_ISSUER	// 单提交者（DEFER_TASKRUN 前提）
	       | IORING_SETUP_DEFER_TASKRUN,	// 完成推迟到 enter 时跑
	.sq_thread_cpu = 3,			// 独占核
	.sq_thread_idle = ~0U,			// 永不休眠（用 CPU 换确定性）
};
io_uring_setup(4096, &p);
// 后续：
// io_uring_register_files(...)      — 注册行情/交易 socket
// io_uring_register_buffers(...)    — 注册发送 buffer
// 提交收包：RECV + multishot + BUFFER_SELECT（见 02）
```

**权衡表**：

| 配置 | 延迟 | CPU 代价 | 适用 |
|---|---|---|---|
| 默认（enter 提交） | 基线 | 低 | 管理通道 |
| SQPOLL + idle=∞ | 最低 | 1 核 100% | 行情/交易热路径 |
| DEFER_TASKRUN（无 SQPOLL） | 低 | 低 | 中间态：多 ring 场景省核 |
| SQPOLL + DEFER_TASKRUN | 需实测 | 1 核 | 组合语义要测（taskwork 推迟到 sqpoll 线程） |

---

## 7. HFT 要点

1. **零系统调用是真的，但有前提**：SQPOLL 模式下提交侧完全无 syscall；完成侧仍要自旋读 CQ（或 busy poll）。「零 syscall」≠「零成本」，买的是**路径确定性**（无进出内核的 cache 污染）
2. **`sq_thread_idle` 设成永不到期**——sqpoll 线程一旦睡眠，唤醒路径（几十 µs）会撕碎延迟分布的右尾
3. **registered files/buffers 是必做项**：fd 引用计数和 GUP 的开销在微秒级路径上不可忽略
4. **`IOSQE_ASYNC` 是毒药**（热路径上）：io-wq 唤醒是几十 µs 级；数据就绪时默认的同步快路径才是低延迟正道
5. **链接模式（LINK）编排「收→算→发」**：依赖操作一次提交，内核保序

---

## 8. 代码自测

<details>
<summary>Q1：SQPOLL 模式下，用户态提交一个 RECV 的完整动作序列是什么？</summary>

1. 从 SQ 数组头部取下一个空闲 SQE 槽位（`sq->sqe_tail` 管理）
2. 填写字段：opcode=IORING_OP_RECV, fd, addr=buffer, len, user_data
3. 写内存屏障（`io_uring_smp_store_release`——liburing 封装）
4. 更新 SQ ring 的 tail 索引
5. **什么都不用再调**——sqpoll 线程在轮询 tail，看到新条目就消费

完成侧：自旋读 CQ ring tail（与内核共享的内存），有新 CQE 就处理，更新 head。

全程 0 系统调用。对比默认模式：第 4 步之后还要 `io_uring_enter()` 通知内核（每批 1 次 syscall）。
</details>

<details>
<summary>Q2：为什么 v6.6 推荐 DEFER_TASKRUN？它解决了什么问题？</summary>

**问题**：io_uring 的完成回调（task_work）默认由**收到数据的中断上下文**投递并执行——软中断里跑完成逻辑会：

1. 打断正在跑的用户线程（IPI + 缓存污染）
2. 完成路径的执行核不确定（哪个核收到中断就在哪跑）

**DEFER_TASKRUN 的答案**：完成回调不在中断上下文执行，而是挂到队列上，**等本线程调 `io_uring_enter()` 时才统一执行**（`io_uring_core.c:3320` 附近：`io_allowed_defer_tw_run(ctx)` 校验「正在跑的就是提交线程」）。

效果：完成逻辑和用户逻辑**同核同上下文**，缓存局部性最好；代价是必须 `SINGLE_ISSUER`（多线程提交会破坏「等 enter 时跑」的假设）。

注意与 SQPOLL 的组合：sqpoll 模式下用户连 enter 都不调，task_work 由 sqpoll 线程代跑——语义变成「完成在 sqpoll 线程执行」，要实测确认符合预期。
</details>

<details>
<summary>Q3：`IORING_SETUP_SQE128` / `CQE32` 是什么？网络操作需要吗？</summary>

SQE 从 64 扩到 128 字节、CQE 从 16 扩到 32 字节（`IORING_SETUP_CQE32` 时 `res64` 可用）。

- `CQE32`：网络侧**确实有用**——后续内核的收方向零拷贝（`io_uring/zcrx.c`，v6.6 尚无此文件）会用 extra CQE 数据回传 buffer 信息；v6.6 上普通网络操作用不上
- `SQE128`：主要给 `URING_CMD` 类带大参数的命令用；普通网络 opcode 塞 64 字节足够

结论：v6.6 的网络热路径**不需要**这两个 flag；开 SQE128 反而让 SQE 数组的 cache footprint 翻倍。
</details>

<details>
<summary>Q4：registered buffers 和 provided buffers 有什么区别？为什么收包用后者、发送用前者？</summary>

**registered buffers**（`io_uring_register_buffers`）：静态注册一组 iovec，内核 pin 页建立映射。用在 `READ_FIXED`/`WRITE_FIXED`/`SEND_ZC` 的 `IORING_RECVSEND_FIXED_BUF`——**发送方知道要发什么**，buffer 是应用选定的。

**provided buffers**（`PROVIDE_BUFFERS` opcode，配 `IOSQE_BUFFER_SELECT`）：应用把一池 buffer 交给内核，**收包时内核从池里挑一个**装数据，CQE 里带回 buffer id（`IORING_CQE_F_BUFFER` + buffer 编号在 flags 高 16 位）——**接收方不知道会收到多少包、多大**，必须让内核动态分配。

发送 = 应用有确定性（用 registered/fixed）；接收 = 不确定性（用 provided 池 + 应用侧还池）。
</details>

<details>
<summary>Q5：一个 ring 吃满（CQ 溢出）会发生什么？</summary>

CQ 满时新完成事件无处写——默认**丢事件**（`IORING_FEAT_NODROP` 之前的行为是 drop；有 NODROP 后内核会挂起提交侧，enter 返回 `-EBUSY`，并在溢出时发 `IORING_CQE_FLAG_OVERFLOW` 标记的兜底 CQE）。

对 HFT 的启示：

1. CQ 大小要按「最大突发完成数 × 处理延迟」配置（`IORING_SETUP_CQSIZE`），宁可大
2. 消费 CQE 必须及时——自旋模式下这是自然保证；休眠模式下要小心
3. CQ 溢出 = 延迟分布右尾的隐形来源，监控 `io_uring_enter` 的 EBUSY 返回

</details>

---

## 导航

- **下一篇：** [02-io-uring-net-lwn.md](02-io-uring-net-lwn.md) — v6.6 网络收发深水区：multishot recv、provided buffers、SEND_ZC 零拷贝链路
- **相关：** [chapter-13-zerocopy-highperf/](../../chapter-13-zerocopy-highperf/) 零拷贝体系全景 · [chapter-12 的 03](03-io-uring-vs-epoll.md) 与 epoll 的选型
- **章节主页：** [README](../README.md)
