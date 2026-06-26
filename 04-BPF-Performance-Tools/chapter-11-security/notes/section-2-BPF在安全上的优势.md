# 2. BPF 在安全上的优势

### 零日 / 应急响应

| 传统 | bpftrace/BCC |
|------|--------------|
| 等 vendor 补丁或静态规则 | **数分钟** 写自定义脚本 |
| 固定 audit 规则 | **kprobe/uprobe/tracepoint** 追任意函数参数 |

**例：** 新披露漏洞涉及某 syscall → 临时 `tracepoint:syscalls:sys_enter_*` 或 `kprobe` 看调用栈与参数。

→ 语法：[Ch 5 bpftrace](../../chapter-05-bpftrace/)

### 性能 vs `auditd`

Gregg 2016 内部对比（书中引用）：

| | **auditd** | **等价 BPF** |
|---|------------|--------------|
| 开销 | 基准 | 约 **低 6×** |
| 粒度 | 规则驱动日志 | 内核过滤 + 聚合 |

**HFT：** 若合规要求 syscall 审计，评估 **BPF 替代/补充** 对 **P99** 的影响 — 仍须短规则、限 PID。

### seccomp + BPF

**seccomp** 用 **BPF 程序** 在 **syscall 入口** 决定 allow/deny — 安全策略即「小型 BPF 过滤器」。

```
syscall 入口 → seccomp BPF 程序 → ALLOW / ERRNO / TRAP / KILL
```

**HFT：** 策略进程 **沙箱化** 时可参考 `capable`/`eperm` 观测结果反推 seccomp 规则集。

### BPF 自身安全配置

|  sysctl / 配置 | 作用 |
|----------------|------|
| **`kernel.unprivileged_bpf_disabled=1`** | 禁止非 root 加载 BPF（推荐生产） |
| **`net.core.bpf_jit_harden`** / **`bpf_jit_harden`** | JIT 强化，降低代码注入面 |
| 限制 perf_event / ptrace | 纵深防御（发行版策略各异） |

**运维：** 观测工具需要 **root/CAP_BPF/CAP_PERFMON** — 与 **最小权限** 平衡。

---
