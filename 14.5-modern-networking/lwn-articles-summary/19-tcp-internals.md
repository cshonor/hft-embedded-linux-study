# 19 — TCP 内部优化（TSO / pacing / RACK）

> **对应 Rosen:** Ch11（TCP 基础实现）
> **内核版本:** TSO 很早；pacing 3.12+；RACK 4.9+；TCP internal offload 持续演进

## TSO（TCP Segmentation Offload）

内核构造一个大的 TCP 段（最多 64KB），由硬件负责分段：
- 减少协议栈处理次数
- 减少驱动 DMA 描述符数量
- 现代网卡普遍支持

HFT 注意：TSO 会影响发送时机——内核等待凑够大段才发送，增加延迟。
```bash
# 关闭 TSO（降低发送延迟）
ethtool -K eth0 tso off
```

## Pacing（发送节奏控制）

传统 TCP 依赖拥塞窗口（cwnd）控制发送量，但不控制发送节奏：
- cwnd 允许发送 N 个包，内核可能瞬间全部发出（burst）
- burst 导致交换机队列拥塞 → 尾延迟增加

Pacing 将 cwnd 均匀分布在 RTT 内发送：
- `sk_pacing_rate`：每个 socket 的发送速率
- 内核用 fq qdisc 或 sch_fq 实现 pacing
- 3.12+ 默认为每个 TCP 流设置 pacing rate

HFT 影响：交易报文通常很小，pacing 影响不大。但行情转发流受 pacing 影响。

## RACK（Recent ACKnowledgement，4.9+）

RACK 改进 TCP 丢包检测：
- 传统：3 个重复 ACK 或超时 → 判定丢包
- RACK：根据最近 ACK 的时间戳推断哪些包可能丢失
- 更快检测丢包，减少等待时间
- 已成为默认丢包检测算法（替代 SACK + FACK）

## 其他现代 TCP 优化

| 特性 | 内核版本 | 作用 |
|------|---------|------|
| TCP Fast Open | 3.6+ | 首包携带数据，省一个 RTT |
| TCP repair | 3.5+ | 迁移 TCP 连接（容器/进程迁移） |
| TLS offload | 4.13+ | 网卡硬件 TLS 加解密 |
| TCP_AUTHOPT | 5.15+ | TCP MD5 替代（AO 选项） |

## HFT 关联

| 特性 | HFT 建议 |
|------|---------|
| TSO | 交易报文关闭（降低延迟） |
| Pacing | 交易流影响小；行情转发流注意 |
| RACK | 保持默认（更快丢包检测） |
| TCP Fast Open | 交易连接建立时可启用 |
| TLS offload | 加密交易流时可用 |
