# 5.15 bpftrace 的下一步工作

> 底本：《BPF之巅》第 5 章 bpftrace（印刷 p137–190），5.15 节（印刷 p183–185）

## 内容详解

计划中的演进（拿到书时可能已完成，查发布文档）。

**读这节的正确姿势**：它是一份 2019 年的路线图，很多条目如今已落地。对学习者的价值不在"预测未来"，而在**理解每条演进背后的驱动力**——限制为什么存在、又是怎么被拆除的。

### 5.15.1 显式区分地址模式（最大改动）

背景：为支持部分处理器架构，`bpf_probe_read()` 将拆分为 `bpf_probe_read_kernel()` 与 `bpf_probe_read_user()`，bpftrace 相应要求**显式区分内核态/用户态地址**，并新增 `kstr()/ustr()` 函数。对书中工具无影响——bpftrace 按探针上下文**自动判定**地址空间：

| 探针上下文 | `*addr` / `str(addr)` | 跨空间访问 |
|------------|----------------------|-----------|
| kprobe/kretprobe（内核） | 按内核态解引用 | `*uptr(addr)` / `str(uptr(addr))` → 用户态 |
| uprobe/uretprobe（用户） | 按用户态解引用 | `*kptr(addr)` / `str(kptr(addr))` → 内核态 |
| 其他探针 | 默认内核态 | 特例：syscall 跟踪点带**用户地址空间**上下文 |

curtask() 之类函数无论上下文都返回内核指针（符合预期）。（极少数架构如 sparc32、老 x86 4G:4G 分隔模式确实需要这种区分——Linus 语。）

**这条已落地**：现代 bpftrace 提供 `kptr()/uptr()`；内核侧对应 `bpf_probe_read_kernel()/bpf_probe_read_user()`（v5.5 起，x86 上的分开寻址 helper）。书上"自动判定"的表如今读法是：默认规则不变，**跨空间访问必须显式标注**——/syscall 探针里读内核结构配 kptr，读用户缓冲配 uptr，写反了读回来的是别的地址空间的数据（不崩，但全是垃圾）。

### 5.15.2 其他扩展

逐条标注落地状态（★=已进入主线版本；以所用版本 reference guide 为准）：

- ★ 内存观察点（`watchpoint` 探针——监控变量被谁改）、socket 和 skb 程序（kfunc/iter 路线）、裸跟踪点（rawtracepoint）探针；
- 带偏移量的 uprobe/kprobe；
- ★ 支持 Linux 5.3 BPF 有界循环的 **for/while**；
- 裸 PMC 探针类型（掩码+事件选择）；
- uprobe 支持相对函数名（免完整路径）；
- ★ `signal()` 向进程发信号（含 SIGKILL）；
- `return()/override()` 重写事件返回值（bpf_override_return()）；
- **ehist()** 指数区间直方图（比 2 的幂 hist 精度更高）——截至近年版本仍未进入主线，**别在脚本里指望它**，hist()/lhist() 仍是唯二选择；
- **pcomm**：进程名（comm 是线程名，Java 每线程不同 comm，pcomm 恒为 "java"）；
- file 结构体指针→完整路径的辅助函数。

### 5.15.3 ply

Tobias Waldekranz 创建的 BPF 前端：类 bpftrace 高级语言，但**尽量避免依赖（不需要 LLVM/Clang）**——适合资源受限环境；代价是无法包含头文件/访问结构体成员。后续版本可能直接支持 bpftrace 语言 + BTF 获取结构体信息。示例（跟踪 open）：

```bash
# ply 'tracepoint:syscalls/sys_enter_open { printf("PID: %d (%s) opening: %s\n", pid, comm, str(data->filename)); }'
PID: 22737 (Chrome IOThread) opening: /dev/shm/.org.chromium.Chromium...
```

**ply 填的生态位**（和 Ftrace 形成"无 LLVM"双选项）：

| | bpftrace | ply | Ftrace |
|--|----------|-----|--------|
| 依赖 | LLVM/Clang + libbcc/libbpf | 几乎零依赖（自含编译器后端） | 内核自带 |
| 结构体成员访问 | ✓（Clang 解析头文件/BTF） | ✗（只能用 tracepoint 的 `data->` 原始字段） | tracepoint format 字段 |
| 语言表达力 | 高（聚合/hist/栈） | 中 | 自有语法 |
| 适用 | 服务器/交易机 | 深度嵌入式（路由器固件级） | 内核功能全覆盖 |

## HFT 关联

- pcomm vs comm：多线程引擎（尤其 Java 系策略/风控）按"进程"聚合用 pcomm，按线程用 comm；
- kptr/uptr 区分在读 syscall 参数（用户指针）×内核结构（内核指针）混用场景（如网络探针里读 skb）必须正确，否则读错地址空间数据；
- **演进意识是脚本资产的一部分**：runbook 里每个 .bt 脚本头部注明"验证于 bpftrace vX.Y / kernel A.B"——语言还在长（unroll→for/while、kptr 落地都是近年的事），不带版本戳的脚本两年后跑挂了说不清是谁的锅。

## 陷阱

- ⚠️ 书中代码基于"自动判定"时代；新版 bpftrace 若已强制地址模式，跨空间解引用要显式 uptr/kptr。
- ⚠️ ply 无 Clang → 无结构体成员访问；复杂脚本别指望 ply 跑得动。
- ⚠️ 表格里的"已落地"以你手上的版本为准——同一台机器上内核和 bpftrace 双双要够新（如 for/while 要内核 5.3+ **且** bpftrace 新版），单边达标没用。

<details>
<summary>自测题</summary>

1. bpf_probe_read() 为什么要拆分为 kernel/user 两个变体？
   <details><summary>答案</summary>部分处理器架构（如 sparc32、x86 4G:4G 分隔）内核态与用户态地址空间需要不同的读取方式。</details>

2. pcomm 和 comm 的区别？
   <details><summary>答案</summary>comm 是线程名（Java 每线程可不同）；pcomm 恒为进程名（如 "java"）。</details>

3. syscall 探针里同时要读 args->filename（用户指针）和当前 task_struct 的字段（内核指针），要注意什么？
   <details><summary>答案</summary>地址空间混用场景：syscall 跟踪点上下文按用户地址空间解引用，读内核结构要 kptr() 包住显式切换；写反了不报错但读回垃圾数据。</details>

4. ehist() 能用吗？
   <details><summary>答案</summary>书里是"计划中"，至今未进主线——别在脚本里用；指数精度需求用 lhist() 手工划桶替代。</details>
</details>
