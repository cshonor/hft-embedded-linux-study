# Bootlin: 页回收 (MGLRU / swap / zswap)

> **来源:** [Bootlin Kernel Training — Memory Management](https://bootlin.com/docs/kernel/)
> **主题:** 页回收机制、MGLRU、swap、zswap
> **对标旧书:** ULK3 Ch17 / LKD3 Ch18

---

## 讲义要点

### 回收路径

```
内存压力 (free < watermark)
  │
  ├── kswapd (后台异步回收)
  │     ├── 扫描 LRU (传统) 或 MGLRU (6.1+)
  │     ├── clean file page → 丢弃
  │     ├── dirty file page → writeback 后丢弃
  │     └── anon page → swap (或 zswap 压缩)
  │
  └── direct reclaim (同步回收, 分配路径)
        └── 同 kswapd 但同步执行, 阻塞调用者
```

### MGLRU (6.1+)

```bash
# 启用 MGLRU
echo y > /sys/kernel/mm/lru_gen/enabled

# MGLRU 将页按访问时间分为多代
# 最老代的页优先回收
# 减少 90% 页扫描量
```

### swap

```bash
# 配置 swap
swapon /dev/sda2          # swap 分区
swapon /swapfile          # swap 文件

# 查看 swap 状态
cat /proc/swaps
cat /proc/meminfo | grep Swap

# swappiness (0-200, 默认 60)
# 0 = 尽量不用 swap, 200 = 积极使用 swap
cat /proc/sys/vm/swappiness

# HFT: 禁用 swap
swapoff -a
echo 0 > /proc/sys/vm/swappiness
```

### zswap

```bash
# 启用 zswap
echo 1 > /sys/module/zswap/parameters/enabled
echo zstd > /sys/module/zswap/parameters/compressor
echo 20 > /sys/module/zswap/parameters/max_pool_percent
```

### OOM Killer

```bash
# 调整 OOM 优先级
echo -1000 > /proc/<pid>/oom_score_adj  # 永不被 OOM
echo 1000 > /proc/<pid>/oom_score_adj   # 优先被 OOM

# 查看当前 OOM 分数
cat /proc/<pid>/oom_score

# 禁用 OOM killer (危险!)
echo 1 > /proc/sys/vm/panic_on_oom
```

---

## 动手实验

```bash
# 1. 观察 kswapd 活动
cat /proc/vmstat | grep -E "kswapd|pgscan|pgsteal"

# 2. 触发回收 (慎用)
echo 3 > /proc/sys/vm/drop_caches

# 3. 内存压力测试
stress-ng --vm 4 --vm-bytes 1G --timeout 60s

# 4. 观察 OOM
dmesg | grep -i "oom\|killed"

# 5. 查看 MGLRU 状态
cat /sys/kernel/mm/lru_gen/debugfs
```

---

## 与旧书差异

| ULK3 | Bootlin 讲义 |
|------|-------------|
| 2 代 LRU | MGLRU 多代 (6.1+) |
| 无 zswap | zswap (3.11+) |
| `badness()` OOM | `oom_badness()` + oom_score_adj |
| per-zone LRU | per-node + per-memcg LRU |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** kswapd 和 direct reclaim 哪个对延迟敏感应用影响更大？如何避免 direct reclaim？

> direct reclaim 影响更大——它在分配路径同步执行，阻塞调用者微秒到毫秒级。kswapd 是后台线程，不影响分配延迟（除非 kswapd 来不及导致 direct reclaim 兜底）。避免 direct reclaim：(1) 确保空闲内存始终高于 watermark（预留足够内存）；(2) `mlockall` 锁定关键页；(3) 禁用 swap 减少 anon 页回收；(4) 调高 `vm.min_free_kbytes` 增加安全余量。

**Q2:** HFT 系统为什么应该 `swapoff -a`？

> (1) swap I/O 延迟毫秒级，对 HFT 不可接受；(2) swap 活动导致 kswapd 唤醒，消耗 CPU；(3) 即使不 swap，swappiness>0 时内核仍会尝试 swap，影响调度。HFT 应：`swapoff -a` + `echo 0 > /proc/sys/vm/swappiness` + `mlockall`。

</details>
