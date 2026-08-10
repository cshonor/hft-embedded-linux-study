# 8.4 lock_stat：锁竞争统计

> 🔴 精读

## 本节要点

### lock_stat 功能

LOCKDEP 除了检测死锁，还能统计锁的**竞争情况**：等待时间、持有时间、争用次数。

### 启用 lock_stat

```bash
# 内核配置 (已包含在 LOCKDEP 中)
CONFIG_LOCK_STAT=y

# 运行时控制
echo 1 > /proc/sys/kernel/lock_stat  # 启用
echo 0 > /proc/sys/kernel/lock_stat  # 禁用
echo 0 > /proc/sys/kernel/lock_stat  # 重置统计
```

### 查看锁统计

```bash
cat /proc/lock_stat
# 输出示例:
#                               class name    con-bounces    contentions   waittime-min   waittime-max   waittime-total   acq-bounces   acquisitions   holdtime-min   holdtime-max   holdtime-total
#                               ----------    -----------    -----------   -----------    -----------    -------------    -----------    -----------    -----------    -----------    -------------
#                               &my_lock_b             15             12          0.12         15.34          123.45             45            234           0.10          5.67           89.12
#                               &my_lock_a              8              5          0.05          8.90           45.67             30            180           0.08          3.45           56.78
```

### 关键指标

| 指标 | 含义 | 优化目标 |
|------|------|---------|
| `contentions` | 争用次数（获取时被阻塞） | 越低越好 |
| `waittime-total` | 总等待时间 | 越低越好 |
| `waittime-max` | 最大单次等待时间 | 越低越好 |
| `holdtime-max` | 最大持有时间 | 越低越好 |
| `acquisitions` | 总获取次数 | 参考值 |

### 识别问题锁

```bash
# 找出争用最严重的锁
cat /proc/lock_stat | sort -k4 -rn | head -10
# 按 contentions 降序排列

# 找出等待时间最长的锁
cat /proc/lock_stat | sort -k6 -rn | head -10
# 按 waittime-total 降序排列
```

### HFT 关联

lock_stat 帮助识别 HFT 内核模块中的锁瓶颈——高争用的锁可能导致延迟毛刺。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 如何从 lock_stat 输出中判断一个锁是否需要优化？

> 看 contentions（争用次数）和 waittime-max（最大等待时间）。如果 contentions 高且 waittime-max 超过可接受阈值（如 HFT 中 > 1μs），需要优化：1) 缩短临界区；2) 改用 RCU 替代读写锁；3) 改用 per-CPU 数据避免共享。

**Q2:** lock_stat 的开销有多大？能用于生产环境吗？

> lock_stat 需要 LOCKDEP 支持，每次加锁/解锁额外 ~100-200ns 开销，整体 slowdown 约 2-5x。不适合生产环境。应在 staging 环境用压力测试模拟生产负载，收集锁统计后优化。

</details>
