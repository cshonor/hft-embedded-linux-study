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

---
