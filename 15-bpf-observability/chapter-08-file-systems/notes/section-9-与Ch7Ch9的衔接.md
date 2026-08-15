# 9. 与 Ch 7 / Ch 9 的衔接

| 层 | 章 | 关键工具 |
|----|-----|----------|
| 内存回收挤占 cache | Ch 7 | `drsnoop`、`vmscan` |
| **文件系统 / 页缓存** | **Ch 8** | `cachestat`、`fileslower` |
| 块设备 / 磁盘 | Ch 9 | `biolatency`、`biosnoop` |

**下钻顺序：** `fileslower`（逻辑慢）→ `cachestat`（是否 cache miss）→ `ext4dist`（FS 层）→ `biolatency`（盘）。

---
