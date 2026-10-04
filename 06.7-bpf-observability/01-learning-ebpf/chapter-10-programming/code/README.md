# ch10 code · bpftrace 编程与统计

> 《Learning eBPF》第 10 章：bpftrace 编程三招 + BPF 程序自身性能统计。
> 笔记中 `bpftrace -l` 探针勘探（10.1 块1）已并入 README 与 06.6/ch15 脚本。

## 程序

| 文件 | 出处 | 内容 | 验证（bpftrace 0.20.2） |
|------|------|------|------|
| [open-trace.bt](./open-trace.bt) | 10.1 块2 | open/openat 入口出口配对追踪（逗号多挂点 / @[tid] 配对 / delete 防泄漏 / END 清理） | 语义解析 ✓（报错为权限而非语法；实跑需 root） |

## scripts/

| 脚本 | 出处 | 内容 | 验证 |
|------|------|------|------|
| [bpf-stats.sh](./scripts/bpf-stats.sh) | 10.2 块3 | `kernel.bpf_stats_enabled=1` → bpftool prog list 读 run_time_ns/run_cnt → 用完关 | `bash -n` ✓（sysctl 需 root） |

## bpftrace 编程三招（本章核心）

1. **逗号多挂点**：`tracepoint:a, tracepoint:b { }` —— open/openat 逻辑复用
2. **entry/exit 配对**：入口 `@x[tid] = args->p`，出口 `/@x[tid]/` 过滤后取用
3. **delete + END clear**：用完 `delete(@x[tid])` 防泄漏，`END { clear(@x); }` 兜底
