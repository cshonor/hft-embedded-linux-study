## 3.3–3.4 内核演进与 Linux 特性

### 历史演变（选读摘要）

|  lineage | 对现代性能的影响 |
|----------|------------------|
| **Unix** | 进程模型、syscall 接口 |
| **BSD** | 按需分页、高性能 TCP/IP 栈思路 |
| **Solaris** | **VFS**、**Slab 分配器**、**DTrace**、ZFS |

**要点：** 今天 Linux 里的 VFS、slab、观测文化，很多来自这条演进线 — 不是「Linux 独有发明一切」。

→ 设计脉络：[LKD Ch 1 简介](../../../05-Linux-Kernel-Development/00_Book_3rd_Notes/chapter-01-intro/)（Unix 基因与 Linux 对比）

---

### Linux 性能相关里程碑

| 引入 | 作用 |
|------|------|
| **O(1) / CFS 调度器** | 可扩展调度 |
| **RCU** | 读多写少路径低争用同步 |
| **epoll** | 高并发 I/O 多路复用（相对 select/poll） |
| **cgroups** | 资源隔离与限额 |
| **THP（透明大页）** | 减少 TLB miss，亦有延迟抖动争议 |
| **KVM** | 硬件虚拟化（共置 vs 云） |

---

### Linux 现代性能焦点（三件）

#### 1. systemd

- 现代服务管理器；**`systemd-analyze`** 可分解**启动时间**（哪 unit 慢）。
- HFT 裸机：关注 **服务依赖、After=、是否拖慢共置机器就绪**；热路径进程常不由 systemd 频繁重启。

#### 2. KPTI（Meltdown 缓解）

- **内核页表隔离**：修复 CPU 侧信道漏洞，增加 **syscall / 上下文切换** 时的页表与 **TLB 刷新** 开销。
- 影响：**约 0.1%–6%**（ workload 依赖）；syscall 密集或切换频繁时更明显。
- HFT：评估是否可用 **PCID**、内核版本、mitigations 开关（与安全合规权衡）→ 与 [10-HFT ch05](../../../15-HFT-Low-Latency-Practice/chapter-05-操作系统内核极致调优/) 对照。

#### 3. Extended BPF（eBPF）

- 内核态 **安全虚拟机**：验证后运行，**可编程观测**（tracepoint、kprobe、uprobe、XDP/tc-BPF…）。
- 驱动 **BCC、bpftrace** 等 — Gregg 称「当前最重要技术之一」。
- **全书观测主线：** [Ch 15 BPF](../../chapter-15-bpf/) → [04-BPF-Performance-Tools](../../../04-BPF-Performance-Tools/)

```
Ch 3 知道「BPF 能在内核里安全插桩」
  → Ch 4 选工具
  → Ch 15 + 09 系统学
```

---


---

← [本章导读](../README.md)
