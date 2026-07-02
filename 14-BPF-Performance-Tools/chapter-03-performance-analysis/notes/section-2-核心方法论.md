# 2. 核心方法论

### 工作负载表征 (Workload Characterization)

回答 **系统在被什么驱动**：

| 问题 | 例子 |
|------|------|
| 谁引起负载？ | 哪个进程、哪条策略、哪类报文 |
| 为什么调用这段代码？ | 配置变更、新合约、retry 风暴 |
| 类型与吞吐？ | QPS、消息率、订单 cancel 比 |
| 随时间如何变？ | 开盘尖峰、roll 窗口 |

**Gregg 观点：** 最大收益常来自 **消除不必要的工作** — 比微优化一条热路径更猛。

→ [SysPerf 2.4 Workload vs Resource](../../../13-Systems-Performance-2nd/chapter-02-methodologies/notes/section-2.4-两种分析视角.md)

### 下钻分析 (Drill-Down Analysis)

从 **宏观指标** 一层层剥：

```
应用阻塞在文件系统
  → 阻塞在 write 路径
    → 阻塞在更新 access timestamp (atime)
      → 挂载选项 noatime 解决
```

**BPF 角色：** 每一层用 **不同工具** 验证假设 — `opensnoop` → `ext4slower` → `biolatency`，而非一次 attach 所有 probe。

### USE 方法 (USE Method)

对 **每个资源** 查三项：

| 字母 | 含义 | 问什么 |
|------|------|--------|
| **U** | Utilization 使用率 | 资源忙的时间比例？ |
| **S** | Saturation 饱和度 | 有排队/等待吗？（超额需求） |
| **E** | Errors 错误 | 硬件/驱动/协议错误？ |

**资源例：** CPU、内存、磁盘 I/O、网络、总线…

→ 本仓库展开：[SysPerf 附录 A USE](../../../13-Systems-Performance-2nd/appendix-A-USE方法Linux.md) · [Ch 2.3.1 走查](../../../13-Systems-Performance-2nd/chapter-02-methodologies/notes/section-2.3.1-时间尺度与排查走查.md)

---
