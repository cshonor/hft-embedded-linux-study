# Cross-ref · Timing / pipeline ↔ HFT x86

> **Core Concept:** Propagation delay, hazards, cache → software latency on the trade path
> **Link Target:** CSAPP §4.5 · `-O3` ceiling · branchless / inline / dependency chains

| Hardware fact | Software response |
|---------------|-------------------|
| Long combo path | Keep hot work short / predictable |
| Data hazard / load-use | Break dependent chains; avoid load-then-use |
| Control hazard / mispredict | Branchless, tables, `cmov`; `inline` cuts `call`/`ret` |
| Structural / memory port | Locality, cache-line-aware layout |
| Cache miss ≫ gate delay | Data layout > micro-mux tweaks · 见 [8.3](../ch08_memory/8.3_高速缓存.md) |
| TLB / page fault | Huge pages、锁页；见 [8.4](../ch08_memory/8.4_虚拟存储器.md) |
| AMAT 被 MissPenalty 主导 | 热路径避冷 miss；量化用 Hit+MR×Penalty |

## Why this book helps HFT

书里的 **通路延迟、流水线冒险、存储层级** = 交易链路里「多出来的纳秒」的硬件来源之一。目标不是在 ARM 上跑撮合，而是：**会从底层解释延迟**，再在 x86 热路径上用软件手段少 stall / 少 miss / 少冲刷。

## Notes

Ch7/Ch8 口述已落笔记；对接 `perf`：`cache-misses` / `dTLB-load-misses` / branch-misses / IPC。
