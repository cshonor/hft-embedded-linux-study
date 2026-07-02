## 16.1.3–16.1.4 统计数据与静态配置

### 常规统计（Statistics）— 由浅入深第一步

**目的：** 建立 **baseline 对比** — 「变快」是相对什么？

| 层级 | 工具/数据 | 对应章 |
|------|-----------|--------|
| 全局 | `uptime`、`vmstat`、`mpstat`、`iostat`、`sar` | Ch 6–9 |
| 网络 | `ss`、`ip -s link`、`nstat` | Ch 10 |
| 云/租户 | steal time、cgroup throttle | Ch 11 |
| 历史 | **sar 归档** — 案发前后 | 附录 B |

**要问：**

- 利用率是 **真降** 还是 **测量窗口变**？
- 负载 **workload** 变轻了吗？（Ch 2 工作负载分析）
- 邻居/流量是否 **同时段不可比**？

### 静态配置（Configuration）

**部署/环境 diff：**

| 类别 | 检查项 |
|------|--------|
| 内核 | 版本、cmdline、mitigations |
| sysctl | 网络/VM（Ch 10/7） |
| 实例类型 | AWS 机型、NUMA、CPU credits |
| 应用 | 库版本、GC、线程池、绑核 |
| 挂载/FS | noatime、磁盘类型 |

**HFT checklist：**

```
[ ] 内核/驱动版本 vs baseline
[ ] isolcpus / IRQ affinity / governor
[ ] THP / swappiness / swap
[ ] 同机是否还有 noisy neighbor
[ ] 行情源/合约集合是否相同
```

→ Ch 3 [OS 背景](../../chapter-03-operating-systems/) · Ch 11 [云](../../chapter-11-cloud-computing/)

---


---

← [本章导读](../README.md)
