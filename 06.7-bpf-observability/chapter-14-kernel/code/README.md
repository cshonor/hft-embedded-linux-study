# 02-ch14 code · 内核域 bpftrace 程序

> 《BPF Performance Tools》第 14 章示例落盘。
> 笔记中 tasklet_init 内核源码（§7 块8）为引用，不落盘。

## 程序

| 文件 | 出处 | 内容 | 验证（bpftrace 0.20.2） |
|------|------|------|------|
| [loadavg.bt](./loadavg.bt) | §4 块1 | kaddr("avenrun") 读内核 loadavg（定点数 11 位小数移位换算） | 语义解析 ✓（实跑需 root） |
| [mlock-latency.bt](./mlock-latency.bt) | §5 块2 | 内核 mutex 等待直方图，按 [锁符号, 栈, comm] 聚合 | 语义解析 ✓ |
| [kmalloc-stats.bt](./kmalloc-stats.bt) | §6 块3/4 | 内核分配 stats()（count+avg+total）+ 页分配计数 | 语义解析 ✓ |
| [slab-name.bt](./slab-name.bt) | §6 块5 | 按 slab cache 名统计（比 slabtop 多调用方上下文） | 语义解析 ✓ |
| [numa-migrate.bt](./numa-migrate.bt) | §6 块6 | NUMA balancing 页迁移耗时（搬页=绑定没做好的证据 → 06.6/ch07） | 语义解析 ✓ |
| [workq-latency.bt](./workq-latency.bt) | §7 块7 | workqueue 工作函数耗时（args->function 指针 + ksym 译名） | 语义解析 ✓ |

## 要点

- `kaddr("符号")`：从 BPF 程序读内核全局变量（avenrun/sys_call_table…）
- 聚合键里的地址都要 `ksym()` 译名，否则输出全是裸地址
- 这六个程序覆盖了内核四大件：调度（loadavg/mlock）、内存（kmalloc/slab/numa）、延后执行（workq）
