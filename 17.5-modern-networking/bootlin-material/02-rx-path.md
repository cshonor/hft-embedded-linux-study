# 02 — 收包路径

> **Bootlin 课程模块：** RX Path
> **对应 Rosen:** Ch1/Ch11

## 现代 RX 路径（5.x/6.x）

```
1. NIC 收帧 → DMA 写入 Rx ring buffer（page_pool 分配）
2. NIC 更新 Rx 描述符 → 中断或 NAPI 唤醒
3. NAPI poll → 驱动从 Rx ring 取帧
4. XDP hook 执行（如果挂载了 BPF 程序）
   ├─ DROP → page 回收到 page_pool（不分配 sk_buff）
   ├─ PASS → 继续
   ├─ REDIRECT → AF_XDP socket / CPUMAP / DEVMAP
   └─ TX → 修改 MAC 后原路返回
5. 分配 sk_buff（仅 XDP PASS）
6. napi_gro_receive() → GRO 合并
7. netif_receive_skb() → 协议栈
8. IP 层 → 路由查找 → TCP/UDP
9. socket 接收队列 → 唤醒用户进程
```

## 延迟分解（典型）

| 阶段 | 延迟（ns） | 优化手段 |
|------|-----------|---------|
| NIC DMA → 中断 | 100-500 | 中断合并参数 |
| NAPI 调度 → poll | 500-2000 | threaded NAPI |
| XDP 程序 | 10-50 | — |
| sk_buff 分配 | 100-200 | XDP DROP 避免 |
| GRO | 100-500 | 关闭 GRO 减少延迟 |
| 协议栈处理 | 500-2000 | — |
| socket 唤醒 | 1000-5000 | busy polling |
| **总计** | ~2-10 μs | XDP + busy poll 可降到 < 2μs |

## 优化手段

| 手段 | 减少延迟 | 代价 |
|------|---------|------|
| 关闭 GRO | -0.5 μs | 吞吐量降低 |
| SO_BUSY_POLL | -3 μs | CPU 100% |
| XDP 早过滤 | -0.5 μs | BPF 开发 |
| AF_XDP 零拷贝 | -5 μs | 独占 RX 队列 |
| 关闭中断合并 | -0.5 μs | CPU 中断增加 |
