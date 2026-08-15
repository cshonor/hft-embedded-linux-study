# 5.5 bpftrace 单行程序

> 底本：《BPF之巅》第 5 章 bpftrace（印刷 p137–190），5.5 节（印刷 p145–146）

## 内容详解

单行程序既有用，又是展示 bpftrace 能力的最佳教材。注意：**许多单行在内核内存中统计数据，Ctrl-C 后才打印摘要**。

### 书中单行清单（`-e` 执行）

```bash
# 1. 谁在执行什么命令（execve 入参）
bpftrace -e 'tracepoint:syscalls:sys_enter_execve { printf("%s -> %s\n", comm, str(args->filename)); }'

# 2. 新进程创建及参数

# 3. openat() 打开的文件，按进程打印
bpftrace -e 'tracepoint:syscalls:sys_enter_openat { printf("%s %s\n", comm, str(args->filename)); }'

# 4. 按程序统计系统调用次数
bpftrace -e 'tracepoint:raw_syscalls:sys_enter { @[comm] = count(); }'

# 5. 按系统调用探针名计数

# 6. 按进程统计系统调用数量

# 7. 按进程统计 read 总字节数
bpftrace -e 'tracepoint:syscalls:sys_exit_read /args->ret > 0/ { @[comm] = sum(args->ret); }'

# 8. 按进程展示 read 返回值分布
bpftrace -e 'tracepoint:syscalls:sys_exit_read { @[comm] = hist(args->ret); }'

# 9. 进程的磁盘 I/O 尺寸
bpftrace -e 'tracepoint:block:block_rq_issue { printf("%d %s %d\n", pid, comm, args->bytes); }'

# 10. 按进程统计页换入（major faults）
bpftrace -e 'software:major-faults:1 { @[comm] = count(); }'

# 11. 按进程统计缺页中断
bpftrace -e 'software:faults:1 { @[comm] = count(); }'

# 12. 对 PID 189 以 49Hz 抓取用户态调用栈
```

### 共性套路

| 套路 | 模式 |
|------|------|
| 看细节 | `printf("%s %s\n", comm, str(args->xxx))` |
| 计数 | `@[comm] = count()`（键可为 comm/pid/probe） |
| 求和 | `@[comm] = sum(args->ret)`（配过滤滤掉负值错误码） |
| 分布 | `@[comm] = hist(...)` |
| 软件事件采样 | `software:faults:1`（每 1 次触发） |

更多单行见**附录 A**（完整单行宝典）与附录 B 备忘单。

## HFT 关联

- 这 12 条就是**分钟级健康检查**的雏形：read 分布看 I/O 尺寸、faults 看内存行为、按 comm 计数看进程活跃度——不用装任何监控 Agent；
- 把单行固化到 runbook 时，务必加**运行时长控制**（`interval:s:N { exit(); }`），避免无限跑。

## 陷阱

- ⚠️ `sum(args->ret)` 必须加 `/args->ret > 0/` 过滤——read 的负返回值是错误码，混入求和结果完全失真；
- ⚠️ 单行默认**不带时长限制**，Ctrl-C 才出摘要——自动化脚本里要配 interval+exit。

<details>
<summary>自测题</summary>

1. 统计 read 总字节数时为什么要过滤 `args->ret > 0`？
   <details><summary>答案</summary>read(2) 负返回值是 -errno 错误码，不是字节数。</details>

2. 单行程序的结果什么时候打印？
   <details><summary>答案</summary>多数在 Ctrl-C 退出时统一打印摘要（内核映射表统计）。</details>
</details>
