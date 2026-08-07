# 1. BPF 与 eBPF

### 起源与演进

| 阶段 | 要点 |
|------|------|
| **经典 BPF** | BSD **包过滤器** — tcpdump 在内核过滤包，减少拷贝到用户态 |
| **eBPF（扩展 BPF）** | 2014+ 通用内核 VM — 寄存器 **2→10**、宽度 **32→64 bit**、**无上限 Map**、可调用 **helper**、经 **验证器** 保证安全 |

### 为什么性能工具需要 BPF

| 传统路径 | BPF 路径 |
|----------|----------|
| 海量事件 **拷贝到用户态** 再聚合 | **内核态** 过滤、计数、建直方图 |
| 高频率 syscall/trace 开销大 | 仅把 **聚合结果**（map、histogram）送到用户态 |

**例子：** `biolatency` 在内核按延迟桶 `++`，用户态只读 map 画图 — 不是每条 I/O 都上报。

### 开发、辅助函数与调试

| 组件 | 作用 |
|------|------|
| **BPF 程序** | C / LLVM → 字节码 → `bpf()` 加载 |
| **Helper** | 内核提供的安全 API，例如：`bpf_map_lookup_elem`、`bpf_probe_read`、`bpf_ktime_get_ns`、`bpf_get_stackid` |
| **bpftool** | 查看已加载程序、map、指令、`bpftool prog dump` 等 |

```bash
sudo bpftool prog list
sudo bpftool map list
```

### 前沿：BTF 与 CO-RE

| 技术 | 解决的问题 |
|------|------------|
| **BTF** (BPF Type Format) | 内核数据结构类型信息 — 供验证器与工具理解 layout |
| **CO-RE** (Compile Once – Run Everywhere) | 不同内核版本 **结构体偏移不同** — 编译期记录偏移，运行时 **relocate**，避免硬编码 `offsetof` |

→ 新工具链渐迁 **libbpf + CO-RE**；本书 BCC 仍大量可用，见 [appendix-D-C语言BPF.md](../../appendix-D-C语言BPF.md)。


### 常见陷阱

1. **混淆 BPF 程序和 BPF Map** — BPF 程序是执行逻辑（探针命中时跑的代码），Map 是数据存储（跨事件共享结果）；新手常把两者混为一谈
2. **以为 CO-RE 消除了所有内核版本兼容问题** — CO-RE 解决结构体偏移重定位，但不保证函数签名和语义不变；kprobe 目标函数本身可能被重命名或删除
3. **忽视 verifier 的安全检查对编程的限制** — verifier 要求所有指针访问有边界检查、循环有上界，这限制了能写的逻辑复杂度；复杂聚合需拆分为多个简单 Map 操作

<details>
<summary>📝 自测题（点击展开）</summary>

1. **经典 BPF 和 eBPF 的主要区别有哪些？**

   <details>
   <summary>参考答案</summary>

   (1) 寄存器数：2 → 10；(2) 数据宽度：32 → 64 bit；(3) Map 无上限（经典 BPF 只有简单累加器）；(4) 可调用内核 helper 函数；(5) 经验证器保证安全终止。eBPF 从包过滤器升级为通用内核 VM。

   </details>

2. **BPF Map 的作用是什么？为什么对性能工具至关重要？**

   <details>
   <summary>参考答案</summary>

   Map 是 BPF 程序与用户态之间、以及多次 probe 触发之间共享数据的键值存储。性能工具在内核态用 Map 聚合结果（如延迟直方图、计数表），用户态只读最终汇总，避免海量事件逐条上报到用户态。

   </details>

3. **CO-RE 解决了什么问题？有什么局限性？**

   <details>
   <summary>参考答案</summary>

   CO-RE (Compile Once – Run Everywhere) 解决不同内核版本间结构体成员偏移不同的问题——编译期记录重定位信息，运行时按目标内核修正。局限性：(1) 仅解决偏移，不解决函数签名变化；(2) 需要 BTF 信息支持；(3) kprobe 目标函数仍可能被重命名。

   </details>

</details>

---
