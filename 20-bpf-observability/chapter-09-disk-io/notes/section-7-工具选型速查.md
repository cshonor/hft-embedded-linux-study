# 7. 工具选型速查

| 症状 | 优先工具 |
|------|----------|
| 磁盘慢但不知分布 | **`biolatency`** |
| 谁在读写的最多 | `biotop` |
| 单次 I/O 明细 | `biosnoop`（短窗口） |
| 疯狂写盘不知来源 | **`biostacks`** |
| 块大小不合理 | `bitesize` |
| random vs sequential | `biopattern` |
| 调度队列拥塞 | `iosched` + `biosnoop` QUE |
| NVMe 设备层 | `nvmelatency` |
| SCSI/SAN | `scsilatency`、`scsiresult` |
| I/O 硬件错误 | `bioerr` |
| 逻辑层先查 | [Ch 8 `cachestat`/`fileslower`](../../chapter-08-file-systems/) |

---
