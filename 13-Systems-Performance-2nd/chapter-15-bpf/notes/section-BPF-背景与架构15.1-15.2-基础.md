## BPF 背景与架构（15.1–15.2 基础）

### 演进

| 阶段 | 内容 |
|------|------|
| **经典 BPF（1992）** | Berkeley Packet Filter — tcpdump 加速包过滤 |
| **eBPF（2013+）** | 通用 **内核态 VM** — 追踪、网络(XDP)、安全、调度… |
| **第二版 SysPerf** | 全书工具链 **perf / Ftrace / BCC / bpftrace** 四支柱 |

### 安全：Verifier

```
用户编写 BPF 程序 → 加载到内核
    → Verifier 静态分析（边界、循环、指针）
    → 通过 → 附加到 hook（kprobe/tracepoint/XDP…）
    → 失败 → 拒绝加载（看 dmesg / bpftool）
```

| Verifier 保证 | 含义 |
|---------------|------|
| 无越界访问 | 不能乱读内核内存 |
| 有界循环 | 不能死循环拖死内核 |
| 类型安全 | 指针追踪 |

**HFT：** 生产只跑 **已知脚本**；自定义 bpftrace 先在 **staging** 验证加载。

### 数据输出：Ring Buffer vs Maps

| 机制 | 用途 | 开销 |
|------|------|------|
| **perf ring buffer** | **每事件** 明细（栈、timestamp、字段）→ 用户态 | 高事件率时大 |
| **BPF maps** | 内核 **聚合** — 计数、直方图、哈希 | 低 — 只读汇总 |

```
高频率 sched_switch：
  ❌ 每条送到用户态 → 打爆
  ✅ map histogram / BCC 内置聚合 → 只看分布
```

**Map 类型（常见）：**

| 类型 | 用途 |
|------|------|
| `HASH` / `ARRAY` | KV 计数 |
| `HISTOGRAM` | 延迟直方图（log2 桶） |
| `PERCPU_*` |  per-CPU 计数 — 减锁 |
| `STACK_TRACE` | 栈 ID 映射 |

→ Ch 14 [Ftrace hist](../../chapter-14-ftrace/) 对比

### 挂载点（Hook）概览

| Hook | 说明 | 例子 |
|------|------|------|
| **tracepoint** | 稳定内核静态点 | syscalls、sched、block |
| **kprobe/kretprobe** | 内核函数动态 | `tcp_sendmsg` |
| **uprobe** | 用户函数 | strategy 内函数 |
| **USDT** | 用户静态探针 | 应用预埋 |
| **XDP / tc** | 网络最早/ qdisc | [14-BPF XDP note](../../../14-BPF-Performance-Tools/note-XDP与tc-BPF.md) |

---


---

← [本章导读](../README.md)
