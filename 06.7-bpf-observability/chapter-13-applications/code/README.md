# 02-ch13 code · 应用域 bpftrace 程序

> 《BPF Performance Tools》第 13 章示例落盘。
> 笔记中 MySQL USDT（§7 块6）并入 app-oneliners.sh。

## 程序

| 文件 | 出处 | 内容 | 验证（bpftrace 0.20.2） |
|------|------|------|------|
| [threadsnoop.bt](./threadsnoop.bt) | §2 块2 | pthread_create 追踪（usym(arg2) 译线程入口函数名） | 语义解析 ✓（实跑需 root） |
| [threaded.bt](./threaded.bt) | §3 块3 | 按线程采样 CPU（99Hz，[comm,tid] 每秒滚动打印，位置参数 $1） | 语义解析 ✓ |
| [offcpu.bt](./offcpu.bt) | §4 块4 | off-CPU 阻塞栈直方图（finish_task_switch 配对，四维聚合键） | 语义解析 ✓ |
| [ioustack.bt](./ioustack.bt) | §5 块5 | I/O syscall 全变体通配 + 用户栈聚合 | 语义解析 ✓ |

## scripts/

| 脚本 | 出处 | 内容 | 验证 |
|------|------|------|------|
| [app-oneliners.sh](./scripts/app-oneliners.sh) | §10 块1、§7 块6 | 单行七条（execve/profile/malloc 栈/kill 追踪/pthread 计数/LLC miss）+ MySQL USDT 查询计时 | `bash -n` ✓ |

## 要点

- `usym(arg2)`：把线程入口函数地址译成名字（需二进制有符号）
- offcpu.bt 是 offcputime-bpfcc 的教学版——理解原理后用成品
- `hardware:cache-misses` 需要 PMU（本机 perf_event_paranoid=4 限制了非 root 使用）
