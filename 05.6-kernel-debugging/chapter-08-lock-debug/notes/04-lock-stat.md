# lock_stat：锁竞争统计

> 🔴 精读

## 概念详解

### lock_stat 功能

LOCKDEP 除了检测死锁，还能统计锁的**竞争情况**：等待时间、持有时间、争用次数。这些数据帮助开发者识别锁瓶颈。

### 启用 lock_stat

```bash
# 内核配置
CONFIG_LOCK_STAT=y

# 运行时控制
echo 1 > /proc/sys/kernel/lock_stat   # 启用
echo 0 > /proc/sys/kernel/lock_stat   # 禁用/重置
```

### 查看锁统计

```bash
cat /proc/lock_stat
#                               class name    con-bounces  contentions  waittime-min  waittime-max  waittime-total  acq-bounces  acquisitions  holdtime-min  holdtime-max  holdtime-total
#                               &my_lock_b             15           12          0.12         15.34          123.45           45          234           0.10          5.67           89.12
```

### 关键指标详解

| 指标 | 含义 | 优化目标 |
|------|------|---------|
| `contentions` | 争用次数（获取时被阻塞） | 越低越好 |
| `waittime-min` | 最小等待时间 | 参考值 |
| `waittime-max` | 最大单次等待时间 | 越低越好 |
| `waittime-total` | 总等待时间 | 越低越好 |
| `holdtime-min` | 最小持有时间 | 参考值 |
| `holdtime-max` | 最大持有时间 | 越低越好 |
| `holdtime-total` | 总持有时间 | 越低越好 |
| `acquisitions` | 总获取次数 | 参考值 |
| `con-bounces` | 争用跳转次数（CPU间迁移） | 越低越好 |

### wait time vs hold time

```
wait time: 线程试图获取锁到实际获取锁的时间
  → 高 wait time = 锁竞争激烈

hold time: 线程获取锁到释放锁的时间
  → 高 hold time = 临界区太大

优化策略:
  wait time 高 → 减少锁获取频率或改用细粒度锁
  hold time 高 → 缩小临界区
```

### 识别问题锁

```bash
# 找出争用最严重的锁
cat /proc/lock_stat | sort -k4 -rn | head -10

# 找出等待时间最长的锁
cat /proc/lock_stat | sort -k6 -rn | head -10

# 找出单次等待最长的锁
cat /proc/lock_stat | sort -k5 -rn | head -10
```

### 优化策略

| 问题 | 症状 | 优化方案 |
|------|------|---------|
| 高争用 | contentions 高 | 缩短临界区 / per-CPU 数据 / RCU |
| 高等待 | waittime-max 高 | 减少持锁频率 / 改用读写锁 |
| 高持有 | holdtime-max 高 | 缩小临界区 / 延迟非关键操作 |
| 高跳转 | con-bounces 高 | per-CPU 锁 / 减少跨 CPU 共享 |

```c
// 优化前: 全局 spinlock，高争用
static DEFINE_SPINLOCK(counter_lock);
void inc_counter(int cpu) {
    spin_lock(&counter_lock);      // 所有 CPU 争用同一锁
    counters[cpu]++;
    spin_unlock(&counter_lock);
}

// 优化 1: per-CPU 数据（无锁）
static DEFINE_PER_CPU(int, counter);
void inc_counter(void) {
    this_cpu_inc(counter);  // 无锁，原子操作
}

// 优化 2: 读写锁（读多写少）
static DEFINE_RWLOCK(counter_rwlock);
void read_counters(void) {
    read_lock(&counter_rwlock);   // 多个读者可并发
    read_unlock(&counter_rwlock);
}

// 优化 3: RCU（读多写极少）
static struct counters __rcu *counters_ptr;
void read_counters(void) {
    rcu_read_lock();
    ptr = rcu_dereference(counters_ptr);  // 无锁读
    rcu_read_unlock();
}
```

### HFT 关联应用

```bash
# HFT staging 环境收集锁统计
echo 1 > /proc/sys/kernel/lock_stat
./run_trading_benchmark --duration 60
cat /proc/lock_stat > /tmp/lock_stat_after_benchmark.txt

# 找出 waittime-max > 1.0 (1微秒) 的锁
awk 'NR>2 && $5 > 1.0 {print $1, "waittime-max:", $5}' /tmp/lock_stat_after_benchmark.txt
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 如何从 lock_stat 输出中判断一个锁是否需要优化？

> 看 contentions 和 waittime-max。如果 contentions 高且 waittime-max 超过可接受阈值（如 HFT 中 > 1μs），需要优化：缩短临界区、改用 RCU、改用 per-CPU 数据。

**Q2:** lock_stat 的开销有多大？能用于生产环境吗？

> 需要 LOCKDEP 支持，每次加锁/解锁额外 ~100-200ns 开销，整体 slowdown 约 3-8x。不适合生产环境。应在 staging 环境用压力测试模拟生产负载。

**Q3:** "hold time" 和 "wait time" 分别反映什么问题？

> hold time 长 → 锁内临界区太大，应缩小。wait time 长 → 锁竞争激烈，应减少持锁频率或改用更细粒度的锁。HFT 关注 wait time——高 wait time 意味着交易线程可能在等锁。

**Q4:** `con-bounces` 指标反映了什么问题？

> 表示锁在不同 CPU 间迁移的次数。高 con-bounces 意味着锁在多个 CPU 间频繁弹跳，导致 cache line 失效。解决：用 per-CPU 数据结构避免跨 CPU 共享。

**Q5:** per-CPU 计数器为什么比全局原子操作更快？

> per-CPU 变量每个 CPU 有独立副本，写入只操作本地 CPU 的 cache line，不会导致跨 CPU 的 cache line 弹跳。全局原子操作每次写都使其他 CPU 的 cache line 失效。

</details>

## 交叉引用

- [05.6 ch08 LOCKDEP 锁依赖检测器](../../chapter-08-lock-debug/notes/02-lockdep.md)
- [05.6 ch08 并发 Bug 类型](../../chapter-08-lock-debug/notes/01-concurrency-bug-types.md)
- [05.6 ch08 树莓派启用 LOCKDEP/KCSAN](../../chapter-08-lock-debug/notes/06-rpi-lockdep-kcsan.md)
