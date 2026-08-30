# XDP / tc-BPF 与 DPDK 对照

> **02-Advanced-Book** · 《Linux 高性能网络详解》配套 · **选读**

## 与 DPDK 的分工

| | DPDK（01-Intro） | XDP / tc-BPF | **AF_XDP** |
|---|------------------|--------------|-----------|
| 位置 | 用户态完全旁路 | 内核最早 hook 点 | 内核 hook + 用户态收包 |
| 网卡归属 | **归 DPDK**，内核看不到 | 归内核 | **归内核**，只把指定流重定向出来 |
| 生效粒度 | 整张网卡 | 每张网卡一个程序 | **可只旁路一条流**，其余照常 |
| 典型用途 | UDP 组播行情、极致 poll | 早期过滤、统计、DDoS 防护 | 渐进式旁路，可回退 |
| 延迟量级 | ~0.3–1μs | ~1–5μs（内核内处理） | zc ~0.5–2μs；**copy 模式无意义** |
| 开发成本 | 高（无 socket 语义） | 中（仍在 eBPF 生态） | 中（ring + UMEM 要自己管） |

**AF_XDP 是两者之间的桥**——这也是它最实用的地方：

- 想提速但**不能动基础设施**（管理流量、SSH、监控都还在）→ AF_XDP
- 已有系统先旁路一条行情流试水，效果满意再考虑整卡切 DPDK → AF_XDP 是低风险第一步
- 全新专用接入卡、要最后那点确定性 → DPDK

⚠ **AF_XDP 的 copy 模式是个陷阱**：它仍然会分配 `sk_buff` 并做一次 memcpy，
只是省了协议栈。只用 **zero-copy**（驱动支持 + 绑定正确队列）才有价值。
→ [12.5/chapter-06/notes/03-af-xdp-umem-layout](../../../12.5-modern-networking/chapter-06-af-xdp/notes/03-af-xdp-umem-layout.md)

### 组合用法（生产环境常见）

```
行情口（独占网卡）  → DPDK 收包，解析，喂策略
管理口（另一张卡）  → 内核栈 + XDP 做过滤/DDoS 防护/统计
```

**这两者不冲突，是分工。** XDP 在管理面继续做它擅长的事（内核内早期处置），
DPDK 在数据面拿确定性。别把 XDP 装到 DPDK 接管的网卡上——那张卡内核已经看不到了。

→ 深入 XDP 实现与工具：[06.7-BPF note-XDP与tc-BPF](../../../06.7-bpf-observability/02-bpf-performance-tools/note-XDP与tc-BPF.md)  
→ 旁路后的完整链路：[01-Intro chapter-04 零拷贝与用户态旁路](../../01-Intro-Book/notes/chapter-04-零拷贝与用户态旁路.md)  
→ 方案总表：[note-openonload-rdma对比](./note-openonload-rdma对比.md)

## 相关章节

- 上一梯度：[01-Intro-Book](../../01-Intro-Book/)
- [10 总目录](../../README.md)
