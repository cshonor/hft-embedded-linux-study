# 4. VFS 与应用 I/O 分析

### `opensnoop`

追踪 **`open()` / `openat()`** — 系统范围。

```bash
sudo opensnoop-bpfcc
sudo opensnoop-bpfcc -p $(pidof myapp)
```

| 场景 | 价值 |
|------|------|
| 找不到配置文件 | 看失败路径 |
| 日志路径、意外文件访问 | 审计热路径是否碰盘 |

→ 亦见 [Ch 4 BCC 单用途工具](../../chapter-04-bcc/)

### `vfsstat` / `vfscount` / `vfssize`

| 工具 | 输出 |
|------|------|
| `vfsstat` | VFS 操作 **速率**（读/写/打开/创建…） |
| `vfscount` | 按操作类型 **计数** |
| `vfssize` | 读写 **大小分布直方图** |

```bash
sudo vfsstat-bpfcc 1
sudo vfssize-bpfcc 10
```

**用途：** 极高层的 **工作负载表征** — 读多还是写多、请求块大小。

### `fsrwstat`

类似 `vfsstat`，但按 **文件系统类型** 分类：`ext4`、`xfs`、`sockfs`、`sysfs`…

```bash
sudo fsrwstat-bpfcc 5
```

**场景：** 区分 **真实磁盘 FS** vs **伪文件系统** 流量。

### `filetop`

像 `top` 一样列出 **当前读写最频繁 / 吞吐量最高** 的文件。

```bash
sudo filetop-bpfcc
```

**HFT incident：** 谁在打满磁盘日志或意外写大文件。

### `fileslower`

打印慢于阈值的 **同步** 文件读写（默认如 **10ms**）。

```bash
sudo fileslower-bpfcc 10
sudo fileslower-bpfcc -p $(pidof myapp) 5
```

**直接定位** 拖慢应用的 **逻辑 I/O** — 在 blame 磁盘前先跑此工具。

---
