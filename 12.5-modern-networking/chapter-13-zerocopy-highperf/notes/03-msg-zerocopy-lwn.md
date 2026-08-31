# 13-03 — MSG_ZEROCOPY 实现内幕：ubuf_info、page pin 与引用计数生命周期

> **对应 Rosen:** Ch11
> **内核源码路径:** `net/core/skbuff.c:1540-1760`、`net/ipv4/tcp.c`、`include/linux/skbuff.h`

## 章节导航

| 上一篇 | 本篇 | 下一篇 |
|---|---|---|
| [13-02 MSG_ZEROCOPY](02-msg-zerocopy.md) | **13-03 实现内幕** | [13-04 TCP_ZEROCOPY_RECEIVE](04-tcp-zero-copy-recv.md) |

## 本节讲什么

上一篇讲了用户态协议；本篇沿 LWN 对该机制的分析路线下沉到实现：**内核如何做到"免拷贝挂接用户 page"，代价（pin、refcount、通知）分别发生在哪条代码路径上**，以及四条零拷贝路线（sendfile / splice / mmap+write / MSG_ZEROCOPY）的机制对比。读完应该能回答："一次 64KB 的 ZC send，内核里额外发生了什么"。

```
tcp_sendmsg_locked()
   │ sock_owned_by_user ✓
   ▼
msg_zerocopy_realloc()           skbuff.c:1577 ── 复用或新建 uarg
   │
   ├─ 复用路径：bytelen ≤ 512KB 且 sk_zckey 连续 → uarg->len++
   └─ 新建路径
        ▼
msg_zerocopy_alloc()             skbuff.c:1540
   ├─ sock_omalloc(sk, 0)        ← 零长 skb，只为借它的 cb
   ├─ uarg = (void *)skb->cb     ← BUILD_BUG_ON 保证放得下
   ├─ mm_account_pinned_pages()  ← RLIMIT_MEMLOCK 记账
   └─ uarg->ubuf.callback = msg_zerocopy_callback
        ▼
skb_zerocopy_iter_stream()       tcp.c:1233 ── pin_user_pages → frag list
        ▼
   ...NIC DMA 用户 page...
        ▼
skb 释放 → skb_zcopy_clear() → msg_zerocopy_callback()   skbuff.c:1700
   └─ refcount_dec_and_test == 0 → __msg_zerocopy_callback()  skbuff.c:1647
                                    └─ errqueue 通知 + sk_error_report()
```

## 要点（先记住结论）

1. **`ubuf_info_msgzc` 不单独分配，藏在零长 skb 的 `cb` 数组里**（skbuff.c:1540 `msg_zerocopy_alloc`）——一次 `sock_omalloc(sk, 0, ...)` 同时解决了 uarg 存活（借 skb 的析构机制）和通知 skb（完成时直接把宿主 skb 塞进 errqueue）两个问题。
2. **一个 uarg 的生命周期 = 它覆盖的所有 skb 的生命周期之并**：`net_zcopy_get/put` 维护 refcount，最后一个引用释放才触发完成通知。这就是"buffer 释放边界"在内核侧的对应物。
3. **page pin 走 `mm_account_pinned_pages`**，受 RLIMIT_MEMLOCK 约束——ZC 的内存代价不是"拷贝多少"而是"pin 多少页 + 一个 uarg"。
4. **失败路径也要遵守协议**：sendmsg 中途出错时 `msg_zerocopy_put_abort()` 走 refcount 归零但 `uarg->len == 0` 的分支——`__msg_zerocopy_callback` 里 `if (!uarg->len) goto release` 保证**不排队假通知**。
5. **四条零拷贝路线本质是两类**：sendfile/splice 是"页缓存→socket"的内核内部管道（用户态根本不接触数据）；mmap+write 与 MSG_ZEROCOPY 是"用户态→socket"方向（用户先拥有数据）。HFT 转发场景数据在用户态加工过，只能走第二类。

## 一、`msg_zerocopy_alloc()`：借 skb 的 cb 免分配

```c
// skbuff.c:1540（完整逻辑）
static struct ubuf_info *msg_zerocopy_alloc(struct sock *sk, size_t size)
{
	struct ubuf_info_msgzc *uarg;
	struct sk_buff *skb;

	WARN_ON_ONCE(!in_task());

	skb = sock_omalloc(sk, 0, GFP_KERNEL);     // ① 零长 skb
	if (!skb)
		return NULL;

	BUILD_BUG_ON(sizeof(*uarg) > sizeof(skb->cb));  // ② cb 放得下 uarg
	uarg = (void *)skb->cb;
	uarg->mmp.user = NULL;

	if (mm_account_pinned_pages(&uarg->mmp, size)) { // ③ MEMLOCK 记账
		kfree_skb(skb);
		return NULL;
	}

	uarg->ubuf.callback = msg_zerocopy_callback;
	...
}
```

设计精妙之处：
- **零长 skb 是宿主也是通知载体**：uarg 需要一个"活到 DMA 结束"的容器——内核里现成的引用计数容器就是 skb。而这个 skb 完成使命后直接被 `__msg_zerocopy_callback` 塞进 errqueue（装 `sock_exterr_skb` 也正好用它的 cb），一物两用，**从 alloc 到通知零额外分配**。
- **`sizeof(*uarg) > sizeof(skb->cb)` 编译期断言**：`skb->cb` 是 48 字节的 per-layer 私有区（TCP 用它放 `tcp_skb_cb`，这里放 `ubuf_info_msgzc`），越界会在编译期爆炸而不是运行期踩内存。

`sock_omalloc` 分配的 skb 不计入 socket 发送队列内存（`sk_omem_alloc` 单独记账，受 `sysctl_optmem_max` 约束）——所以 ZC 的内存预算是 optmem 而不是 wmem。

## 二、refcount 生命周期：通知的唯一触发器

```
msg_zerocopy_alloc           refcnt = 1（宿主 skb 持有）
   │
   ├─ msg_zerocopy_realloc 复用（TCP 流）→ net_zcopy_get()：每个共享 skb +1
   │
   ▼
每个挂了 uarg 的 skb 发送完毕被释放
   → skb_zcopy_clear(skb) → uarg->ubuf.callback = msg_zerocopy_callback()
       │
       ▼ skbuff.c:1700
void msg_zerocopy_callback(struct sk_buff *skb, struct ubuf_info *uarg,
			   bool success)
{
	uarg_zc->zerocopy = uarg_zc->zerocopy & success;  // ① 任一失败 → COPIED
	if (refcount_dec_and_test(&uarg->refcnt))
		__msg_zerocopy_callback(uarg_zc);          // ② 最后一个引用
}
```

- **① `success` 参数**：callback 的调用方告知"这段数据是否真的零拷贝发出"。TCP 重传时如果走重建 skb 的路径（拷贝数据），对应回调 `success=false`，uarg 的 `zerocopy` 位被清零——最终通知带 `SO_EE_CODE_ZEROCOPY_COPIED`。多个 skb 共享 uarg 时只要有一个非 ZC 路径，整条通知都标 COPIED（保守语义）。
- **② `refcount_dec_and_test`**：通知只在最后一个引用释放时发生。应用侧看到的"一条 [lo,hi] 通知"，内核侧可能对应十几次 dec。

**TCP 的 clone 增引用**：TSO/GSO 分段、重传克隆（`skb_clone`）都会 `net_zcopy_get`——同一个 uarg 被 clone 链上每个 skb 引用，全部消化完才算完。这也是 `msg_zerocopy_realloc` 里 TCP 特有 `net_zcopy_get(uarg)` 的原因（流式追加时新旧边界）。

## 三、中止路径：不出假通知

```c
// __msg_zerocopy_callback 开头（skbuff.c:1647 之后）
mm_unaccount_pinned_pages(&uarg->mmp);

/* if !len, there was only 1 call, and it was aborted
 * so do not queue a completion notification
 */
if (!uarg->len || sock_flag(sk, SOCK_DEAD))
	goto release;
```

sendmsg 中途失败（比如拷贝 iov 时 EFAULT）走 `msg_zerocopy_put_abort()`：把 refcount 清到 0 触发回调，但 `uarg->len == 0`（还没有成功的 send 序号挂在上面）→ 直接 release，**不排队通知**。否则应用会收到一个从未 send 成功的序号的"完成"，把错误的 buffer 标记为可释放。

`SOCK_DEAD` 检查：socket 已关闭时也没必要通知了，直接释放。

## 四、pin 语义：`mm_account_pinned_pages` 与两条限制

1. **RLIMIT_MEMLOCK**：pin 的 page 总量计入进程 memlock 配额（与 mlock 共享），超限 `sendmsg` 返回 `-ENOBUFS`（tcp_sendmsg_locked 里 uarg == NULL 的处理）。容器/cgroup 环境里这个 limit 常是 64KB 默认值——ZC 大流量会先撞这里。
2. **页不可迁移的代价**：pin 期间该 page 不能被 swap/迁移（DMA 需要**物理地址稳定**）。长连接大流量 ZC 会让内存碎片整理（compaction）在这些页上失败——对延迟敏感机器上属于慢性病，量级取决于并发 pinned 集合大小。

## 五、四条零拷贝路线对比

| 路线 | 数据路径 | 用户态接触数据 | 需要 fd 源 | 适用 |
|---|---|---|---|---|
| `sendfile(out, in)` | page cache → socket，splice 管道 | ❌ | 输入是文件 | 静态文件服务 |
| `splice(pipe ↔ socket)` | page cache ↔ pipe ↔ socket，页引用搬移 | ❌ | 任意两个带管道的 fd | 内核内转发（代理） |
| `mmap(file) + write(sock)` | page cache 映射进用户态 → write 仍拷贝进 skb | ✅（只读） | 输入是文件 | 需要读数据但不改 |
| `MSG_ZEROCOPY` | 用户 page → skb frag（pin） | ✅（读写，异步释放） | 无 | **用户态生成/加工的数据** |

关键差异：前三条处理"数据源头在内核（页缓存）"的场景；MSG_ZEROCOPY 处理"数据源头在用户态"——HFT 的订单报文是**序列化/加密后生成的**，页缓存里没有现成品，必须走第四条。

splice/sendfile 的成本模型也完全不同：它们没有 pin/unpin、没有完成通知（页缓存 page 天生稳定引用），代价是管道 fd 的 syscall 次数——所以"文件→socket"场景永远优先前三者。

## 六、一次 64KB ZC send 的完整成本清单

| 步骤 | 代码位置 | 成本 |
|---|---|---|
| uarg 分配/复用 | skbuff.c:1540/1577 | ~100ns（含 sock_omalloc；复用时为 0） |
| pin 16 个 page | `skb_zerocopy_iter_stream` → pin_user_pages | ~500ns-1μs（fault 会计账） |
| frag 填充 | tcp.c:1233 | ~100ns |
| NIC DMA | 网卡读用户 page | 0（这正是收益来源：省 64KB memcpy ≈ 4-6μs） |
| skb 释放回调链 | 每段 TSO 一次 dec + test | ~几十 ns × 段数 |
| 通知排队+唤醒 | skbuff.c:1647 | ~200ns（含可能的区间合并） |
| 应用收通知 | recvmsg(MSG_ERRQUEUE) | ~1μs syscall |

对比：拷贝模式只有一项"64KB memcpy ≈ 4-6μs"，但它是**每字节线性**的；ZC 的固定开销 ~2-3μs，**与数据量无关**——这就是拐点在 ~10KB 的原因，两边都是可推导的。

## HFT 关联

- **订单流不用**（< 1KB，固定开销 > 线性拷贝），但**策略回放/行情录制回放**（GB 级文件重放）走"mmap 录制文件 + sendfile/splice"路线更优——源头在页缓存。
- **发布/订阅转发器**（收行情 → 加工 → 发给策略进程）是第二类场景的真命中：数据在用户态被改写过，且单条大（整本书的深度快照可达 MB 级）。
- 监控真实 ZC 命中率：统计带 `SO_EE_CODE_ZEROCOPY_COPIED` 的通知占比——持续偏高说明路由没走 SG 网卡或重传率高。
- 与 io_uring SEND_ZC 对比见 [12-02](../../chapter-12-io-uring-net/notes/02-io-uring-net-lwn.md)：同一 uarg 机制，通知从 errqueue 换成 NOTIF CQE，省掉收通知 syscall。

## 衔接

发送侧零拷贝到此完整。下一篇 [13-04 TCP_ZEROCOPY_RECEIVE](04-tcp-zero-copy-recv.md) 补接收侧：`tcp_zerocopy_receive()`（tcp.c:2068 起）如何把接收队列映射进用户态只读 mapping，以及它和"接收路径零拷贝"（AF_XDP / DPDK bypass）的本质区别。

## 代码自测

<details>
<summary>Q1：为什么 uarg 要藏在零长 skb 的 cb 里，而不是 kmalloc 一个独立结构？</summary>

需要一个生命周期与"DMA 引用集合"精确同步、且有现成引用计数机制的容器。独立 kmalloc 结构得自建 refcount + 释放钩子 + 错误路径清理；而 skb 天生具备：①引用计数（skb_zcopy_get/put 直接复用）；②cb 私有区刚好放得下 uarg（编译期断言保证）；③完成时这个宿主 skb 直接变身为 errqueue 通知（装 sock_exterr_skb），零额外分配。一次 `sock_omalloc(sk, 0)` 解决全部三件事。
</details>

<details>
<summary>Q2：TSO 分段后一个 uarg 被 8 个 skb 引用，通知什么时候发？refcount 怎么变？</summary>

`msg_zerocopy_realloc` 聚合时（TCP 流式）每追加一段 `net_zcopy_get` +1；每个分段 skb 被网卡确认释放时 `msg_zerocopy_callback` 里 -1。最后一个 -1（refcount_dec_and_test 为真）才进 `__msg_zerocopy_callback` 排通知。所以应用收到一条通知 = 内核里可能发生了一串 dec——通知的"区间语义"（[id, id+len-1]）就是为这种多 skb 聚合设计的。
</details>

<details>
<summary>Q3：应用 sendmsg 返回 -EFAULT（iov 地址非法），会误发一条完成通知吗？</summary>

不会。错误路径调用 `msg_zerocopy_put_abort()`：清 refcount 触发 `__msg_zerocopy_callback`，但此时 `uarg->len == 0`（没有任何 send 序号成功挂上），函数开头 `if (!uarg->len ...) goto release` 直接释放资源、跳过排队。这个 len==0 分支就是防假通知的闸门——否则应用会收到一个失败 send 的"完成"，错误释放 buffer。
</details>

<details>
<summary>Q4：为什么 sendfile/splice 没有完成通知机制？它们不需要吗？</summary>

不需要。页缓存的 page 由 page cache 持有稳定引用，物理地址天然稳定，socket 侧只是"借用页引用"——skb 释放后引用归还页缓存，数据源永远不会消失。MSG_ZEROCOPY 的数据源是**用户匿名内存**，没有 page cache 那样的持久宿主，才需要 pin + 通知来定义"何时用户可安全复用"。问题根源不同，机制重量级自然不同。
</details>

<details>
<summary>Q5：容器里跑 ZC 大流量 sendmsg 报 -ENOBUFS，最可能撞了什么？</summary>

RLIMIT_MEMLOCK（或 sysctl_optmem_max）。`mm_account_pinned_pages` 把 pin 的页计入进程 memlock 配额，容器默认常只有 64KB——几十个并发 ZC send 就会超。处理：调大 ulimit -l / cgroup 的 memlock 限额，或改小单次 send 聚合（byte_limit 本来就限制 512KB/8 页级别 × 并发数）。
</details>
