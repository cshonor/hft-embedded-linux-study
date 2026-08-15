# 7. 具体文件系统：ext4 / XFS

当问题沉到 **具体 FS 实现** 层：

### `ext4dist` / `xfsdist`

**延迟分布直方图** — ext4/XFS 内部常见操作（读、写、打开、fsync…）。

```bash
sudo ext4dist-bpfcc 10
sudo xfsdist-bpfcc 10
```

### `ext4slower` / `xfsslower`

打印该 FS 内慢于阈值的操作。

```bash
sudo ext4slower-bpfcc 10
sudo xfsslower-bpfcc 10
```

**拆解示例：**

```
cachestat  → 命中率高（读不是瓶颈）
ext4dist   → fsync 尾延迟极大
writeback  → 内存压力触发大量 flush
```

→ 下一步：应用是否 **过度 fsync**、挂载选项、`noatime`、或 [Ch 9 块层](../../chapter-09-disk-io/)。

---
