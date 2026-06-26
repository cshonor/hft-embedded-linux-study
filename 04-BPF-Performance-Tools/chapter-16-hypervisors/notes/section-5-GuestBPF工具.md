# 5. Guest BPF 工具

### Xen Hypercalls（半虚拟化 PV）

追踪 **`xen:xen_mc_*`** tracepoints — 批量/单次 **hypercall** 计数与延迟。

### `xenhyper`（bpftrace）

通过 **`xen_mc_entry`** 统计具体 hypercall 类型：

| 类型（示例） | 含义 |
|--------------|------|
| `mmu_update` | 页表更新 |
| `stack_switch` | 栈切换 |
| … | 见工具输出 |

```bash
# 工具名以发行版 man 为准
sudo xenhyper.bt   # 或 bpftrace 脚本 xenhyper
```

**适用：** **Xen PV Guest** — 现代云多为 **HVM/PVH**，hypercall 分析范围变窄。

### Xen Callbacks（Host → Guest）

Xen 向 Guest **回调**（如 **IRQ / evtchn upcall**）— 追踪 **`xen_evtchn_do_upcall()`** 等，定位 **哪类进程被注入中断**。

### `cpustolen` — Stolen CPU 时间 🔴

Guest 内 **CPU 被偷取时间** 的 **延迟直方图**。

```bash
sudo cpustolen-bpfcc 10
# 或 bpftrace 版 cpustolen(8)
```

| 直方图 | 含义 |
|--------|------|
| 右尾长 | **宿主机过度订阅 (oversubscription)** — 与其他 VM 抢物理 CPU |
| 接近 0 | Guest vCPU **基本独占** 物理核（或 steal 未计量场景） |

**HFT：** 云 VM 上 P99 无法压下去 → **先看 stolen**；高 stolen → **换 dedicated host / 裸金属**，而非调策略代码。

### HVM Guest 注意

**全虚拟化 (HVM)** hypercall 路径少 — 延迟推断靠 **Ch 6–13 常规工具** + **`cpustolen`/`%st`**。

---
