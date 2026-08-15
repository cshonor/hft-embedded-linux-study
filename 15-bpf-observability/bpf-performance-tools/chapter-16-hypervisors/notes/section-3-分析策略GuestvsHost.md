# 3. 分析策略：Guest vs Host

### 访客机 (Guest) 策略

| 检查项 | 工具/指标 |
|--------|-----------|
| **Hypercall 过多**（Xen PV） | `xen:xen_mc_*` tracepoint、`xenhyper` |
| **CPU 被偷取** | **`cpustolen`**、`%st`（top/mpstat） |
| 虚拟资源性能 | Ch 6–10 全套 — 结果 **含宿主机争抢** |

**解读 stolen time：** Guest 认为有 vCPU，但 **物理 CPU 被分给别的 Guest/Host** — P99 抖动 **与业务无关** 的常见根因。

### 宿主机 (Host) 策略

| 检查项 | 工具/指标 |
|--------|-----------|
| **VM exit** 频率与原因 | **`kvmexits`**、`perf kvm:*` |
| I/O 代理负载 | **QEMU** 进程 CPU、`biotop` |
| 物理资源 | Host 上 Ch 6–10 + **noisy VM** |

**原则：** Guest 内看到慢 → 先在 Guest **`cpustolen`**；仍可疑 → Host **`kvmexits`**。

---
