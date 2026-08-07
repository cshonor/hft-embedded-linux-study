# 04 — sk_buff → xdp_buff：收包路径分流

> **对应 Rosen:** Ch1/Ch11（sk_buff 是唯一数据结构）
> **内核版本:** xdp_buff 4.8+；xdp_frame 4.18+

## 传统收包路径（Rosen 3.x）

```
NIC DMA → alloc_page → NAPI poll → sk_buff alloc → 协议栈 → socket
```
- 每个收到的包都分配一个 `sk_buff`（约 256 字节）
- sk_buff 包含大量元数据（指针、协议头偏移、队列映射等）
- 分配 sk_buff 后才能传递给协议栈处理

## XDP 引入的数据路径分流

```
NIC DMA → page_pool alloc → XDP hook (xdp_buff) → 决策：
  ├─ XDP_PASS  → 分配 sk_buff → 协议栈 → socket（传统路径）
  ├─ XDP_DROP  → 直接丢弃，不分配 sk_buff
  ├─ XDP_TX    → 原路反弹发送
  ├─ XDP_REDIRECT → 转发到其他设备/CPUMAP/AF_XDP socket
  └─ XDP_ABORTED → 错误，丢弃
```

## xdp_buff vs sk_buff

| 维度 | sk_buff | xdp_buff |
|------|---------|---------|
| 分配时机 | 收到包后立即分配 | XDP 处理完才分配（PASS 时） |
| 大小 | ~256 字节元数据 | ~64 字节，轻量 |
| 数据访问 | 指针跳转多层 | 线性 data/data_end 指针 |
| 协议栈 | 必需 | 不经过协议栈 |
| 可修改性 | 可改头部但开销大 | 可原地改包内容 |

## 性能影响

XDP DROP 路径完全不分配 sk_buff：
- 传统路径：alloc_page + alloc sk_buff + 协议栈处理 → ~300 cycles
- XDP DROP：检查包头 → ~10 cycles
- 对 HFT 行情流中的垃圾包过滤非常有效

## HFT 关联

| 场景 | xdp_buff 优势 |
|------|-------------|
| 行情早过滤 | XDP 检查组播地址/端口，丢弃无关包，不分配 sk_buff |
| 行情早分类 | XDP 修改包的队列映射，引导到特定 CPU |
| AF_XDP 零拷贝 | xdp_buff 直接 redirect 到用户态，不经过协议栈 |
