## 8.6 观测工具

### 传统统计工具

| 工具 | 用途 |
|------|------|
| **`mount`** | 挂载选项：`noatime`、`data=writeback` 等 |
| **`free` / `top` / `vmstat`** | cache 占用；与 Ch 7 联动 |
| **`sar -v`** | dentry/inode cache 统计 |
| **`slabtop`** | dentry/inode 等 slab 占用 |

### BPF / BCC 工具集

| 工具 | 作用 | HFT 场景 |
|------|------|----------|
| **`opensnoop`** | 谁 open 了什么文件 | 找意外读配置、权限问题 |
| **`filetop`** | 按文件 I/O 吞吐排序 | 哪份日志/数据文件在狂读写的 |
| **`cachestat`** | page cache **命中率** | 区分 cache 命中 vs 真读盘 |
| **`ext4dist` / `xfsdist`** | FS 操作延迟直方图 | 看双峰（快 cache / 慢 disk） |
| **`ext4slower` / `xfsslower`** | 超过阈值（如 10ms）的慢操作 | 抓 fsync、journal 尖刺 |
| **bpftrace VFS 单行** | 追踪 `vfs_read` 等 | 附录 C 扩展 |

```bash
# 页缓存命中情况（需 BCC）
sudo cachestat-bpfcc 5

# 慢 ext4 操作 > 10ms
sudo ext4slower-bpfcc 10

# 谁在 open 文件
sudo opensnoop-bpfcc
```

→ [Ch 15 BPF](../../chapter-15-bpf/) · [附录 C](../../appendix-C-bpftrace单行命令.md) · [03-BPF](../../../03-BPF-Performance-Tools/)

---


---

← [本章导读](../README.md)
