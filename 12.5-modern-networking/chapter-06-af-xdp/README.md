# Chapter 06: AF_XDP

> 来源：kernel-docs（`af_xdp.rst`）+ LWN（AF_XDP 设计）+ 补写（UMEM 布局、工程实践）
> 对标：Rosen（**无 AF_XDP**，3.x 时代该技术不存在，本章是纯增量）
> 内核版本：以 **v6.6** 为准，所有常量、errno、判定条件均已核对源码
> （`include/uapi/linux/if_xdp.h`、`net/xdp/xsk.c`、`net/xdp/xdp_umem.c`、
> `net/xdp/xsk_buff_pool.c`、`include/linux/filter.h`）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [af-xdp](notes/01-af-xdp.md) | **接口精读**：八步生命周期与顺序约束、UMEM 注册四条硬校验、`bind()` 的 zc/copy 判决、`xsks_map` 的 flags 陷阱、`xdp_statistics` 六计数器诊断表 |
| 2 | [af-xdp-lwn](notes/02-af-xdp-lwn.md) | **工程实践**：三条收包路线决策、延迟预算表、`XDP_USE_NEED_WAKEUP` 与 busy poll、批处理与 flush 语义、容量规划、多队列、15 条排障清单 |
| 3 | [af-xdp-umem-layout](notes/03-af-xdp-umem-layout.md) | UMEM frame 布局、`frame_len` 公式、FILL/RX ring 所有权交接、copy vs zero-copy 微观差异与量化对比 |

## 本篇的五个核心结论

1. **不写 `XDP_ZEROCOPY` 就会静默降级。** `xp_assign_dev()`（xsk_buff_pool.c:149）在
   驱动不支持时走 `err_unreg_pool: if (!force_zc) err = 0; /* fallback to copy mode */`——
   bind 返回 0，一切正常，只是慢 2–3 倍。**bind 后必查 `XDP_OPTIONS_ZEROCOPY`。**
2. **`bpf_redirect_map(&xsks_map, idx, 0)` 是自杀式写法。** flags 的低位是
   "lookup 失败时的返回值"（`include/linux/filter.h:1498`），0 = `XDP_ABORTED`
   = **静默丢掉所有非目标流量**（ARP/ICMP/SSH 全断），而 tcpdump 和 ethtool 都显示正常。
   写 `XDP_PASS`。
3. **AF_XDP 零拷贝不用 page_pool。** UMEM 由内核 `xp_dma_map()` 自己映射
   （`xsk_buff_pool` 有独立的 `dma_pages` 数组）。
   "AF_XDP 基于 page_pool"只在 **copy 模式**下成立。
4. **RX ring 的 producer 只在 `xsk_flush()` 时发布，而 flush 在 NAPI poll 结束时发生**
   （`__xsk_map_flush()`，xsk.c:382）。所以用户态**一次看到一批包**，
   批内位置决定单包延迟——这是"零拷贝却仍有几微秒延迟"的主要来源之一。
5. **`chunk_size` 只有 2048 / 4096 可选**（下界 2048、上界 PAGE_SIZE、必须 2 的幂），
   且 `frame_len = chunk_size - headroom - 256`。**jumbo frame 单个 chunk 装不下**，
   只能靠 `XDP_USE_SG`。

## HFT 关联

- **决策树**：驱动不支持 zc → 不用 AF_XDP（回内核栈 + busy poll）；
  要独占整卡 → DPDK；**要跟内核共存 → AF_XDP**
- **只接管指定队列**，其余队列仍走内核栈——SSH/监控/NTP 不受影响，
  这是它相对 DPDK 最大的工程优势
- **UMEM 常驻内存**（`pin_user_pages(FOLL_LONGTERM)`）且吃 `RLIMIT_MEMLOCK`，
  容器部署前先查 `ulimit -l`（失败 errno 是 `-ENOBUFS` 不是 `-ENOMEM`）
- **丢包诊断看计数器比值**：`rx_ring_full` 涨 = 消费慢；
  `rx_fill_ring_empty_descs` 涨 = 归还 frame 慢；
  `rx_dropped` 涨而前两者为 0 = **包比 `frame_len` 大**
- **`XDP_USE_NEED_WAKEUP` + busy poll 是标准配置**：
  前者避免无谓 syscall，后者让 `recvfrom()` 直接在调用上下文驱动 NAPI 并跳过 `ndo_xsk_wakeup`
- **批处理从 32 起测**（内核 `TX_BATCH_SIZE` 也是 32），按 P99 与吞吐两条曲线找拐点
- **一个 (dev, queue_id) 只能有一个 xsk**，`SO_REUSEPORT` 对 AF_XDP 无效；
  多核扩展靠多队列 + `ethtool -N/-X` 流 steering

## 交叉引用

- `12.5-modern-networking/chapter-04-page-pool/`：驱动侧缓冲区池；
  ⚠️ AF_XDP 零拷贝**不用** page_pool，两者是两套体系
- `12.5-modern-networking/chapter-05-xdp-architecture/`：XDP 架构与
  [四个 ring 的设计](../chapter-05-xdp-architecture/notes/02-xdp-rings.md)，
  AF_XDP 是 XDP_REDIRECT 的目的地
- `12.5-modern-networking/chapter-02-napi-rx-path/notes/06-queue-steering-rss.md`：
  按队列旁路前需先用 ntuple / RSS 把行情流钉到独占队列
- `12.5-modern-networking/chapter-07-xdp-redirect-dpdk/`：redirect 机制本体与 DPDK 对比
- `13-dpdk/`：完全 bypass 内核的另一种选择
