# 6.4 BPF 单行程序

> 底本：《BPF之巅》第 6 章 CPU，6.4 节（印刷 p251–253）。尽量同时给出 BCC 与 bpftrace 两版 — 无 shell 选项记忆负担的"袖珍武器库"。

## 6.4.1 BCC 版

| 任务 | 命令 |
|------|------|
| 跟踪新进程（含参数） | `execsnoop` |
| 看谁在创建新进程 | `execsnoop`（PPID 列） |
| 按进程统计系统调用 | `syscount -P` |
| 按系统调用名统计 | `syscount` |
| 49Hz 采样 PID 189 的用户态栈 | `profile -F 49 -u -p 189` |
| 采样所有调用栈+进程 | `profile` |
| 统计 vfs* 内核函数调用频率 | `funccount 'vfs_*'` |
| 跟踪 pthread_create 新线程 | `trace '/lib/x86_64-linux-gnu/libpthread-2.27.so:pthread_create'` |

## 6.4.2 bpftrace 版

```bash
# 跟踪新进程（含参数）
bpftrace -e 't:syscalls:sys_enter_execve { join(args->argv); }'

# 输出"哪个进程执行了什么"：comm → filename
bpftrace -e 't:syscalls:sys_enter_execve { printf("%s -> %s\n", comm, str(args->filename)); }'

# 按进程名统计系统调用
bpftrace -e 't:raw_syscalls:sys_enter { @[comm] = count(); }'

# 按 PID+comm 统计系统调用
bpftrace -e 't:raw_syscalls:sys_enter { @[pid, comm] = count(); }'

# 按系统调用函数名统计（kaddr + sys_call_table 反查名字）
bpftrace -e 't:raw_syscalls:sys_enter { @[sym(*(kaddr("sys_call_table") + args->id*8))] = count(); }'

# 99Hz 采样正在运行的进程名
bpftrace -e 'profile:hz:99 { @[comm] = count(); }'

# 49Hz 采样 PID 189 的用户态栈
bpftrace -e 'profile:hz:49 /pid == 189/ { print(ustack); clear(@x); }'   # 书中核心形式：@ = ustack 采样

# 采样所有进程名和调用栈
bpftrace -e 'profile:hz:49 { @[ustack, kstack, comm] = count(); }'

# 99Hz 采样当前 CPU 编号，线性直方图（看负载均衡）
bpftrace -e 'profile:hz:99 { @cpu = lhist(cpu, 0, 256, 1); }'

# 统计 vfs* 内核函数调用频率
bpftrace -e 'kprobe:vfs_* { @[func] = count(); }'

# 按名字+内核栈统计 SMP 调用
# （smpcalls.bt 的简化版，见 6.3.15）

# 按名字+内核栈统计 Intel x2APIC IPI
bpftrace -e 'kprobe:x2apic_send_IPI* { @[probe, kstack(5)] = count(); }'

# 跟踪 pthread_create（打印探针、进程、PID）
bpftrace -e 'u:/lib/x86_64-linux-gnu/libpthread-2.27.so:pthread_create { printf("%s by %s (%d)\n", probe, comm, pid); }'
```

值得注意的技巧：

- **`kaddr("sys_call_table") + args->id*8`**：raw_syscalls 只给 ID，通过系统调用表指针运算 + `sym()` 直接取函数名 — 单探针达到 syscount 效果
- **`lhist(cpu, 0, 256, 1)`**：把内置变量 cpu 当统计对象 — CPU 编号分布均匀 = 负载均衡正常
- **`kstack(5)`**：限定栈深度，IPI 这种高频事件防输出爆炸

## HFT 关联

- 一行 `profile:hz:99 { @[comm] = count(); }` 就是最轻量的"谁在吃核"巡检，可常驻低频跑
- `lhist(cpu,...)` 验证策略线程是否均匀落在绑定核集合上
- x2APIC IPI 单行量化核间打断频率 — 隔离核调优前后的对照指标

## 常见陷阱

1. **单行里忘过滤 pid==0** — idle 线程会把采样结果全淹没（swapper 占大头）
2. **对高频事件直接 print 栈** — 输出量失控；用 count() 聚合或 kstack(5) 限深
3. **bpftrace 版无选项** — 单行是核心功能演示，生产用完整工具（BCC 版）或把单行存成 .bt 加选项

<details>
<summary>📝 自测题（点击展开）</summary>

1. **如何用一个 bpftrace 单行把 raw_syscalls 的 ID 转成函数名统计？**

   <details>
   <summary>参考答案</summary>

   `t:raw_syscalls:sys_enter { @[sym(*(kaddr("sys_call_table") + args->id*8))] = count(); }` — kaddr 取 sys_call_table 地址，ID×8（指针大小）偏移到对应表项，解引用得函数地址，sym() 转名字。单探针 + 名字统计，兼具性能与可读性。
   </details>

2. **`profile:hz:99 { @cpu = lhist(cpu, 0, 256, 1); }` 能回答什么问题？**

   <details>
   <summary>参考答案</summary>

   采样期间 CPU 编号的分布 — 是否所有核被均匀使用（负载均衡）、是否某些核特别热（绑核/中断亲和集中）。分布平坦=均衡；尖峰=负载集中。
   </details>

</details>
