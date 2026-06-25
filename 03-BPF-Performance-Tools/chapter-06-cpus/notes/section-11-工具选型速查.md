# 11. 工具选型速查

| 症状 | 优先工具 |
|------|----------|
| 整体 CPU 高 | `mpstat` → `profile` / 火焰图 |
| 延迟高但 CPU 不高 | `offcputime` |
| 怀疑调度/抢核 | `runqlat`、`runqslower` |
| 短命进程 | `execsnoop` |
| 单核打满 | `mpstat -P ALL` + `profile -C` |
| 缓存/IPC 差 | `perf stat`、`llcstat` |
| 中断风暴 | `hardirqs`、`softirqs` |
| syscall 过多 | `syscount` |

---
