# Chapter 03: 发包路径与 sk_buff

> 来源：Bootlin（发包路径）+ kernel-docs（txrx + sock/sk_buff）+ LWN（sk_buff/xdp_buff）
> 对标：Rosen Ch3/5
> 内核版本：以 v6.6 为准，本篇的函数名与行号均已核对内核源码

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [tx-path-bootlin](notes/01-tx-path-bootlin.md) | 发包主线：`sendmsg` → tc egress → qdisc → `ndo_start_xmit` → 完成中断 |
| 2 | [txrx](notes/02-txrx.md) | 驱动与内核的**契约**：`ndo_start_xmit` 返回值、DMA 时点、ring 清理、释放函数 |
| 3 | [sock-sk-buff](notes/03-sock-sk-buff.md) | `sk_buff` 生命周期：三个 slab cache、fclone、clone/copy、释放语义 |
| 4 | [sk-buff-xdp-buff](notes/04-sk-buff-xdp-buff.md) | `xdp_buff` ↔ `sk_buff` 的**三种转换**路径与 headroom 的真相 |

## 本篇的四个核心结论

1. **qdisc 是发包延迟的主战场**：默认 `fq_codel` 的 CoDel target 是 **5 ms**，
   它会主动把队列攒到 5 ms 才丢包。换 `pfifo_fast` / `noqueue` 是收益最大的一步。
2. **`NETDEV_TX_BUSY` 不是正常的背压机制**：正规流控是
   `netif_tx_stop_queue()` / `netif_tx_wake_queue()`（零成本），
   BUSY + requeue 是"走到门口又退回来"的纯浪费。看 `tc -s qdisc` 的 `requeues`。
3. **释放函数选错会让丢包监控说谎**：完成路径该用 `napi_consume_skb(skb, budget)`
   （`SKB_CONSUMED`，不计丢包，走 per-CPU 缓存），误用 `kfree_skb()` 会把
   **每个成功的包都记成一次丢包**。
4. **`xdp_frame` 存在包自己的 headroom 里**（`xdp_convert_buff_to_frame()` 里
   `xdp_frame = xdp->data_hard_start;`）——这才是 XDP 强制要求 headroom 的硬性理由，
   不只是"为了加封装头"。

## HFT 关联

- **qdisc 延迟**：默认 `fq_codel` 引入毫秒级尾延迟；HFT 发包应用 `pfifo_fast` 或直接 bypass
- **发包延迟是双峰的**：qdisc 空时 ~2 μs，一开始排队就跳到"队列深度 × 单包时间"。
  **只测 mean 会完全看不到第二个峰**
- **`MSG_ZEROCOPY` 对小包是负优化**：真实代价不是系统调用，而是发送 buffer 在完成通知
  到达前**不能复用**（通知还被 `tx-usecs` 合并延迟）。约 10 KB 以下用普通 send
- **`xmit_more` 会攒住前几个包**：单包延迟测量必须一次只发一个，否则测到的是被攒住的部分
- **下单链路先查 `TCP_NODELAY`**：Nagle + Delayed ACK 能凭空造出 200 ms
- **TCP 发包没有 busy poll**：`SO_BUSY_POLL` 只管收包；发包的完成清理依赖中断，
  所以 `ethtool -C` 的 **TX** 合并同样重要
- **`skbuff_small_head`**（6.x 新增）：下单报文几十~几百字节正好命中这个 cache，省一次页面分配
- **fclone 省在分配、亏在释放**：`skb_clone()` 几乎零成本，但 clone 出的 skb 释放时
  **走不了 per-CPU 缓存**（`napi_consume_skb` 里 `fclone != SKB_FCLONE_UNAVAILABLE` 就退化）
- **`users` 和 `dataref` 是两套计数**：`skb_clone()` 后 `users == 1` 但 `cloned == 1`
- **别往 `cb[48]` 塞跨层的东西**：48 字节被每层复用，下一层会覆盖

## 交叉引用

- `12.5-modern-networking/chapter-01-net-stack-architecture/`：12 个 hook 点的全局顺序
- `12.5-modern-networking/chapter-02-napi-rx-path/`：收包侧 NAPI 与 GRO
- `12.5-modern-networking/chapter-04-page-pool/`：page_pool 与 DMA 保持映射
- `12.5-modern-networking/chapter-05-xdp-architecture/`：XDP 程序本身
- `12.5-modern-networking/chapter-06-af-xdp/`：AF_XDP 零拷贝（`MEM_TYPE_XSK_BUFF_POOL`）
- `12.5-modern-networking/chapter-09-tc-bpf/`：qdisc 与 tc-BPF 详细机制
- `12.5-modern-networking/chapter-13-zerocopy-highperf/`：MSG_ZEROCOPY 零拷贝发送
