# 6.3 生产环境内存监控

> ⬜ 跳读

## 本节要点

### 生产环境可用工具

| 工具 | 开销 | 检测内容 |
|------|------|---------|
| KFENCE | ~1% | 越界/UAF (采样) |
| /proc/meminfo | 0 | 内存使用统计 |
| /proc/slabinfo | 0 | slab 使用统计 |
| vmstat | 0 | 系统内存压力 |
| perf stat -e 'kmem:*' | 极低 | 内存分配事件计数 |

### 监控脚本示例

```bash
# 1. slab 使用监控
watch -n 5 'cat /proc/slabinfo | head -20'

# 2. 内存增长趋势
while true; do
    echo "$(date) $(grep MemAvailable /proc/meminfo)"
    sleep 60
done >> /var/log/mem_monitor.log

# 3. KFENCE 告警监控
while true; do
    new_bugs=$(cat /sys/kernel/debug/kfence/stats | grep "bugs detected" | awk '{print $3}')
    if [ "$new_bugs" -gt 0 ]; then
        echo "ALERT: KFENCE detected $new_bugs bugs"
        dmesg | grep -A 20 "KFENCE" | tail -20
    fi
    sleep 300
done

# 4. 定期 kmemleak 扫描 (非交易时段)
# crontab: 每天凌晨 3 点扫描
0 3 * * * echo scan > /sys/kernel/debug/kmemleak && sleep 10 && cat /sys/kernel/debug/kmemleak >> /var/log/kmemleak_$(date +\%Y\%m\%d).log
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 生产环境中如何在不影响实时性的前提下监控内存泄漏？

> 使用 KFENCE 做运行时采样检测（开销 ~1%），配合非交易时段的 kmemleak 定期扫描。通过 /proc/meminfo 和 /proc/slabinfo 趋势监控内存增长。不使用 KASAN 和 SLUB debug（开销过大）。

</details>
