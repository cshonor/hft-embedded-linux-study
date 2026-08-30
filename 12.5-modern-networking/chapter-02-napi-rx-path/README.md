# Chapter 02: NAPI 与收包路径

> 来源：Bootlin（收包路径）+ kernel-docs（NAPI）+ LWN（NAPI modern + GRO/GSO）
> 对标：Rosen Ch2（3.x NAPI → 6.x NAPI + page_pool + XDP hook）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [rx-path-bootlin](notes/01-rx-path-bootlin.md) | Bootlin：收包全路径、IRQ → NAPI poll → netif_receive_skb |
| 2 | [napi](notes/02-napi.md) | kernel-docs：NAPI poll 机制、budget、busy polling |
| 3 | [napi-modern](notes/03-napi-modern.md) | LWN：6.x NAPI 改进、threaded NAPI、GRO 统一 |
| 4 | [gro-gso](notes/04-gro-gso.md) | LWN：GRO 收包聚合、GSO 发包分段、性能影响 |
| 5 | [multicast-rx-path](notes/05-multicast-rx-path.md) | UDP 组播收包：L2 有损映射、组播复制开销、**旁路后 IGMP 失效**、gap 检测 |
| 6 | [busy-poll-mechanism](notes/06-busy-poll-mechanism.md) | 三代 busy polling：socket → SO_PREFER_BUSY_POLL → NAPI defer（内核栈延迟天花板） |
| 7 | [queue-steering-rss](notes/07-queue-steering-rss.md) | 多队列 / RSS / ntuple 流定向、中断绑核、irqbalance 与 managed_irq |

## HFT 关联

- **NAPI polling**：HFT 网卡应启用 `SO_BUSY_POLL` 或 threaded NAPI，减少 IRQ 延迟
- **GRO 聚合**：GRO 将多个小包合并为一个大 skb，减少协议栈处理次数；但增加延迟（需等待聚合窗口）
- **busy polling**：`SO_BUSY_POLL=50` 让用户态线程主动 poll NAPI，避免 IRQ 唤醒延迟
- **budget 调优**：`netdev_budget` 控制单次 NAPI poll 处理的包数，HFT 应适当增大
- **行情走 UDP 组播不是 TCP**：组播路径多出 `ip_route_input_mc()` 与 per-socket skb 复制；
  旁路后 IGMP 不再自动发出，必须先与交换机确认静态组播 → [05](notes/05-multicast-rx-path.md)
- **NAPI defer 是内核栈天花板**：`napi_defer_hard_irqs` + `gro_flush_timeout`（单位**纳秒**）
  零代码成本压到 2-5μs；p99 仍不达标才值得付出旁路复杂度 → [06](notes/06-busy-poll-mechanism.md)
- **行情流必须用 ntuple 钉队列**：组播四元组固定 → RSS 恒定落同一队列，不主动规划就队头阻塞；
  关 irqbalance、关 RPS/RFS、`isolcpus` 带 `managed_irq` → [07](notes/07-queue-steering-rss.md)

## 交叉引用

- `12.5-modern-networking/chapter-04-page-pool/`：page pool 为 NAPI 提供缓冲区
- `12.5-modern-networking/chapter-05-xdp-architecture/`：XDP 在 NAPI poll 之前拦截
- `12.5-modern-networking/chapter-06-af-xdp/notes/03-af-xdp-umem-layout.md`：AF_XDP 按队列旁路，配合 07 的流定向
- `13-dpdk/01-Intro-Book/notes/chapter-05-组播行情接入.md`：05 的内核路径 → DPDK 旁路对照
- `12-kernel-networking/note-组播IGMP.md`：组播协议基础（IGMP snooping / 组播路由）
