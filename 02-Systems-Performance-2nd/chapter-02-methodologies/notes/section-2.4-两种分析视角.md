## 2.4 两种分析视角

自上而下（业务→系统）或自下而上（资源→业务）均可；**互补**，不必二选一。

### 资源分析（Resource Analysis）

- **谁用：** 系统管理员、SRE、平台工程。
- **看什么：** CPU、内存、磁盘、网络等资源的 **使用率、饱和度、错误**。
- **典型问题：** 「哪块 CPU 100%？」「网卡有没有 drop/error？」

### 工作负载分析（Workload Analysis）

- **谁用：** 应用开发者、策略/交易工程。
- **看什么：** 请求率、各操作延迟分布、成功/错误比。
- **典型问题：** 「每秒多少 tick？」「发单 P99 多少 μs？」「错误是 timeout 还是 reject？」

### HFT 合并用法

```
1. Workload：tick → parse → signal → send 各段 latency（追踪 / 时间戳）
2. Resource：对应阶段绑定的 CPU、cache、NIC、队列（USE + perf/BPF）
3. 对齐：慢的阶段 ↔ 饱和/错误的资源
```

→ 工具选型 [Ch 4](../../chapter-04-observability-tools/)；追踪 [Ch 13](../../chapter-13-perf/) / [Ch 15](../../chapter-15-bpf/)

---


---

← [本章导读](../README.md)
