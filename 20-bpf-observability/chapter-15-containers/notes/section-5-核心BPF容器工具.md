# 5. 核心 BPF 容器工具

### `runqlat --pidnss`

[Ch 6 `runqlat`](../../chapter-06-cpus/) + **按 PID namespace 分桶**。

```bash
sudo runqlat-bpfcc --pidnss 10
```

| 解读 | 含义 |
|------|------|
| 某 pidns 右尾长 | 该 **容器** CPU 调度延迟高 — **quota/shares** 或争抢 |
| 裸金属对比 | 无 `--pidnss` 全局 vs 分容器 |

**HFT：** 共宿 K8s 上某 pod P99 升 → 是否 **CPU throttle**（配合 `kubectl describe` limits）。

### `pidnss`

统计调度器在 **不同 PID namespace 间切换** 的次数。

```bash
sudo pidnss-bpfcc 5
```

**回答：** 多容器是否在 **同一 CPU 上激烈交错** — noisy neighbor **直接证据**。

### `blkthrot`

统计 **cgroup 块 I/O 节流 (throttle)** 次数。

```bash
sudo blkthrot-bpfcc
```

| 解读 | 含义 |
|------|------|
| 计数增长 | 容器触碰 **blkio 速率上限** — 盘未满也慢 |

→ 衔接 [Ch 9 `biolatency`](../../chapter-09-disk-io/) — 块层慢但 **iostat 不忙**。

### `overlayfs`

追踪 **OverlayFS** 读写 **延迟** — 容器镜像层/可写层。

```bash
sudo overlayfs-bpfcc 10
```

**场景：** 容器内 **大量小文件读**、日志写 overlay — 比 host ext4 多一层开销。

---
