# 13-04 — TCP_ZEROCOPY_RECEIVE：接收侧零拷贝（mmap 语义，v6.6 源码级）

> **对应 Rosen:** Ch11（recvmsg 拷贝模式）
> **内核源码路径:** `net/ipv4/tcp.c:1743-2205`（`tcp_mmap`、`tcp_zerocopy_receive`）

## 章节导航

| 上一篇 | 本篇 | 下一篇 |
|---|---|---|
| [13-03 ZC 实现内幕](03-msg-zerocopy-lwn.md) | **13-04 接收侧 ZC** | [13-05 SO_REUSEPORT](05-so-reuseport.md) |

## 本节讲什么

发送侧零拷贝（MSG_ZEROCOPY）讲完了，接收侧的对称问题是：`recvmsg()` 要把 skb 数据 `copy_to_user()` 进用户 buffer——每字节一次。Linux 在 TCP 上给出的答案是 `TCP_ZEROCOPY_RECEIVE`（4.18+）：**把 skb frag 里的 page 直接映射进用户地址空间**，应用读 mapping 就等于读接收队列，拷贝次数归零（严格说是归到"页对齐部分"）。

先纠正一个常见误解（本仓库旧版笔记也写错过）：**io_uring registered buffers 不是接收零拷贝**——它只是把 buffer 预 pin 好省掉每次操作的 pin 开销，recv 数据仍然从 skb memcpy 进用户 buffer。真正的"DMA 直达用户态"只有 AF_XDP / DPDK bypass。

## 要点（先记住结论）

1. **协议是两步的：先 `mmap(socket_fd)` 拿只读 VMA，再循环 `setsockopt(TCP_ZEROCOPY_RECEIVE)` 换页**。VMA 由 `tcp_mmap()`（tcp.c:1746）创建并打上 `tcp_vm_ops` 标记——后续 setsockopt 只接受带这个标记的 VMA（`find_tcp_vma` 校验，tcp.c:2041）。
2. **只映射整页**：`inq < PAGE_SIZE` 时直接返回 0（tcp.c:2101，`recv_skip_hint` 告知还差多少）；页边界外的"零头"用 `copybuf` 拷贝兜底（`tcp_zc_handle_leftover`）。**`inq <= copybuf_len` 时整个调用退化为纯拷贝**（tcp.c:2098 `receive_fallback_to_copy`）。
3. **每次调用默认做一次 TLB flush**：`zap_page_range_single`（tcp.c:2121）——把上一轮映射的旧页从 TLB 踢掉再 `vm_insert_pages` 新页。这是单次调用的固定成本，也是为什么高频小量调用反而亏。`TCP_RECEIVE_ZEROCOPY_FLAG_TLB_CLEAN_HINT` 可延迟 flush。
4. **消费进度 = `tp->copied_seq` 前进**：映射完成即认为已读（tcp.c:2200 `WRITE_ONCE(tp->copied_seq, seq)` + `tcp_cleanup_rbuf` 发 ACK）——**没有"应用确认"环节**，映射进去但没读也会被 ACK 掉，这是与 recvmsg 最大的语义差异。
5. **RFS 联动**：入口处 `sock_rps_record_flow(sk)`（tcp.c:2093）——每次 ZC 接收都更新全局流表，让 RFS 把后续包引导到消费线程所在 CPU（呼应 [13-01](01-scaling.md)）。
6. **映射是只读的**（`VM_WRITE → -EPERM`，tcp.c:1750），且页属于内核 skb——skb 被清理后 mapping 里是脏数据，**必须按序推进**。

## 一、用户态完整协议

```c
/* 步骤 1：在 socket fd 上 mmap 一个只读窗口 */
size_t mapping_len = 1 << 21;              /* 建议 2MB 起步 */
void *base = mmap(NULL, mapping_len, PROT_READ, MAP_SHARED,
                  tcp_fd, 0);              /* ← fd 是 TCP socket 本身 */

/* 步骤 2：循环"换页" */
uint32_t offset = 0;
for (;;) {
	struct tcp_zerocopy_receive zc = {
		.address  = (uintptr_t)(base + offset),
		.length   = mapping_len - offset,
		.copybuf_address = (uintptr_t)copybuf,   /* 零头拷贝缓冲 */
		.copybuf_len     = sizeof(copybuf),
	};
	if (setsockopt(tcp_fd, IPPROTO_TCP, TCP_ZEROCOPY_RECEIVE,
	               &zc, sizeof(zc)) < 0) {
		if (errno == EINVAL) { /* 没有 mappable 数据 */ }
		break;
	}
	/* zc.length  = 本次映射了多少字节（页对齐部分） */
	/* zc.recv_skip_hint = 建议下次跳过多少（下页对齐边界） */

	处理数据(base + offset, zc.length);

	offset += zc.length + zc.recv_skip_hint;
	if (offset >= mapping_len) offset = 0;   /* 环形复用窗口 */
}
```

## 二、源码主流程：`tcp_zerocopy_receive()`（tcp.c:2068）

### 入口三连判（决定了行为分支）

```c
if (address & (PAGE_SIZE - 1) || address != zc->address)
	return -EINVAL;                          // ① 必须页对齐

if (inq && inq <= copybuf_len)
	return receive_fallback_to_copy(sk, zc, inq, tss);  // ② 数据量≤copybuf → 全拷

if (inq < PAGE_SIZE) {
	zc->length = 0;
	zc->recv_skip_hint = inq;                // ③ 不满一页 → 等数据，别空转
	if (!inq && sock_flag(sk, SOCK_DONE))
		return -EIO;
	return 0;
}
```

三级判定总结：

| 条件 | 行为 |
|---|---|
| 地址未页对齐 | `-EINVAL`，协议错误 |
| `0 < inq <= copybuf_len` | 整段拷贝进 copybuf（拷贝比换页便宜） |
| `inq < PAGE_SIZE` | 返回 0，`recv_skip_hint` 提示等待 |
| `inq >= PAGE_SIZE` | 走映射 |

### 映射核心循环

```c
total_bytes_to_map = avail_len & ~(PAGE_SIZE - 1);   // 只取页对齐部分
if (total_bytes_to_map) {
	if (!(zc->flags & TCP_RECEIVE_ZEROCOPY_FLAG_TLB_CLEAN_HINT))
		zap_page_range_single(vma, address, total_bytes_to_map, NULL);  // TLB flush
	...
}
while (length + PAGE_SIZE <= zc->length) {
	...
	page = skb_frag_page(frags);              // 直接取 skb 的 frag page
	prefetchw(page);
	pages[pages_to_map++] = page;
	length += PAGE_SIZE;
	...
	if (pages_to_map == TCP_ZEROCOPY_PAGE_BATCH_SIZE || ...) {
		ret = tcp_zerocopy_vm_insert_batch(...);   // 32 页一批插入 VMA
		...
	}
}
```

- **批量 32 页**（`TCP_ZEROCOPY_PAGE_BATCH_SIZE`，tcp.c:2066）：一次 `vm_insert_pages` 映射一批，摊薄 PTE 锁开销；批边界对齐 skb 边界（"cannot unroll failed ops across skbs"——失败回滚不跨 skb）。
- **`prefetchw(page)`**：预取 page 结构体并准备写（`vm_insert_pages` 要写 page 的 mapcount）——热路径细节。
- **`vm_insert_pages` 半成功也要推进**：`tcp_zerocopy_vm_insert_batch_error` 里先算已映射页数、更新 seq/address，再决定 zap 重试或回滚 length（tcp.c:1941-1983）。

### 尾部结算

```c
copylen = tcp_zc_handle_leftover(zc, sk, skb, &seq, copybuf_len, tss); // 零头拷贝

if (length + copylen) {
	WRITE_ONCE(tp->copied_seq, seq);         // ← 消费进度推进
	tcp_cleanup_rbuf(sk, length + copylen);  // ← 清 skb + 发 ACK
	...
}
zc->length = length;
```

`tcp_zerocopy_set_hint_for_skb()`（tcp.c:1806）在 `recv_skip_hint` 里维护"到下一个可映射 frag 的距离"——应用据此跳过不可映射区（如 hdr 占的首页偏移）。

## 三、成本模型与拐点

| 项目 | 成本 |
|---|---|
| 每次 setsockopt syscall | ~0.5-1 μs |
| TLB flush（zap_page_range_single） | ~1 μs 级（广播 IPI 到用过该 mapping 的核） |
| vm_insert_pages 32 页 | ~1-2 μs |
| 换来的收益 | 省掉 N×PAGE_SIZE 的 memcpy（~60-80 GB/s → 每页 ~50ns） |

**单页净亏**（成本 > 收益），**一个 batch（32 页=128KB）净赚**。拐点约 32KB+ 且持续满页到达——典型命中场景是**镜像/日志/行情录制回放类大流量单连接**，而不是高频小报文。

## 四、与发送侧 MSG_ZEROCOPY 的机制对照

| | MSG_ZEROCOPY（发送） | TCP_ZEROCOPY_RECEIVE（接收） |
|---|---|---|
| 零拷贝对象 | 用户 page → skb frag（pin） | skb frag page → 用户 VMA（insert） |
| page 归属 | 用户态（内核借用） | 内核态（用户借用） |
| 生命周期管理 | refcount + errqueue 通知 | copied_seq 推进，无确认环节 |
| 固定成本 | pin/unpin + 通知 | TLB flush + PTE 批量插入 |
| 拐点 | ~10KB | ~32KB（一个 batch） |

发送侧问题是"用户内存物理地址要稳定"（pin），接收侧问题是"内核页要出现在用户页表里"（insert）——方向相反，但都是"用页表/引用操作换 memcpy"。

## 五、三方案真实对比（修正版）

| 方案 | 拷贝次数 | 真相 |
|---|---|---|
| 传统 recvmsg | 1 次 skb→user | 每字节线性，小包其实很快 |
| io_uring + registered buffers | **仍是 1 次** | 注册只是预 pin buffer，recv 内核态拷贝照旧；省的是 get_user_pages 不是 memcpy |
| TCP_ZEROCOPY_RECEIVE | 页对齐部分 0 次 | TLB flush + PTE 换页换来的；零头仍拷贝 |
| AF_XDP / DPDK | 0 次且不经协议栈 | RX 描述符直指用户态内存，真正 bypass |

## HFT 关联

| 场景 | 判定 |
|---|---|
| 行情接收（小包、突发） | **不用**——`inq < PAGE_SIZE` 直接返回 0，高频调用只剩 syscall + 判空开销；小包 recvmsg 的 memcpy 只有几十 ns |
| 行情录制/回放服务（大流量 TCP 单连接下发） | **命中**——持续满页到达时拐点之上收益稳定 |
| 追求极致接收 | AF_XDP（见 chapter-08）——零拷贝且免协议栈，是 TCP_ZEROCOPY_RECEIVE 的真上位替代 |
| 语义陷阱 | 映射即 ACK：内核认为"给你看了就是消费了"。策略层如果读一半崩溃，TCP 层已无法重传——**可靠性契约比 recvmsg 弱** |

## 衔接

至此收发两侧的"页级零拷贝"闭环。下一篇 [13-05 SO_REUSEPORT](05-so-reuseport.md) 换维度：不省拷贝，省的是**多个监听者之间的分发开销**——reuseport 组、eBPF 选择器与 `reuseport_select_sock_by_hash` 的 CPU 亲和联动（把 13-01 的 `SO_INCOMING_CPU` 收口补完）。

## 代码自测

<details>
<summary>Q1：为什么第一步是 mmap(socket_fd) 而不是普通匿名内存？</summary>

`tcp_mmap()`（tcp.c:1746）给 VMA 打上 `vm_ops = &tcp_vm_ops` 标记并强制只读。后续 setsockopt 里 `find_tcp_vma()`（tcp.c:2041）用这个标记校验"这是我为 TCP ZC 准备的窗口"——防止用户拿任意的匿名 mapping 来接收内核 page（那会破坏该 VMA 的匿名页语义）。tcp_vm_ops 本身是个空操作表：它只当"身份证"用，fault/缺失处理全靠 vm_insert_pages 预先填好 PTE。
</details>

<details>
<summary>Q2：TLB_CLEAN_HINT 标志解决什么问题？为什么默认不开？</summary>

默认行为是每次调用先 zap 旧映射（TLB flush）——安全但贵（flush 是广播 IPI）。如果应用保证"换页前已读完旧数据、且不跨线程读"，可置 `TCP_RECEIVE_ZEROCOPY_FLAG_TLB_CLEAN_HINT` 跳过主动 flush，只在 `vm_insert_pages` 返回 -EBUSY（PTE 还被别的核缓存着）时才补一次 zap（tcp.c:1941 的 error 路径）。默认关是因为乱序读写旧页的后果是读到脏数据——正确性换性能的开关。
</details>

<details>
<summary>Q3：零头数据（straggler）为什么不干脆也映射进去？</summary>

映射的粒度是 PTE = 页。零头要么不足一页（`inq < PAGE_SIZE`），要么是 frag 内偏移非零（`offset_frag != 0`，主循环里直接 break）。把半页数据凑成整页映射需要内核先搬移数据进对齐页——那就变成拷贝了，不如 `tcp_copy_straggler_data` 直接拷进 copybuf（应用栈上 buffer，一次 memcpy 到位），语义和成本都更干净。
</details>

<details>
<summary>Q4：io_uring registered buffers 省的到底是什么开销？</summary>

省的是**每次 I/O 的 pin/校验**：普通 recv 每 op 都要走 get_user_pages（查 VMA、pin page、记 accounting，数百 ns 固定成本）；注册过的 buffer 一次性 pin 好，后续 op 直接用。但数据路径不变：中断/软中断收进 skb，io_uring 的 io_recv 仍然 memcpy skb→user buffer。它是"syscall 与 pin 优化"，不是零拷贝——零拷贝的判定标准是有没有那次逐字节 memcpy。
</details>

<details>
<summary>Q5：TCP_ZEROCOPY_RECEIVE 与 recvmsg 在可靠性契约上的差异，对上层协议意味着什么？</summary>

recvmsg 语义：拷贝进用户 buffer 才算消费，应用崩了没拷的数据还在接收队列，对端可以重传（应用层可重连续传）。ZC 语义：`copied_seq` 在映射完成即推进、`tcp_cleanup_rbuf` 即 ACK（tcp.c:2200）——数据"交付"给了一个内核不追踪其读取进度的 mapping。应用读一半崩溃，那些数据永久丢失且对端以为已交付。上层协议如果自己有序列号/确认机制（行情序号），可以自愈重传；纯字节流协议要自己加校验点。
</details>
