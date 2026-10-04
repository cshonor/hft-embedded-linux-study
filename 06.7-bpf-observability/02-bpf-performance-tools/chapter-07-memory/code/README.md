# 02-ch07 code · 内存域 bpftrace 程序

> 《BPF Performance Tools》第 7 章示例落盘。
> 笔记中 perf 传统工具命令（§2 块3/4）已在 06.6/ch13 充分落盘，此处不重复。

## 程序

| 文件 | 出处 | 内容 | 验证（bpftrace 0.20.2） |
|------|------|------|------|
| [oomkill.bt](./oomkill.bt) | §3 块5 | OOM 杀手现场：谁触发/杀了谁/多少页/loadavg | 语义解析 ✓（实跑需 root） |
| [brk-ustack.bt](./brk-ustack.bt) | §5 块7 | brk 扩堆按 [用户栈, comm] 聚合 | 语义解析 ✓ |
| [pagefault-ustack.bt](./pagefault-ustack.bt) | §6 块10 | 缺页错误用户栈聚合（software:page-fault:1） | 语义解析 ✓ |
| [faults-by-file.bt](./faults-by-file.bt) | §6 块11 | 按文件统计缺页（mmap 文件首次触碰热点） | 语义解析 ✓ |
| [mem-expand.bt](./mem-expand.bt) | §10 块1 | 指定 PID 的 brk 增量（exit.ret - entry.brk）+ mmap 追踪（位置参数 $1） | 语义解析 ✓ |

## scripts/

| 脚本 | 出处 | 内容 | 验证 |
|------|------|------|------|
| [mem-oneliners.sh](./scripts/mem-oneliners.sh) | §4-6/10 | memleak 对账 / brk 观测 / 缺页 stackcount / compaction 耗时模板（THP 证据） | `bash -n` ✓；memleak/stackcount/trace-bpfcc 已装 |
