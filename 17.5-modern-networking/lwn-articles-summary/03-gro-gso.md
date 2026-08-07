# 03 — GRO/GSO 演进与性能

> **对应 Rosen:** Ch11（Layer 4，sk_buff 处理）
> **内核版本:** GRO 2.6.29+；GSO 更早；硬件 offload 持续演进

## GRO（Generic Receive Offload）

GRO 在收包路径将多个小包合并成一个大包：
- 减少协议栈处理次数（每个大包只走一次 IP/TCP 处理）
- 减少 sk_buff 分配数量
- 合并条件：同 flow、同 IP/TCP 头部、连续序列号

现代演进：
- `napi_gro_receive()` → `napi_gro_flush()` 路径优化
- GRO 可按协议禁用：`ethtool -K eth0 gro off`
- XDP 路径不经过 GRO（XDP 在 GRO 之前处理）

## GSO（Generic Segmentation Offload）

GSO 在发包路径将大包延迟分段：
- 协议栈构造一个大的 sk_buff（最多 64KB）
- 驱动或硬件负责实际分段
- 减少协议栈处理开销

现代演进：
- TSO（TCP Segmentation Offload）：硬件分段 TCP 大包
- GSO partial：部分硬件分段 + 部分软件分段
- UFO → USO（UDP Segmentation Offload）：5.x+ 支持 UDP 分段

## 性能权衡（HFT 视角）

| 机制 | 吞吐量 | 延迟 | HFT 建议 |
|------|--------|------|---------|
| GRO on | 高（合并包） | 增加延迟（等待合并窗口） | 行情接收关闭 GRO |
| GRO off | 低 | 最低延迟 | HFT 行情流推荐 |
| GSO/TSO on | 高（大包发送） | 发送节奏不可控 | 交易报文关闭，行情组播关闭 |
| GSO/TSO off | 低 | 每包独立发送 | 小交易报文推荐 |

## HFT 关联

- **行情接收**：GRO 会引入合并等待延迟（微秒级），HFT 应关闭 `ethtool -K eth0 gro off`
- **交易发送**：TSO 会将多个小报文合并发送，影响发送时机，HFT 应关闭 `ethtool -K eth0 tso off`
- **UDP GRO**：5.0+ 引入，组播行情批量接收可提升吞吐，但增加延迟
