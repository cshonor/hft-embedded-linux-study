# Ch12 vDSO 与现代调试

> 来源: LWN.net
> 对标旧书: LKD3 Ch18 (printk/kgdb 为主, 已过时)

vDSO 系统调用加速、ftrace 现代增强、eBPF 观测、crash/drgn 事后分析。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 12.1 vDSO 系统调用加速 (LWN) | `notes/01-vdso.md` |
| 12.2 ftrace 现代增强 | `notes/02-ftrace-modern.md` |
| 12.3 eBPF 可编程追踪 | `notes/03-ebpf-observability.md` |
| 12.4 crash 与 drgn 事后分析 | `notes/04-crash-drgn-analysis.md` |

---

## HFT 关联

ftrace wakeup_rt + irqsoff 是交易系统延迟排查的两大法宝。eBPF/BCC 工具集用于生产环境在线观测。crash/drgn 用于内核 panic 后的 vmcore 分析。
