# 6. BCC 调试与排障

工具 **编译失败**、**无输出**、**输出离谱** 时的手段。

### `bpf_trace_printk()` — 内核 printf

在 BPF C 里插入调试打印，从 trace pipe 读取：

```c
bpf_trace_printk("hit pid=%d\n", pid);
```

```bash
sudo cat /sys/kernel/debug/tracing/trace_pipe
# 或
sudo trace-cmd stream
```

**注意：** `printk` 格式有限、有开销；**调通后删除**。生产热路径禁用。

### Python 层 Debug Flags

在 BCC Python 脚本中开启（具体常量名以所用 bcc 版本为准）：

| 标志 | 作用 |
|------|------|
| `DEBUG_LLVM_IR` | 查看 LLVM IR |
| `DEBUG_BPF` | 预处理后的 BPF C、加载细节 |
| `DEBUG_SOURCE` | 源码与行号映射 |

用于：**验证 Clang 是否按预期编译**、**验证器拒绝原因**。

### 状态查看与清理

```bash
sudo bpftool prog list
sudo bpftool map list
# 部分环境
sudo bpflist-bpfcc
```

| 场景 | 做法 |
|------|------|
| 工具 **Ctrl-C 后探针残留** | 确认无孤儿 kprobe；必要时卸载模块或重启 tracing |
| **kprobe 过多** | 合并 probe、改用 tracepoint、缩短采集窗口 |
| **验证器失败** | 减循环、减栈深度、用 `bpf_probe_read` 替代直接解引用 |

→ 指令级：`bpftool prog dump` · [appendix-E-BPF指令.md](../../appendix-E-BPF指令.md)


### 常见陷阱

1. **BCC 工具无输出时不排查根因** — 常见原因：目标函数不存在、PID 错误、权限不足、事件未触发；应逐一排查而非反复重试
2. **忽视 BPF verifier 拒绝的错误信息** — verifier 拒绝时输出详细的拒绝原因（如「invalid bpf_context access」），这些信息是调试 BPF C 代码的关键线索
3. **在错误的内核版本上使用工具** — BCC 工具依赖特定内核功能（如某些 tracepoint 在旧内核不存在）；查看工具的 OS 字段确认兼容性

<details>
<summary>📝 自测题（点击展开）</summary>

1. **BCC 工具无输出的常见原因有哪些？如何排查？**

   <details>
   <summary>参考答案</summary>

   (1) 目标函数不存在：用 `bpftrace -l 'kprobe:func*'` 验证；(2) PID 错误：用 `ps` 确认进程在运行；(3) 权限不足：需要 root 或 CAP_BPF；(4) 事件未触发：用 strace 先确认事件确实发生；(5) 过滤条件太严：放宽 filter 重试。

   </details>

2. **BPF verifier 拒绝程序时如何调试？**

   <details>
   <summary>参考答案</summary>

   Verifier 输出包含拒绝点（指令编号、寄存器状态、访问的偏移）。常见原因：(1) 指针未做 bounds check；(2) 循环无确定上界；(3) 栈溢出（>512B）；(4) Map 操作类型不匹配。用 `bcc -e` 或 `bpftool prog dump` 看生成的字节码辅助调试。

   </details>

3. **HFT 排障中 BCC 工具报「probe not found」怎么办？**

   <details>
   <summary>参考答案</summary>

   (1) 确认内核版本——函数可能在旧内核叫不同名字（用 `cat /proc/kallsyms | grep func`）；(2) 改用 tracepoint 替代 kprobe（`ls /sys/kernel/debug/tracing/events/`）；(3) 检查函数是否被 inline 或优化掉（编译时可能不存在独立符号）；(4) 考虑用 uprobe 在用户态追踪等效路径。

   </details>

</details>

---
