# Chapter 13: 零拷贝与高性能网络

> 来源：kernel-docs（scaling + msg_zerocopy）+ LWN（MSG_ZEROCOPY + TCP zero-copy recv + SO_REUSEPORT）
> 对标：Rosen（无零拷贝，3.x 无 MSG_ZEROCOPY）
> 本版基于 **v6.6 源码核验**重写（dev.c / skbuff.c / tcp.c / sock.c / sock_reuseport.c 全部锚点到行号）

## 核心结论（全章浓缩）

1. **多核扩展是一条五层链**：RSS（硬件 hash）→ RPS（软件 hash）→ RFS（包追消费者）→ `SO_INCOMING_CPU`+reuseport（socket 归核）→ XPS（发送侧镜像）——任何一环断裂就是一次跨核 cache miss 或 IPI；`get_rps_cpu()`（dev.c:4557）两表皆空时零开销通过。
2. **MSG_ZEROCOPY 的本质是"逐字节拷贝 → 页 pin + 完成通知"**：buffer 释放边界从 sendmsg 返回推迟到 errqueue 通知（`[ee_info, ee_data]` 序号区间，相邻区间在内核侧合并）；TCP 要求 `sk_route_caps & NETIF_F_SG`，不满足则带 `SO_EE_CODE_ZEROCOPY_COPIED` 降级。
3. **uarg 藏在零长 skb 的 cb 里**（skbuff.c:1540）：一次 `sock_omalloc(sk, 0)` 同时解决容器生命周期、refcount、通知载体三件事；512KB/USHRT_MAX 聚合上限控制通知粒度与 pin 集合。
4. **接收侧 `TCP_ZEROCOPY_RECEIVE` 是 mmap 语义**：`mmap(socket_fd)` 建 `tcp_vm_ops` VMA，setsockopt 换页（32 页一批 `vm_insert_pages`），只映射整页、`inq <= copybuf_len` 退化为拷贝；映射即 ACK——可靠性契约比 recvmsg 弱。
5. **零拷贝收益全是量级问题**：发送拐点 ~10KB（固定开销 2-3μs vs 线性 memcpy），接收拐点 ~32KB（TLB flush 成本）；**io_uring registered buffers 不是接收零拷贝**（省 pin 不省 memcpy），DMA 直达只有 AF_XDP/DPDK。
6. **SO_REUSEPORT = 内核 lookup 路径里的分发器**：BPF 优先（`sk_reuseport_md` 从传输层头开始可见）、hash 兜底、5.14+ 支持 listener 迁移（滚动重启不丢连接）；UDP 组播是每 socket 一份的天然 fan-out。

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [scaling](notes/01-scaling.md) | RSS/RPS/RFS/aRFS/XPS/SO_INCOMING_CPU 五层链，dev.c 源码级 |
| 2 | [msg-zerocopy](notes/02-msg-zerocopy.md) | MSG_ZEROCOPY 用户态协议：errqueue、区间合并、copied 降级 |
| 3 | [msg-zerocopy-lwn](notes/03-msg-zerocopy-lwn.md) | 实现内幕：ubuf_info_msgzc、refcount 生命周期、四条 ZC 路线对比 |
| 4 | [tcp-zero-copy-recv](notes/04-tcp-zero-copy-recv.md) | TCP_ZEROCOPY_RECEIVE：mmap 换页协议、TLB 成本、语义陷阱 |
| 5 | [so-reuseport](notes/05-so-reuseport.md) | reuseport 组管理、BPF 选择器、listener 迁移、组播 fan-out |

## HFT 关联

- **交易报文（< 1KB）**：不用任何零拷贝——拷贝几十 ns，pin+通知的固定开销是它的 10 倍
- **行情转发/回放（大 payload）**：MSG_ZEROCOPY（发送）+ TCP_ZEROCOPY_RECEIVE（接收）拐点之上收益 3-25x
- **全链路同核范式**：ntuple/RSS 分流 + SO_INCOMING_CPU accept-retry + sched_setaffinity + XPS——终极形态是收发包和处理在同核零跨
- **HFT 完整方案**：AF_XDP 收 + MSG_ZEROCOPY/SEND_ZC 发 + SO_REUSEPORT+BPF 按 symbol 分发

## 交叉引用

- `12.5-modern-networking/chapter-03-tx-path-skbbuff/`：发包路径与零拷贝
- `12.5-modern-networking/chapter-06-af-xdp/`：AF_XDP 零拷贝收包
- `12.5-modern-networking/chapter-12-io-uring-net/`：SEND_ZC（同一 uarg 机制的异步化）
- `04-cpp/M2-cpp-network-programming/`：SO_REUSEPORT 基础
