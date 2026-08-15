# 追踪类（perf-related）

设计初衷：把事件信息高效报告给用户态，**不影响**内核行为（返回码被忽略）。`bpftool perf show` 可查看 perf 类挂载（pid/fd/prog_id/类型/函数名）。

### 2.1 五种 execve 入口挂法对比（本章核心实验）

同一事件的五种程序，全在 chapter7/hello.bpf.c：

| 挂法 | SEC 名 | 宏 | 上下文 |
|---|---|---|---|
| syscall kprobe | `ksyscall/execve` | `BPF_KPROBE_SYSCALL(name, char *pathname)` | syscall 参数（稳定接口） |
| 内核函数 kprobe | `kprobe/do_execve` | `BPF_KPROBE(name, struct filename *filename)` | 内核函数参数 |
| fentry | `fentry/do_execve` | `BPF_PROG(name, struct filename *filename)` | 内核函数参数 |
| tracepoint | `tp/syscalls/sys_enter_execve` | 手写结构体 | 从格式文件映射好的结构 |
| BTF tracepoint | `tp_btf/sched_process_exec` | `BPF_PROG(...)` | vmlinux.h 里的 `trace_event_raw_*` 结构 |

### 2.2 kprobe 要点

- 几乎可挂内核任何位置（黑名单除外：`/sys/kernel/debug/kprobes/blacklist`）；可挂函数入口、也可挂"入口+N 条指令"的偏移处——但任意偏移在版本间极不稳定，别这么干
- **被内联的函数没有 kprobe 入口点**
- syscall 入口是稳定接口；但**基于 syscall kprobe 的探针不能当安全工具用**（可被绕过，第 9 章详述）
- 挂普通内核函数时，参数类型要照抄内核函数签名（`do_execve(struct filename *filename, ...)`）——参数按内存顺序排列，**可忽略尾部参数，不能跳过前面的**
- kretprobe 挂函数返回处，只能拿到返回值 `ret`，**拿不到入参**

### 2.3 fentry/fexit（5.5 引入，x86；ARM 要 6.0）

- 基于 **BPF trampoline**，比 kprobe/kretprobe 更高效——新内核上的首选
- `BPF_PROG` 宏适用于 fentry / fexit / tracepoint
- 关键优势：**fexit 同时能拿入参和返回值**（kretprobe 只有返回值）：

```c
SEC("kretprobe/do_unlinkat")
int BPF_KRETPROBE(do_unlinkat_exit, long ret)                 // 只有 ret

SEC("fexit/do_unlinkat")
int BPF_PROG(do_unlinkat_exit, int dfd, struct filename *name, long ret)  // 全都有
```

### 2.4 tracepoint 要点

- 内核代码里**静态标记**的位置（早于 eBPF 存在，SystemTap 也用）；版本间稳定；5.15 内核有 1400+ 个
- 清单：`/sys/kernel/tracing/available_events`；格式：`/sys/kernel/tracing/events/<子系统>/<事件>/format`
- 手写上下文结构要照抄 format 文件（offset/size 对齐）；**前 4 个公共字段（common_type 等）不允许访问**，否则 `invalid bpf_context access`
- **raw_tp**：跳过参数映射直接拿原始 `__u64` 参数，更快但要自己做类型转换（syscall 入口参数还与架构相关）
- **tp_btf**：BTF 自动提供匹配结构（`trace_event_raw_<事件名>`，vmlinux.h 里），免手写、免版本漂移风险——首选

### 2.5 用户态挂点

uprobe / uretprobe / USDT 都用 `BPF_PROG_TYPE_KPROBE` 程序类型：

```c
SEC("uprobe/usr/lib/aarch64-linux-gnu/libssl.so.3/SSL_write")
```

典型应用：钩 SSL 库输出**解密后**的明文（第 8 章）、Parca 持续 profiling。

四大约束：
1. 共享库路径**架构相关**
2. 目标机器装了什么库不可控
3. 应用可能静态链接（共享库探针全 miss）；容器内库路径与宿主机不同
4. **语言调用约定差异**：C 参数走寄存器，Go 1.17 之前参数走栈——pt_regs 取参对老 Go 二进制无效

### 2.6 LSM

- `BPF_PROG_TYPE_LSM` 挂 Linux Security Module API（稳定接口，原本给内核模块用）；经 `bpf(BPF_RAW_TRACEPOINT_OPEN)` 附加
- 与追踪类的本质区别：**返回码影响内核行为**——非零 = 安全检查不通过，内核拒绝该操作（第 9 章）
