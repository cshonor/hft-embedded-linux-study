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

---
