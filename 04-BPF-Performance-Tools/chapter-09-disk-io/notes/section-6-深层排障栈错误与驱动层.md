# 6. 深层排障：栈、错误与驱动层

### `biostacks` — 发起 I/O 的调用栈 🔴

块 I/O 延迟 + **发起该 I/O 时的内核/用户栈** — 排障「谁在写盘」神器。

```bash
sudo biostacks-bpfcc 10
sudo biostacks-bpfcc -p $(pidof myapp)
```

| 典型栈顶 | 含义 |
|----------|------|
| `journal_submit_commit_record` | 文件系统日志 |
| `swap_writepage` | **Swap 换出** — HFT 配置事故 |
| `zfs_*` / `btrfs_*` | 特定 FS 后台 |
| 应用 `write` | 业务同步写盘 |

**与 Ch 8：** `fileslower` 见逻辑慢 → `biostacks` 见 **块层 + 栈**。

### `bioerr`

块设备 **I/O 错误** 时打印详情（错误码、设备）— 静默坏盘/链路问题。

```bash
sudo bioerr-bpfcc
```

### `iosched`

测量请求在 **I/O 调度器** 队列中的等待延迟（blk-mq 时代仍有用）。

```bash
sudo iosched-bpfcc
```

**解读：** `biolatency` 总延迟高 + `iosched` 排队高 → **软件队列** 瓶颈。

### `scsilatency` / `scsiresult`

| 工具 | 作用 |
|------|------|
| `scsilatency` | SCSI 命令延迟分布 |
| `scsiresult` | SCSI 返回状态（`DID_OK`、`DID_BAD_TARGET`…） |

**场景：** SAN、HBA、多路径存储。

### `nvmelatency`

**NVMe 驱动层** 命令延迟（如 `nvme_cmd_read`）— 分离 **纯设备延迟** vs OS 上层开销。

```bash
sudo nvmelatency-bpfcc
```

**HFT 共置机：** 系统盘多为 NVMe — 区分「盘本身慢」还是「上层 flush 堆叠」。

---
