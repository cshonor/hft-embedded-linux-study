# Chapter 09: Memory Cgroup 与监控

> 来源：Bootlin（memory cgroup + 监控工具）
> 对标：Mel Gorman（无 memcg 现代实现）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [memory-cgroup](notes/01-memory-cgroup.md) | Bootlin：memcg v1/v2、memory.high/low/max、oom 控制 |
| 2 | [monitoring-tools](notes/02-monitoring-tools.md) | Bootlin：/proc/meminfo、vmstat、sar、drsm、numastat |

## HFT 关联

- **cgroup 隔离**：HFT 进程放入独立 cgroup，设 `memory.low` 保证最低内存供应，`memory.max` 限制非关键进程
- **NUMA 统计**：`numastat -p <pid>` 查看 HFT 进程的 NUMA 内存分布，确保本地节点
- **vmstat 监控**：`vmstat 1` 观察 si/so（swap in/out）、pgscand（direct reclaim），任何非零值都是 HFT 的红灯
- **memcg v2**：v2 统一层级，`memory.current` / `memory.peak` 精确控制

## 交叉引用

- `06.5-modern-mm/chapter-08-oom-psi-zswap/`：memcg 触发 OOM 的流程
- `06.6-systems-performance/`：性能监控工具体系
