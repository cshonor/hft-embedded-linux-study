# Cross-ref · Timing / pipeline ↔ HFT x86

> **Core Concept:** Propagation delay, hazards, cache → software latency
> **Link Target:** CSAPP §4.5 · `-O3` ceiling · branchless / inline / dependency chains

| Hardware fact | Software response |
|---------------|-------------------|
| Long combo path | Keep hot work short / predictable |
| Data hazard / load-use | Break dependent chains; avoid load-then-use |
| Control hazard / mispredict | Branchless, tables, `cmov`; `inline` cuts `call`/`ret` |
| Structural / memory port | Locality, cache-line-aware layout |
| Cache miss ≫ gate delay | Data layout > micro-mux tweaks |

## Notes

（Ch6 读完后扩写；对接 `perf` branch-misses / IPC。）
