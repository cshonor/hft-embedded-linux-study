# 8. 内存回收与交换

### `vmscan`

追踪 VM 扫描器 **kswapd** 等在 shrink / reclaim 上花费的时间（tracepoint）。

```bash
sudo vmscan-bpfcc 10
```

**解读：** 内存压力持续时 kswapd 是否 **长时间运行**。

### `drsnoop` — 直接回收 🔴

追踪 **direct reclaim** 事件及 **延迟** — 量化「内存不足时分配路径被拖慢多久」。

```bash
sudo drsnoop-bpfcc
```

| 现象 | 含义 |
|------|------|
| 频繁 `drsnoop` 输出 | 系统在 **同步回收** — P99 抖动常见元凶 |
| 与行情峰值同相 | 共置机内存争用或 cache 过大 |

**HFT：** 延迟尖刺但 CPU/网都正常 → 查 **`drsnoop` + `vmstat`**。

### `swapin`

显示 **从 swap 换入** 的进程 — 谁受了 swap 影响。

```bash
sudo swapin-bpfcc
```

**HFT：** 生产若见非零输出 → **配置事故**（swap 应关）。

---
