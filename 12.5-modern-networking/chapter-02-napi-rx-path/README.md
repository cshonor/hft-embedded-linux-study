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

## HFT 关联

- **NAPI polling**：HFT 网卡应启用 `SO_BUSY_POLL` 或 threaded NAPI，减少 IRQ 延迟
- **GRO 聚合**：GRO 将多个小包合并为一个大 skb，减少协议栈处理次数；但增加延迟（需等待聚合窗口）
- **busy polling**：`SO_BUSY_POLL=50` 让用户态线程主动 poll NAPI，避免 IRQ 唤醒延迟
- **budget 调优**：`netdev_budget` 控制单次 NAPI poll 处理的包数，HFT 应适当增大

## 交叉引用

- `12.5-modern-networking/chapter-04-page-pool/`：page pool 为 NAPI 提供缓冲区
- `12.5-modern-networking/chapter-05-xdp-architecture/`：XDP 在 NAPI poll 之前拦截
