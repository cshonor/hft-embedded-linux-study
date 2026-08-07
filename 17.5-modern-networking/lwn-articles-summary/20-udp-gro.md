# 20 — UDP GRO：批量接收

> **对应 Rosen:** Ch11（UDP 无 GRO）
> **内核版本:** UDP GRO 5.0+

## 背景

组播行情通常使用 UDP：
- 每个行情更新一个 UDP 包
- 高频行情流每秒数万包
- 每个包小（几百字节到 1KB）
- 高 PPS 导致每包处理开销大

## UDP GRO 机制

GRO（Generic Receive Offload）原只支持 TCP，5.0+ 扩展到 UDP：
- 收包路径将同 flow 的多个 UDP 包合并成一个大包
- 协议栈只处理一次（而非 N 次）
- `recvmsg()` 一次读取合并后的大包

```c
// 接收 UDP GRO 合并包
struct msghdr msg = {};
char buf[65535];
struct iovec iov = { .iov_base = buf, .iov_len = sizeof(buf) };
msg.msg_iov = &iov;
msg.msg_iovlen = 1;

int n = recvmsg(sockfd, &msg, 0);
// n 可能是多个原始包合并后的大小
// 需要解析 GSO 段（msg.msg_flags & MSG_EOR 标记分段）
```

## UDP GRO forwarding

合并后的 UDP 包可以转发到其他 socket 或网卡：
- `UDP_GRO` → `sendmsg(MSG_ZEROCOPY)` → GSO 发送
- 减少转发路径处理开销

## 性能影响

| 指标 | 无 UDP GRO | UDP GRO |
|------|-----------|---------|
| PPS 处理能力 | ~3 Mpps/core | ~8 Mpps/core |
| 延迟 | 基准 | 增加合并窗口延迟 |
| CPU 占用 | 高 | 低 |

## HFT 关联

| 场景 | UDP GRO 适用性 |
|------|---------------|
| 行情接收（需最低延迟） | 不推荐（合并窗口增加延迟） |
| 行情转发/录制（吞吐优先） | 推荐（减少 CPU 开销） |
| 组播行情中继 | 推荐 |

> HFT 交易路径不建议开 UDP GRO（延迟优先）。行情录制/转发等非关键路径可开（吞吐优先）。
