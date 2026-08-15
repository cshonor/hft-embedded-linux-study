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
| **XDP / tc** | 网络最早/ qdisc | [17-BPF XDP note](../../../15-bpf-observability/note-XDP与tc-BPF.md) |

---


### 常见陷阱

1. Verifier 不理解就绕过——Verifier 是安全保证（无越界/有界循环/类型安全），绕过 = 内核崩溃风险
2. Ring Buffer vs Maps 不分场景——高频事件用 map 聚合（低开销），低频事件用 ring buffer（明细）
3. kprobe 和 tracepoint 不分优先级——优先 tracepoint（稳定 ABI），kprobe 是后备

<details>
<summary>自测题（点击展开）</summary>

1. BPF Verifier 保证什么？
   <details><summary>答</summary>无越界访问、有界循环（不能死循环）、类型安全指针——是生产安全的基础</details>
2. Ring Buffer 和 BPF Maps 的场景区别？
   <details><summary>答</summary>Ring Buffer 适合低频事件明细输出；Maps 适合高频事件聚合（计数/直方图）——高频用 map 低开销</details>
3. 为什么优先 tracepoint 而非 kprobe？
   <details><summary>答</summary>tracepoint 是稳定 ABI 不会随内核版本变；kprobe 追踪的函数名可能变更而失效</details>

</details>


---

← [本章导读](../README.md)
