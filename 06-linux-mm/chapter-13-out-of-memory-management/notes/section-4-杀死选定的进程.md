# Ch 13 §4 杀死选定的进程

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

### 4. 杀死选定的进程

选中后：

| 步骤 | 说明 |
|------|------|
| **通知范围** | 该 **task** + **共享同一 `mm_struct` 的所有线程** |
| **调度优先** | 提高优先级 — **尽快退出释内存** |
| **内存标志** | **`PF_MEMALLOC` + `PF_MEMDIE`** — 退出路径 **仍可分配** 必要页（Ch 6） |
| **信号选择** | 有 **`CAP_SYS_RAWIO`** → **`SIGTERM`**（优雅）；否则 **`SIGKILL`** |

**日志：** dmesg 常见 **`Out of memory: Kill process <pid> (<comm>) score …`** — 运维 **定位谁被杀**。

---
