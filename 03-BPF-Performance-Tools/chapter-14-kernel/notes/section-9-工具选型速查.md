# 9. 工具选型速查

| 症状 | 工具 |
|------|------|
| 阻塞链不清 | **`offwaketime`** |
| 谁唤醒谁 | `wakeuptime` |
| 剔 sleep 噪音的 Off-CPU | `offcputime` + D 状态过滤 |
| 内核 mutex 竞争 | `mlock` / `mheld` |
| 内核 Slab 泄漏/暴涨 | `kmem`、`slabratetop` |
| 大页 alloc 风暴 | `kpages` |
| NUMA 迁移开销 | `numamove` |
| workqueue 慢 | `workq` |
| 读内核代码流 | **Ftrace funcgraph** |

---
