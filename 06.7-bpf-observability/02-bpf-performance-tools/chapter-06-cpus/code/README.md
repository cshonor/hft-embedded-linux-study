# 02-ch06 code · CPU 域 bpftrace 程序

> 《BPF Performance Tools》第 6 章示例落盘。
> 笔记中 perf 传统工具命令（§2 块4-7）已在 06.6/ch13 充分落盘，此处不重复。

## 程序（书上经典 .bt 完整版）

| 文件 | 出处 | 内容 | 验证（bpftrace 0.20.2） |
|------|------|------|------|
| [execsnoop.bt](./execsnoop.bt) | §3 块8 | 新进程追踪含参数（join argv + 毫秒时间戳表头） | 语义解析 ✓（实跑需 root） |
| [runqlat.bt](./runqlat.bt) | §4 块9 | 运行队列延迟直方图（wakeup→switch 配对，调度延迟金标准） | 语义解析 ✓ |
| [runqlen.bt](./runqlen.bt) | §4 块10 | 运行队列长度（99Hz 采 cfs_rq.nr_running - 1） | 语义解析 ✓ |
| [fork-trace.bt](./fork-trace.bt) | §11 块2 | sched_process_fork 一个点覆盖 fork/clone/vfork | 语义解析 ✓ |

## scripts/

| 脚本 | 出处 | 内容 | 验证 |
|------|------|------|------|
| [cpu-oneliners.sh](./scripts/cpu-oneliners.sh) | §10 块1、§11 块3 | 单行精选：execve/syscall 聚合（含 sys_call_table 反查）/profile 采样四式/IPI/自愿与非自愿切换思路 | `bash -n` ✓ |

## 与 BCC 成品的对应

| .bt（本书实现） | BCC 成品 | 关系 |
|---|---|---|
| runqlat.bt | runqlat-bpfcc | 同原理，BCC 版支持按 PID/CPU 过滤 |
| runqlen.bt | runqlen-bpfcc | 同上 |
| execsnoop.bt | execsnoop-bpfcc | BCC 版有返回值追踪 |
