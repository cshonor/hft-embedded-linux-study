# 5. 缓存效能分析

### `cachestat` — 页缓存命中率 🔴

显示 **页缓存 HIT / MISS** 及 **命中率 (HITRATIO)**。

```bash
sudo cachestat-bpfcc 1
```

| 解读 | 含义 |
|------|------|
| 命中率高 | 工作集在内存内 — 物理 I/O 少 |
| 命中率骤降 | 工作集 > RAM 或冷启动 — 衔接 [Ch 7](../../chapter-07-memory/) / [Ch 9](../../chapter-09-disk-io/) |
| DB / 时序库调优 | 数据集是否 **fit in cache** |

**经典组合：** `cachestat` 命中高 + `ext4dist` 写延迟大 → 问题可能在 **写回/fsync**，而非读缓存。

### `writeback`

追踪 **脏页写回** 磁盘：刷新页数、耗时、触发原因（周期、内存压力…）。

```bash
sudo writeback-bpfcc
```

**场景：** 延迟尖刺与 **后台 flush** 同相。

### `readahead`

追踪 **预读** 行为 — 含 **预读但从未访问** 的页比例。

```bash
sudo readahead-bpfcc
```

**调优：** 顺序大文件读可受益；随机小读可能 **浪费**。

### `dcstat` / `dcsnoop`

| 工具 | 作用 |
|------|------|
| `dcstat` | **dcache** 命中率统计 |
| `dcsnoop` | 目录查找详情（路径解析） |

```bash
sudo dcstat-bpfcc 1
```

### `icstat`

按秒统计 **inode cache** 引用、未命中、命中率。

```bash
sudo icstat-bpfcc 1
```

---
