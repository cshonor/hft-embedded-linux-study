# 03 — sk_buff 生命周期：从 slab 到释放

> **对应 Rosen:** Ch1/Ch11（sk_buff 是核心数据结构）
> **内核源码路径:** `include/linux/skbuff.h`、`net/core/skbuff.c`
> **内核版本:** 以 v6.6 为准，符号与函数体已核对源码

## 文档概述

`sk_buff` 是 Linux 网络栈里唯一贯穿全程的数据结构。Rosen 用了一整章讲它，但讲的是 2.6 时代的形态。本篇按 v6.6 重写，重点补三件原笔记没讲的事：

1. **三个 slab cache**（不止一个 `skbuff_head_cache`，v6.6 还有 `skbuff_small_head`）
2. **fclone 机制**——为什么"克隆"有时比"分配"快得多
3. **释放函数的语义差异**——选错会让你的丢包监控说谎

本篇与兄弟篇的分工：

| 篇 | 讲 sk_buff 的哪一面 |
|----|-------------------|
| **03（本篇）** | 分配 / 克隆 / 释放的**生命周期** |
| [04](04-sk-buff-xdp-buff.md) | 与 `xdp_buff` 的**表示转换** |
| [02](02-txrx.md) | 驱动侧的**释放路径选择** |
| [chapter-01](../../chapter-01-net-stack-architecture/notes/01-net-stack-architecture.md) | `len` / `data_len` **线性区陷阱** |

---

## 一、结构全景：四指针 + 三偏移

```
        head                 data                  tail              end
         |                    |                     |                 |
         v                    v                     v                 v
  +------+--------------------+---------------------+-----------------+------+
  | headroom | <- 已 push 的协议头 -> |   线性数据区     |   tailroom      | shinfo|
  +------+--------------------+---------------------+-----------------+------+
         |                    |                                       |
         |<--- skb_headroom() ->|                                     |
         |<----------- skb_headlen() = len - data_len --------------->|
         |<------------------ skb->end - skb->head ------------------->|
                                                                        ^
                                                          skb_shared_info
                                                          （frags[] / gso 等）

  非线性区（data_len > 0）：由 skb_shared_info->frags[] 指向的页面承载
```

| 字段 | 含义 | 注意 |
|------|------|------|
| `head` / `end` | 整个缓冲区的起止（**固定**） | 分配后不变 |
| `data` / `tail` | 当前"有效数据"的起止（**会动**） | 各层 push/pull 就是在动这两个 |
| `len` | **总**长度（线性 + 非线性） | ⚠️ 不是线性区长度 |
| `data_len` | **非线性区**长度 | 线性区 = `len - data_len` |
| `mac_header` / `network_header` / `transport_header` | 各层起始偏移 | 可能为 `~0U` 表示未设置 |
| `users` | skb 头自身的引用计数 | `skb_get()` / `kfree_skb()` |
| `cloned` | **数据区**是否被共享 | 与 `users` 是两套计数！ |
| `fclone` | 是否来自 fclone cache | `SKB_FCLONE_ORIG` / `_CLONE` / `_UNAVAILABLE` |
| `cb[48]` | 控制块，**每层复用同一块内存** | 跨层传递会互相踩 |
| `sk` | 归属的 socket | 影响 socket 内存记账与释放时序 |

### ⚠️ 两套引用计数，别混

这是最容易搞错的地方：

- **`users`**：`struct sk_buff` **这个结构体本身**被引用几次。`skb_get()` 加 1，`kfree_skb()` 减 1，归零才释放 skb 头。
- **数据区引用**：在 `skb_shared_info->dataref` 里。`skb_clone()` **不增加 `users`**，它新建一个 skb 头、共享数据区、把 `dataref` 加 1，并把原 skb 和克隆 skb 都打上 `cloned` 标记。

所以 `skb_clone()` 出来的 skb，`users == 1`，但 `cloned == 1`。**一个包可以同时被 `users` 和 `dataref` 管着，两个都得归零才真正消失。**

### `cb[48]` 是个共享的 48 字节

`cb` 是"每层自定义"的暂存区——IP 层用完 TCP 层接着用，**同一块 48 字节**。它的用处是省内存（避免每层都加字段），代价是**跨层传递时如果你往 `cb` 里写了东西，下一层会覆盖掉**。写 tc-BPF / kprobe 时往 `cb` 塞自定义元数据，一定要确认中间没有别的层会踩它。

顺带一个安全细节：`skbuff_head_cache` 是用 `kmem_cache_create_usercopy()` 创建的，白名单区间恰好是 `cb`（`offsetof(struct sk_buff, cb)` 起、长度 `sizeof(cb)`）——也就是说**只有 `cb` 允许被 copy 到用户态**，其余字段不行。这是历史上一批 `sk_buff` 信息泄漏漏洞之后的加固。

---

## 二、分配：三个 slab cache

原笔记只写了 `kmem_cache_alloc(skbuff_head_cache)`。v6.6 实际有**三个**（`net/core/skbuff.c` 的 `skb_init()`）：

```c
void __init skb_init(void)
{
	skbuff_cache = kmem_cache_create_usercopy("skbuff_head_cache",
					sizeof(struct sk_buff), 0,
					SLAB_HWCACHE_ALIGN|SLAB_PANIC|
					  FLAG_SKB_NO_MERGE,
					offsetof(struct sk_buff, cb),
					sizeof_field(struct sk_buff, cb),
					NULL);
	skbuff_fclone_cache = kmem_cache_create("skbuff_fclone_cache",
					sizeof(struct sk_buff_fclones), 0,
					SLAB_HWCACHE_ALIGN|SLAB_PANIC, NULL);
	/* usercopy should only access first SKB_SMALL_HEAD_HEADROOM bytes.
	 * struct skb_shared_info is located at the end of skb->head,
	 * and should not be copied to/from user.
	 */
	skb_small_head_cache = kmem_cache_create_usercopy("skbuff_small_head",
					SKB_SMALL_HEAD_CACHE_SIZE, 0,
					SLAB_HWCACHE_ALIGN | SLAB_PANIC,
					0, SKB_SMALL_HEAD_HEADROOM, NULL);
	skb_extensions_init();
}
```

| cache | 装什么 | 什么时候用 |
|-------|--------|-----------|
| `skbuff_head_cache` | 单个 `struct sk_buff` | 普通分配 |
| `skbuff_fclone_cache` | `struct sk_buff_fclones`（**一对** skb） | 预知马上要 clone 时（如 TCP 发送） |
| `skbuff_small_head` | **小数据区**（`SKB_SMALL_HEAD_CACHE_SIZE`） | 小包（如 TCP ACK、控制报文），避免为几十字节单独分配页面 |

`skbuff_small_head` 是 6.x 才加的，常见资料（包括 Rosen）都没提。它对 HFT 有意义：**下单报文通常就几十到几百字节，正好落在这个 cache 里**，省掉一次页面分配。

### 分配函数怎么选

| 函数 | 用在哪 | 特点 |
|------|--------|------|
| `alloc_skb(size, gfp)` | 通用 | 最基础 |
| `netdev_alloc_skb(dev, len)` | 驱动收包（老接口） | 带 `NET_SKB_PAD` headroom |
| `napi_alloc_skb(napi, len)` | 驱动收包（**现代推荐**） | 可能从 NAPI 的 per-CPU 缓存拿，更快 |
| `build_skb(data, frag_size)` | 已有数据区时 | 只分配 skb 头，数据区复用 |
| `napi_build_skb(data, frag_size)` | **XDP_PASS 后接进协议栈** | 见 [04 篇](04-sk-buff-xdp-buff.md) |
| `skb_alloc_from_fclone` | 内部 | 走 fclone cache |

**关键区别**：`alloc_skb()` 要同时分配「skb 头」+「数据区」两笔；`build_skb()` / `napi_build_skb()` **只分配 skb 头**，数据区用现成的（比如 page_pool 里的页面）。后者是 XDP_PASS 能"零拷贝接进协议栈"的原因。

---

## 三、fclone：为"马上要克隆"准备的一对 skb

TCP 发送必须保留一份副本用于重传，所以每发一个包都要 clone。如果每次 clone 都走一次 slab 分配，成本很高。

fclone 的解法：**一次性分配一对** `struct sk_buff`（`sk_buff_fclones`），一个标记为 `SKB_FCLONE_ORIG`，另一个预留为 `SKB_FCLONE_CLONE`。

```c
/* net/core/skbuff.c:673 */
skb->fclone = SKB_FCLONE_ORIG;
```

之后调用 `skb_clone()` 时，如果原 skb 是 `SKB_FCLONE_ORIG` 且那个预留的兄弟还没被用掉，**直接拿来用，完全不走 slab**：

```c
/* net/core/skbuff.c:1009 附近，__kfree_skb / skb_clone 的分支 */
case SKB_FCLONE_UNAVAILABLE:   /* 普通 skb，释放时回收头 */
case SKB_FCLONE_ORIG:          /* orig 释放时，把兄弟的克隆标记清掉 */
default: /* SKB_FCLONE_CLONE */ /* clone 释放时，把兄弟还给可用状态 */
```

**含义**：fclone 路径下 clone 的成本接近零——只是一次结构体填写，没有任何分配器交互。

**代价**：每个 fclone 分配占的内存是单个 skb 的两倍。所以只有**明确知道要 clone** 的路径（TCP 发送）才用它；收包路径不用。

**一个反直觉的细节**（见 `napi_consume_skb` 源码）：

```c
/* if SKB is a clone, don't handle this case */
if (skb->fclone != SKB_FCLONE_UNAVAILABLE) {
	__kfree_skb(skb);
	return;
}
skb_release_all(skb, SKB_CONSUMED, !!budget);
napi_skb_cache_put(skb);
```

**clone 出来的 skb 用不了 per-CPU 缓存**，只能走 `__kfree_skb()`。所以"克隆很便宜"是真的，但"克隆的释放也很便宜"是假的——**省在分配，亏在释放**。

---

## 四、四种"复制"语义完全不同

| 函数 | 复制 skb 头 | 复制数据区 | 可写 | 典型用途 |
|------|------------|-----------|------|---------|
| `skb_clone()` | ✅ 新建 | ❌ **共享**（`dataref++`） | ❌ 只读 | TCP 重传副本、抓包分流 |
| `skb_copy()` | ✅ | ✅ 全部 | ✅ | 需要修改数据 |
| `pskb_copy()` | ✅ | ✅ **仅线性区**（frags 仍共享） | 部分 | 只需改协议头 |
| `skb_copy_expand()` | ✅ | ✅ 全部 + **加大 headroom** | ✅ | 要往前加封装头 |

⚠️ **对 clone 出来的 skb 写数据会污染原 skb**——它们共享同一块数据区。如果确实要写，内核要求先 `skb_cow()`（copy-on-write）确认独占。这是驱动/BPF 开发里的经典 bug 来源。

⚠️ **`pskb_copy()` 只拷线性区**：如果包有分片（`data_len > 0`），`frags[]` 仍然共享。只改头部时没问题，改 payload 就出事了。

---

## 五、生命周期状态机

```
                    alloc_skb() / napi_build_skb()
                              │
                              v
                    ┌──────────────────┐
                    │   users = 1      │
                    │   data_len = 0   │
                    │   cloned = 0     │
                    └──────────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
   skb_get()             skb_clone()          GRO 合并 / IP 分片重组
   users++           新建头 + dataref++         → data_len > 0（非线性）
        │                cloned = 1                 │
        │                     │                     │
        └─────────────────────┼─────────────────────┘
                              │
                  各层 push/pull 改 data/tail
                  skb_pull()  skb_push()  skb_put()  skb_trim()
                              │
                              v
                  释放（四选一，语义不同）
        ┌─────────────┬───────────────┬──────────────────┐
        │             │               │                  │
  kfree_skb()   consume_skb()   napi_consume_skb()  __kfree_skb()
  计入丢包      正常投递         NAPI 上下文        内部实现
  reason=       reason=         reason=CONSUMED    reason=
  NOT_SPECIFIED CONSUMED        + per-CPU 缓存     NOT_SPECIFIED
        │             │               │
        └─────────────┴───────────────┘
                      │
              users-- → 0 ?
              dataref-- → 0 ?
                      │
                      v
        释放数据区（page / page_pool 回收）
        + 回收 skb 头（slab 或 per-CPU 缓存）
```

### `kfree_skb_reason`：丢包原因（5.15+）

这是定位"包在哪一层被丢"最直接的信号。内核给每次 `kfree_skb` 附带一个 `enum skb_drop_reason`，比如 `SKB_DROP_REASON_SOCKET_FILTER`、`SKB_DROP_REASON_UDP_CSUM_ERROR`、`SKB_DROP_REASON_NO_SOCKET`、`SKB_DROP_REASON_SOCKET_RCVBUFF` 等。

```bash
# 按原因聚合丢包
bpftrace -e 'tracepoint:skb:kfree_skb { @[args->reason] = count(); }'

# 带调用栈，直接定位到函数
bpftrace -e 'tracepoint:skb:kfree_skb /args->reason > 2/ { printf("%s\n", kstack); }'

# 老内核（4.x / 5.0-5.14）没有 reason，只能退回到 dropwatch
dropwatch -l kas
```

**`SKB_CONSUMED` 是正常投递**，不算丢包。所以上面的过滤条件 `reason > 2` 就是在排除正常情况。（具体枚举值随版本变化，用 `cat /sys/kernel/tracing/events/skb/kfree_skb/format` 确认。）

---

## 六、`sk` 指针与 socket 的耦合

`skb->sk` 把包和 socket 关联起来，带来两个 HFT 相关的后果：

**① 发送方向的内存记账**
TCP 发出的 skb 在收到 ACK 之前不能释放，`sk->sk_wmem_alloc` 记账。如果发送缓冲耗尽，`tcp_sendmsg()` 会阻塞（非阻塞则返回 `EAGAIN`）。**这是发包侧的隐性背压**——`SO_SNDBUF` 调太小时，你的 send 会莫名其妙地"卡"。

**② 接收方向的释放时序**
包进 socket 接收队列后，skb 仍持有 `sk` 引用。**socket 关闭时不等于 skb 立即释放**——队列里的包要等应用读完（或 socket 真正销毁）才释放。

这意味着：**行情程序退出时，接收队列里积压的包会延迟释放**，短时间内 RSS 不降反升。高频场景下如果进程反复重启，会看到内存"阶梯式"上涨。

`skb_orphan()` 用来切断这个关联（把 `skb->sk = NULL` 并归还记账），常用于转发路径——转发出去的包不应该继续占着收包 socket 的账。

---

## 七、观测

```bash
# slab 使用量（三个 cache 分别多少）
cat /proc/slabinfo | grep -E "skbuff"

# 按原因统计丢包（5.15+）
bpftrace -e 'tracepoint:skb:kfree_skb { @[args->reason] = count(); }'

# 谁在分配 skb（调用栈聚合）
bpftrace -e 'kprobe:__alloc_skb { @[kstack] = count(); }'

# 克隆频率（TCP 发送路径会很高）
bpftrace -e 'kprobe:skb_clone { @[comm] = count(); }'

# socket 接收队列积压（对应上面的 ②）
ss -ulnp            # Recv-Q 列
cat /proc/net/udp   # tx_queue / rx_queue 列
```

---

## HFT 要点

- **XDP_DROP 省的不只是 skb 分配，还有整条协议栈**：原笔记只提了"分配开销 ~100-200 cycles"，但分配只是起点。真正的收益是 `XDP_DROP` 让包在 `napi_build_skb()` **之前**就结束。
- **`skbuff_small_head` 对小包有直接收益**：下单报文几十~几百字节，正好命中这个 cache，省一次页面分配。6.x 才有。
- **fclone 省在分配、亏在释放**：clone 本身接近零成本，但 clone 出来的 skb 释放时**走不了 per-CPU 缓存**。
- **`users` 和 `dataref` 是两套计数**：`skb_clone()` 后 `users == 1` 但 `cloned == 1`。只看一个会误判包是否还在被引用。
- **别往 `cb` 里塞跨层的东西**：48 字节被每层复用，下一层会覆盖。
- **`SO_SNDBUF` 太小会让 send 隐性阻塞**：`sk_wmem_alloc` 记账耗尽时，非阻塞 send 返回 `EAGAIN`，看起来像"随机的发送失败"。
- **进程退出时队列里的包不会立即释放**：高频重启会看到内存阶梯式上涨，这不是泄漏。
- **`kfree_skb_reason` 是第一手的丢包定位手段**：比 `dropwatch` 精确得多，能直接给出原因枚举。

## 与 Rosen 3.x 的差异

| Rosen 3.x（2.6 时代） | 现在（5.x/6.x） |
|----------------------|----------------|
| 一个 `skbuff_head_cache` | **三个** cache（多了 fclone 专用和 `skbuff_small_head`） |
| 未提 fclone | `skbuff_fclone_cache` + `SKB_FCLONE_ORIG/CLONE` 三态 |
| 释放主要讲 `kfree_skb` / `dev_kfree_skb` | `napi_consume_skb(skb, budget)` 走 per-CPU 缓存 |
| 丢包只能看计数器 | 5.15+ `kfree_skb_reason` 带原因枚举 |
| `sk_buff` 有 `dst`、`sp`（sec_path）等字段 | 大量字段已外移到 `skb_ext`（按需分配，省内存） |
| 无 usercopy 白名单 | `kmem_cache_create_usercopy` 只允许 `cb` 拷到用户态 |
| `len` vs `data_len` 区分不明显 | 非线性区（GRO / 分片）普遍存在，必须区分 |

### 补充：`skb_ext` 按需外移

现代内核把一些不常用字段（如 `dst`、`sec_path`、bridge 元数据、TLS 上下文）移出 `struct sk_buff`，放进按需分配的 `skb_ext`。**代价是一次额外的指针跳转，收益是 `struct sk_buff` 本身更小、cache 命中率更高。**

对 HFT 的含义：`struct sk_buff` 在热路径上被反复读写，它越小越好。这也解释了为什么内核愿意为 `cb`（48 字节固定）保留在结构体内，而把其他东西挪走——`cb` 是热路径每包都碰的。

---

## 代码自测

<details>
<summary>Q1：你对一个刚 <code>skb_clone()</code> 出来的 skb 调用 <code>skb_put()</code> 往尾部追加数据，发现原始 skb 的数据也被改了。为什么？怎么修？</summary>

<b>答：</b>因为 `skb_clone()` <b>只新建 skb 头，数据区是共享的</b>（`skb_shared_info->dataref` 加 1，两个 skb 都打上 `cloned` 标记）。`skb_put()` 写的是那块共享数据区，所以原 skb 一起变了。

这是个<b>静默</b> bug——不报错、不崩溃，只是数据莫名其妙被改。

修法：写之前先确认独占。
```c
if (skb_cow(skb, needed_headroom))
        goto drop;      /* 无法独占（拷贝失败），按丢包处理 */
```
`skb_cow()` 会检查 `skb_cloned()`，如果数据区被共享就做一次真正的拷贝（`skb_copy` 语义），确保之后可写。

同类陷阱：`pskb_copy()` <b>只拷线性区</b>，`frags[]` 仍共享。只改协议头时没问题，改 payload 就会踩到。
</details>

<details>
<summary>Q2：你查 <code>/proc/slabinfo</code>，发现 <code>skbuff_head_cache</code> 和 <code>skbuff_fclone_cache</code> 都有大量对象，但你不记得自己写的代码 clone 过任何包。谁在用？</summary>

<b>答：</b>多半是 <b>TCP 发送路径</b>。

TCP 必须为每个发出的包保留重传副本，所以 `tcp_write_xmit()` 走的就是 fclone 路径：分配 `SKB_FCLONE_ORIG`，随即 `skb_clone()` 出 `SKB_FCLONE_CLONE`（因为兄弟是预分配的，这次 clone <b>完全不走 slab</b>）。

纯 UDP 行情接收路径不应该有 fclone——如果你看到 UDP 进程也在大量使用 `skbuff_fclone_cache`，查一下是不是有什么抓包/镜像/Netfilter 模块在做 clone 分流。

验证：
```bash
bpftrace -e 'kprobe:skb_clone { @[kstack] = count(); }'
```
栈里出现 `tcp_*` 就是正常的 TCP 重传副本。
</details>

<details>
<summary>Q3：你的 TCP 下单程序在高负载下 <code>send()</code> 偶尔返回 <code>EAGAIN</code>，但 <code>SO_SNDBUF</code> 明明还有空间。<code>ss -tin</code> 也没看到异常。问题在哪？</summary>

<b>答：</b>你看到的是 `SO_SNDBUF` 的<b>名义大小</b>，但限制你的很可能是 <b>TSQ（TCP Small Queues）</b>——它在 `SO_SNDBUF` 之外，另外限制<b>单个连接在途未确认的字节数</b>。

```bash
sysctl net.ipv4.tcp_limit_output_bytes
```

TSQ 的本意是防止单个连接把 qdisc 和硬件队列塞满（bufferbloat）。对 HFT 的下单场景，它表现为："缓冲区明明够，但 send 就是发不出去"。

另外两条容易混淆的：`SO_SNDBUF` 的<b>实际可用值会小于你 setsockopt 设的值</b>（内核按 `net.ipv4.tcp_wmem` 的上限截断，且有开销记账的折扣），以及 `sk_wmem_alloc` 记账包含了已发出但未 ACK 的数据——那部分你看不见但确实占着额度。

排查顺序：
1. `ss -tinp` 看 `snd_wnd` / `cwnd` / `pacing_rate`——是不是被流控或 pacing 限住
2. `sysctl net.ipv4.tcp_limit_output_bytes`——TSQ 是否太小
3. `cat /sys/class/net/eth0/queues/tx-0/byte_queue_limits/inflight`——BQL 是否已经顶到硬件队列上限
4. 确认 `TCP_NODELAY` 已开（否则 Nagle 会让你以为"发不出去"）
</details>

---

→ 前一篇：[02 驱动与内核的契约](02-txrx.md)
→ 后一篇：[04 sk_buff ↔ xdp_buff](04-sk-buff-xdp-buff.md)
→ 相关：[chapter-01 线性区陷阱](../../chapter-01-net-stack-architecture/notes/01-net-stack-architecture.md) · [chapter-04 page_pool](../../chapter-04-page-pool/)
