## 7.1–7.2 内存核心概念

### 虚拟内存（Virtual Memory）

| 作用 | 说明 |
|------|------|
| **抽象** | 进程看到私有、连续（逻辑上）的地址空间 |
| **隔离** | 进程 A 不能踩进程 B 的页 |
| **多任务** | 物理内存有限，虚拟空间可远大于 RAM |
| **Overcommit** | 内核允许承诺超过物理内存的映射 — **OOM 风险** |

**HFT：** 策略进程 + order book + 页缓存 + 监控 — 要算 **真实 RSS/PSS**，不能假设「还有 free 就安全」。

→ [01-CSAPP Ch9](../../../01-CSAPP-3rd/chapter-09-虚拟内存/) · [06-Gorman](../../../06-Linux-Virtual-Memory-Manager/)

### 按需分页与缺页异常

```
进程访问虚拟地址
    ↓
页表项无效 / 未 present？
    ↓ 是
Page Fault（缺页异常）
    ↓
内核：分配物理页 / 读入文件页 / COW / Swap-in
    ↓
返回用户态继续执行
```

| 缺页类型 | 含义 | 性能 |
|----------|------|------|
| **Minor fault** | 页已在内存，仅更新页表（如 COW、首次 touch 已分配页） | 相对轻 |
| **Major fault** | 需 I/O：读文件页或 **Swap-in** | **重** — 微秒～毫秒级 |

**HFT：** 热路径上 unexpected **major fault** = 延迟尖刺；启动后应 **预热（touch）** 关键数据结构，或启动时 mlock。

### 换页 vs 交换（Paging vs Swapping）

| 术语 | Linux 语境 | 好坏 |
|------|------------|------|
| **File system paging** | 文件映射页在 **page cache** 中换入换出 | 通常可接受（读 mmap 文件等） |
| **Anonymous paging** | 堆、栈、匿名 mmap — **无文件后备** | Swap 到磁盘时 **极慢** |
| **Swapping** | Gregg/Linux 常 **特指匿名页换出到 swap 设备** | **坏** — HFT 裸机目标：si/so ≈ 0 |

```bash
vmstat 1
# si = swap in,  so = swap out  —  任一持续非 0 要立刻查
```

### 工作集大小（WSS）

**WSS** = 进程在一段时间内 **实际频繁访问** 的页面集合大小。

| WSS 相对资源 | 表现 |
|--------------|------|
| WSS ⊂ L3 cache | 最理想 — 与 Ch 6 IPC 高一致 |
| WSS ⊂ RAM | 正常 — 无 Swap |
| WSS > RAM | **Thrashing** — Swap 风暴，系统假死 |

**估算：** BPF 实验工具 `wss`、perf 缺页采样、或短期 `pmap`/RSS 观测 — 用于容量规划与 leak 排查。

**HFT：** order book 常驻结构 = WSS 主体；**预分配 + 池化** 让 WSS 稳定、可预测，避免运行期堆膨胀。

---


---

← [本章导读](../README.md)
