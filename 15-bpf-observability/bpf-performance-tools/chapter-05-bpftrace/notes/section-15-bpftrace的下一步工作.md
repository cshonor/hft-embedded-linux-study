# 5.15 bpftrace 的下一步工作

> 底本：《BPF之巅》第 5 章 bpftrace（印刷 p137–190），5.15 节（印刷 p183–185）

## 内容详解

计划中的演进（拿到书时可能已完成，查发布文档）。

### 5.15.1 显式区分地址模式（最大改动）

背景：为支持部分处理器架构，`bpf_probe_read()` 将拆分为 `bpf_probe_read_kernel()` 与 `bpf_probe_read_user()`，bpftrace 相应要求**显式区分内核态/用户态地址**，并新增 `kstr()/ustr()` 函数。对书中工具无影响——bpftrace 按探针上下文**自动判定**地址空间：

| 探针上下文 | `*addr` / `str(addr)` | 跨空间访问 |
|------------|----------------------|-----------|
| kprobe/kretprobe（内核） | 按内核态解引用 | `*uptr(addr)` / `str(uptr(addr))` → 用户态 |
| uprobe/uretprobe（用户） | 按用户态解引用 | `*kptr(addr)` / `str(kptr(addr))` → 内核态 |
| 其他探针 | 默认内核态 | 特例：syscall 跟踪点带**用户地址空间**上下文 |

curtask() 之类函数无论上下文都返回内核指针（符合预期）。（极少数架构如 sparc32、老 x86 4G:4G 分隔模式确实需要这种区分——Linus 语。）

### 5.15.2 其他扩展

- 内存观察点、socket 和 skb 程序、裸跟踪点探针；
- 带偏移量的 uprobe/kprobe；
- 支持 Linux 5.3 BPF 有界循环的 **for/while**；
- 裸 PMC 探针类型（掩码+事件选择）；
- uprobe 支持相对函数名（免完整路径）；
- `signal()` 向进程发信号（含 SIGKILL）；
- `return()/override()` 重写事件返回值（bpf_override_return()）；
- **ehist()** 指数区间直方图（比 2 的幂 hist 精度更高）；
- **pcomm**：进程名（comm 是线程名，Java 每线程不同 comm，pcomm 恒为 "java"）；
- file 结构体指针→完整路径的辅助函数。

### 5.15.3 ply

Tobias Waldekranz 创建的 BPF 前端：类 bpftrace 高级语言，但**尽量避免依赖（不需要 LLVM/Clang）**——适合资源受限环境；代价是无法包含头文件/访问结构体成员。后续版本可能直接支持 bpftrace 语言 + BTF 获取结构体信息。示例（跟踪 open）：

```bash
# ply 'tracepoint:syscalls/sys_enter_open { printf("PID: %d (%s) opening: %s\n", pid, comm, str(data->filename)); }'
PID: 22737 (Chrome IOThread) opening: /dev/shm/.org.chromium.Chromium...
```

## HFT 关联

- pcomm vs comm：多线程引擎（尤其 Java 系策略/风控）按"进程"聚合用 pcomm，按线程用 comm；
- kptr/uptr 区分在读 syscall 参数（用户指针）×内核结构（内核指针）混用场景（如网络探针里读 skb）必须正确，否则读错地址空间数据。

## 陷阱

- ⚠️ 书中代码基于"自动判定"时代；新版 bpftrace 若已强制地址模式，跨空间解引用要显式 uptr/kptr。
- ⚠️ ply 无 Clang → 无结构体成员访问；复杂脚本别指望 ply 跑得动。

<details>
<summary>自测题</summary>

1. bpf_probe_read() 为什么要拆分为 kernel/user 两个变体？
   <details><summary>答案</summary>部分处理器架构（如 sparc32、x86 4G:4G 分隔）内核态与用户态地址空间需要不同的读取方式。</details>

2. pcomm 和 comm 的区别？
   <details><summary>答案</summary>comm 是线程名（Java 每线程可不同）；pcomm 恒为进程名（如 "java"）。</details>
</details>
