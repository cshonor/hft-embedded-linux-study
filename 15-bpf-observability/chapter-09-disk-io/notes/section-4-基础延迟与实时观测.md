# 4. 基础延迟与实时观测

### `biolatency` — 块 I/O 延迟直方图 🔴

在内核按延迟桶 **聚合** 块 I/O — [Ch 3](../../chapter-03-performance-analysis/) 清单核心工具。

```bash
sudo biolatency-bpfcc -D 10      # 每 10s 打印直方图
sudo biolatency-bpfcc -F 10      # 按 I/O 标志分类（同步写、预读等）
```

| 直方图形态 | 解读 |
|------------|------|
| **双峰** | 部分 hit 设备/控制器 cache，部分走介质 |
| **右尾极长** | 长尾 outlier — HFT 共置机 P99 抖动线索 |
| 整体右移 | 盘饱和、调度拥塞、或网络存储故障 |

**HFT：** 热路径不应常触发；incident 时与 **`cachestat`**（Ch 8）对照 — miss 多 → 块层必忙。

### `biosnoop`

**逐行** 打印每次块 I/O：PID、大小、**QUE(ms)**、**LAT(ms)** 等 — 像「磁盘 tcpdump」。

```bash
sudo biosnoop-bpfcc
sudo biosnoop-bpfcc -d sda
```

| 对比 | 用途 |
|------|------|
| `QUE` 大、`LAT` 更大 | 排队 + 设备都慢 |
| `QUE` 大、`LAT`≈`QUE` | **OS 队列拥塞** 为主 |
| 低频精查 | 适合；高频勿长期开 |

### `biotop`

定期刷新 **块 I/O 流量最大** 的进程 — 块层版 `top` / `filetop`。

```bash
sudo biotop-bpfcc
```

**场景：** 谁在打满磁盘 — 日志进程、swap、未知后台任务。

---
