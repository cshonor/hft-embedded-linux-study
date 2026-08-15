# Chapter 14: TCP/UDP 内部机制

> 来源：LWN（TCP internals + UDP GRO）
> 对标：Rosen Ch4/6（TCP/UDP 3.x 实现）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [tcp-internals](notes/01-tcp-internals.md) | LWN：TCP 状态机、拥塞控制、发送/接收窗口 |
| 2 | [udp-gro](notes/02-udp-gro.md) | LWN：UDP GRO 批量接收、多包聚合 |

## HFT 关联

- **TCP 拥塞控制**：HFT 交易连接用 `tcp_congestion_control=none`（或 BBR）避免 AIMD 降窗
- **Nagle 禁用**：`TCP_NODELAY=1` 是 HFT 必备，禁用 Nagle 算法避免小包等待
- **UDP GRO**：行情数据用 UDP 多播，GRO 聚合多个 UDP 包减少处理开销
- **TCP buffer 调优**：`tcp_rmem` / `tcp_wmem` 调大窗口，避免窗口缩放不足导致限速

## 交叉引用

- `12-tcpip-protocols/`：TCP/UDP 协议基础
- `13.5-modern-networking/chapter-02-napi-rx-path/`：GRO 在收包路径
