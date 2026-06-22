## HFT 版「Unexplained Win」演练模板

**场景：** 部署后 tick P99 从 8 µs → 5 µs，团队不确定该不该庆祝。

| 步骤 | 动作 |
|------|------|
| 1 陈述 | 同一合约集、同一时段、同一机器角色？ |
| 2 统计 | `mpstat`、`pidstat`、`ss`、**邻居是否变化** |
| 3 配置 diff | 内核、绑核、THP、DPDK 参数、策略 binary |
| 4 PMC | `perf stat` IPC、cache-miss、context-switches |
| 5 Profile | `perf record` 热点是否迁移 |
| 6 BPF | `runqlat`、`offcputime`、行情路径 syscall |
| 7 结论 | 写入 **release note** — 可复现的 win 还是环境 noise |

→ [11-HFT ch10 延迟测量](../../../11-HFT-Low-Latency-Practice/chapter-10-延迟测量与基准压测/)

---


---

← [本章导读](../README.md)
