# 2. BCC 架构与特性

### 编译与加载流程

```
用户脚本 (Python 等)
    → 嵌入 BPF C 源码
    → Clang/LLVM 编译为 BPF 字节码
    → bpf() 系统调用加载程序 + 创建 Map
    → 附加到 kprobe/uprobe/tracepoint/USDT 等
    → 用户态轮询/读取 Map，格式化输出
```

### 内核级能力

| 能力 | 典型用途 |
|------|----------|
| **动态 kprobes / uprobes** | 任意内核/用户函数插桩（需符号） |
| **Tracepoint** | 稳定、低开销的内核静态探针 |
| **BPF Map** | 直方图、频率计数、聚合 — **海量事件在内核汇总** |
| **栈回溯** | `bpf_get_stackid` + 栈 Map → `stackcount` / `profile` |

### 用户级能力

| 能力 | 说明 |
|------|------|
| **USDT** | 用户态静态探针（需应用带 SDT 探针，如某些数据库/语言运行时） |
| **debuginfo 符号解析** | 内核/用户栈、函数名 — 依赖 debug 包或 BTF |
| **Python 胶水** | 参数解析、输出格式化、与 CLI 集成 |

```bash
# 常见安装名（发行版差异）：bcc-tools / python3-bcc
ls /usr/share/bcc/tools/ | head
man opensnoop-bpfcc    # 或 bcc-opensnoop 等，视发行版而定
```

→ 自研工具深入：[appendix-C-BCC工具开发.md](../../appendix-C-BCC工具开发.md) · C/libbpf 路线：[appendix-D-C语言BPF.md](../../appendix-D-C语言BPF.md)

---
