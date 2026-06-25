# 7. 工具选型速查

| 场景 | 视角 | 工具 |
|------|------|------|
| 云 VM 延迟抖 | Guest | **`cpustolen`**、`%st` |
| Xen PV hypercall 多 | Guest | `xenhyper`、xen tracepoints |
| 宿主机 VM 争抢 | Host | **`kvmexits`**、perf `kvm:*` |
| QEMU 吃 CPU | Host | `profile` on qemu |
| Nitro/现代云 | Both | Ch 6–10 通用工具 |
| 与容器混部 | — | [Ch 15](../../chapter-15-containers/) + 本章 Host |

---
