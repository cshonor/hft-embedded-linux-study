# 03 — 发包路径

> **Bootlin 课程模块：** TX Path
> **对应 Rosen:** Ch11

## 现代 TX 路径（5.x/6.x）

```
1. sendmsg() → 协议栈构造 sk_buff
   ├─ MSG_ZEROCOPY → 不拷贝数据，映射用户 page
   └─ 普通模式 → copy_from_user() 拷贝数据
2. TCP/UDP 处理 → 设置序列号/校验和
3. IP 层 → 路由查找 → 设置 IP 头
4. tc egress → qdisc 排队
   ├─ tc-BPF → 分类/标记
   └─ fq/codel → pacing/调度
5. 驱动 dequeue → DMA 映射 → 写入 Tx ring
6. NIC DMA 发送 → 线缆
7. NIC 完成中断 → 驱动清理 Tx ring → 释放 sk_buff
```

## 发包延迟优化

| 优化 | 效果 | 代价 |
|------|------|------|
| 关闭 TSO | 每包独立发送，降低延迟 | 吞吐量降低 |
| 关闭 qdisc | 减少 qdisc 排队延迟 | 无 QoS |
| MSG_ZEROCOPY | 大包减少拷贝 | 小包通知开销大 |
| io_uring SEND_ZC | 异步零拷贝 | 需要 6.0+ |
| BQL（Byte Queue Limits） | 限制驱动队列长度 | 默认开启 |

## HFT 发包延迟分解

| 阶段 | 延迟（ns） |
|------|-----------|
| sendmsg → sk_buff | 500-2000 |
| 协议栈处理 | 500-1000 |
| qdisc 排队 | 100-10000（取决于队列长度） |
| 驱动 dequeue → DMA | 100-500 |
| NIC 发送 | 100-500 |
| **总计** | ~1-5 μs |
