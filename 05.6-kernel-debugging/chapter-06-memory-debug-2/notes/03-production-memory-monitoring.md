# 6.3 生产环境内存监控

> ⬜ 跳读 · Part 2: Instrumentation & Memory Debugging

## 本节要点

生产环境需要在低开销下持续监控内存状态，及时发现泄漏和异常。

## 生产环境可用工具

| 工具 | 开销 | 检测内容 | 实时性 | HFT 可用 |
|------|------|---------|--------|---------|
| KFENCE | ~1% | 越界/UAF (采样) | 即时 | ✅ |
| /proc/meminfo | 0 | 内存使用统计 | 即时 | ✅ |
| /proc/slabinfo | 0 | slab 使用统计 | 即时 | ✅ |
| /sys/kernel/slab/* | 0 | 详细 slab 信息 | 即时 | ✅ |
| vmstat | 0 | 系统内存压力 | 即时 | ✅ |
| perf stat -e 'kmem:*' | 极低 | 内存分配事件计数 | 采样 | ✅ |
| kmemleak | 扫描时高 | 泄漏检测 | 非实时 | ⚠️ 离线 |

## 监控脚本

### 1. 内存使用趋势监控

```bash
#!/bin/bash
# mem_trend.sh: 监控内存增长趋势

LOG=/var/log/mem_monitor.log

while true; do
    timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    mem_avail=$(grep MemAvailable /proc/meminfo | awk '{print $2}')
    slab=$(grep Slab /proc/meminfo | awk '{print $2}')
    slabs=$(grep SReclaimable /proc/meminfo | awk '{print $2}')

    echo "$timestamp MemAvailable=${mem_avail}kB Slab=${slab}kB SReclaimable=${slabs}kB" >> $LOG

    sleep 60
done

# 分析趋势
# awk '{print $1, $2}' /var/log/mem_monitor.log | tail -100
# 如果 MemAvailable 持续下降 → 可能泄漏
```

### 2. Slab 使用监控

```bash
#!/bin/bash
# slab_monitor.sh: 监控 slab 分配器使用情况

echo "=== Top 10 slab caches by active objects ==="
cat /proc/slabinfo | tail -n +2 | sort -k3 -rn | head -10

echo "=== Slab cache growth (kmalloc-128) ==="
prev=0
while true; do
    curr=$(grep kmalloc-128 /proc/slabinfo | awk '{print $3}')
    diff=$((curr - prev))
    if [ $prev -ne 0 ] && [ $diff -gt 0 ]; then
        echo "$(date): kmalloc-128 active objects: $curr (+$diff)"
    fi
    prev=$curr
    sleep 60
done
```

### 3. KFENCE 告警监控

```bash
#!/bin/bash
# kfence_monitor.sh: 监控 KFENCE 告警

while true; do
    bugs=$(cat /sys/kernel/debug/kfence/stats 2>/dev/null | \
           grep "bugs detected" | awk '{print $3}')

    if [ "$bugs" -gt 0 ]; then
        echo "$(date) ALERT: KFENCE detected $bugs bugs"
        dmesg | grep -A 20 "KFENCE" | tail -30
        # 发送告警
        # send_alert "KFENCE: $bugs bugs detected"
    fi
    sleep 300
done
```

### 4. 定期 kmemleak 扫描

```bash
#!/bin/bash
# kmemleak_scan.sh: 非交易时段扫描内存泄漏

# crontab: 每天凌晨 3 点扫描
# 0 3 * * * /opt/hft/scripts/kmemleak_scan.sh

echo "scan" > /sys/kernel/debug/kmemleak
sleep 10  # 等待扫描完成

REPORT=$(cat /sys/kernel/debug/kmemleak)
if [ -n "$REPORT" ]; then
    echo "$REPORT" >> /var/log/kmemleak_$(date +%Y%m%d).log
    # 发送告警
    # send_alert "kmemleak: potential memory leak detected"
    echo "$(date) ALERT: kmemleak found potential leaks"
else
    echo "$(date) OK: no memory leaks detected"
fi
```

### 5. 综合监控 Dashboard

```bash
#!/bin/bash
# mem_dashboard.sh: 综合内存状态报告

echo "=========================================="
echo "HFT Memory Status: $(date)"
echo "=========================================="

echo "--- Memory Overview ---"
grep -E "MemTotal|MemFree|MemAvailable|Slab|SUnreclaim|SReclaimable" /proc/meminfo

echo "--- Top 5 Slab Caches ---"
cat /proc/slabinfo | tail -n +2 | sort -k3 -rn | head -5

echo "--- KFENCE Status ---"
cat /sys/kernel/debug/kfence/stats 2>/dev/null

echo "--- Kmemleak (last scan) ---"
cat /sys/kernel/debug/kmemleak 2>/dev/null | head -5

echo "--- Page Allocation Failures ---"
grep -c "page allocation failure" /var/log/kern.log 2>/dev/null || echo "0"

echo "--- OOM Kill Count ---"
dmesg | grep -c "Out of memory" 2>/dev/null || echo "0"
```

## 阈值告警

| 指标 | 正常 | 警告 | 严重 |
|------|------|------|------|
| MemAvailable | >50% | 20-50% | <20% |
| Slab 增长率 | <1%/小时 | 1-5%/小时 | >5%/小时 |
| KFENCE bugs | 0 | 1-5 | >5 |
| kmemleak 报告 | 0 | 1-3 | >3 |
| page alloc failure | 0 | 1-10 | >10 |
| OOM kill | 0 | 0 | >0 |

## HFT 关联

HFT 生产环境内存监控策略：

1. **实时监控**：/proc/meminfo + /proc/slabinfo 趋势（0 开销）
2. **采样检测**：KFENCE 低频运行（~1% 开销）
3. **离线扫描**：非交易时段 kmemleak 扫描
4. **告警机制**：内存增长超阈值时告警
5. **应急方案**：内存不足时 kdump + 重启

```bash
# HFT 生产环境推荐监控配置
# 1. 实时: mem_trend.sh 持续运行
# 2. 采样: kfence.sample_interval=1000 (1s)
# 3. 离线: crontab 每天 3:00 kmemleak 扫描
# 4. 告警: 阈值检查脚本每 5 分钟运行
# 5. 应急: panic_on_oops=1 + kdump 配置
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 生产环境中如何在不影响实时性的前提下监控内存泄漏？

> 使用 KFENCE 做运行时采样检测（开销 ~1%），配合非交易时段的 kmemleak 定期扫描。通过 /proc/meminfo 和 /proc/slabinfo 趋势监控内存增长。不使用 KASAN 和 SLUB debug（开销过大）。

**Q2:** HFT 生产环境应该启用哪些内存调试工具？

> 仅 KFENCE（开销 <1% CPU）。KASAN 太重（2-3x slowdown），LOCKDEP 也重（~200ns/lock op）。KFENCE 的采样模式（1/100）在生产中几乎无感知，但仍能捕获偶发的越界/UAF。如果发现问题，切换到 staging 环境启用 KASAN 深入排查。

**Q3:** 如何通过 /proc/slabinfo 判断内存泄漏？

> 观察 slab cache 的活跃对象数（第三列）是否持续增长。如果某个 cache 的活跃对象数随时间单调增加且不回收，可能是泄漏。对比不同时间点的 slabinfo 快照可以快速定位泄漏的 cache。

**Q4:** kmemleak 扫描在 HFT 环境中应该怎么安排？

> 在非交易时段（如收盘后凌晨 3 点）通过 crontab 手动触发扫描。扫描期间可能暂停内存分配（RCU 停顿），影响延迟。不应在交易时段自动扫描。默认的每 10 分钟自动扫描应关闭（`kmemleak=off` 启动，手动 `echo scan > ...` 触发）。

**Q5:** KFENCE 的 sample_interval 在 HFT 生产环境应该设为多少？

> 建议 1000ms（1秒）。默认 100ms 开销约 1%，设为 1000ms 开销降到 ~0.1%。HFT 对延迟敏感，应尽量降低开销。如果发现问题或需要更密集的检测，临时调小到 100ms。开发环境可以设为 10ms 提高检测率。

</details>

## 交叉引用

- [05.6 ch06 KFENCE](../../chapter-06-memory-debug-2/notes/01-kfence.md)
- [05.6 ch06 内存调试策略](../../chapter-06-memory-debug-2/notes/02-memory-debug-strategy.md)
- [05.6 ch05 kmemleak](../../chapter-05-memory-debug-1/notes/05-kmemleak.md)
