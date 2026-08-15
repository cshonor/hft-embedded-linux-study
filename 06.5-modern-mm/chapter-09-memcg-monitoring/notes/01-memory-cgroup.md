# Bootlin: 内存 cgroup

> **来源:** [Bootlin Kernel Training — cgroup v2](https://bootlin.com/docs/kernel/)
> **主题:** 内存 cgroup v2
> **对标旧书:** 无 (ULK3/LKD3 未涉及 cgroup)

---

## 讲义要点

### cgroup v2 内存控制

```bash
# 创建 cgroup
mkdir /sys/fs/cgroup/myapp

# 设置内存限制
echo 1073741824 > /sys/fs/cgroup/myapp/memory.max  # 1GB 硬限制
echo 536870912 > /sys/fs/cgroup/myapp/memory.high   # 512MB 软限制 (超过则回收)

# 将进程加入 cgroup
echo <pid> > /sys/fs/cgroup/myapp/cgroup.procs

# 查看内存使用
cat /sys/fs/cgroup/myapp/memory.current   # 当前用量
cat /sys/fs/cgroup/myapp/memory.peak      # 历史峰值
cat /sys/fs/cgroup/myapp/memory.events    # 事件计数
```

### memory.max vs memory.high

| 控制 | 行为 |
|------|------|
| `memory.max` | 硬限制，超过则 OOM killer 或拒绝分配 |
| `memory.high` | 软限制，超过则积极回收该 cgroup 的页 |
| `memory.min` | 最低保障，保护这些内存不被回收 |
| `memory.low` | 最佳努力保护，低于此值时尽量不回收 |

### memory.events

```bash
cat /sys/fs/cgroup/myapp/memory.events
# low 0          — 低于 memory.low 被回收的次数
# high 123       — 超过 memory.high 的次数
# max 5          — 达到 memory.max 的次数
# oom 1          — OOM 触发次数
# oom_kill 1     — OOM 杀死进程的次数
```

### cgroup v2 LRU

```c
// 源码路径: mm/memcontrol.c
// 每个 cgroup 有自己的 LRU 链表 (或 MGLRU 代)

struct mem_cgroup {
    struct lruvec lruvec;  // per-node per-cgroup LRU
    unsigned long memory_current;
    unsigned long memory_high;
    unsigned long memory_max;
    // ...
};

// 页回收时优先回收超限 cgroup 的页
```

---

## 动手实验

```bash
# 1. 创建受限 cgroup
mkdir /sys/fs/cgroup/test_mem
echo 268435456 > /sys/fs/cgroup/test_mem/memory.max  # 256MB
echo $$ > /sys/fs/cgroup/test_mem/cgroup.procs

# 2. 分配内存触发 OOM
python3 -c "x = ['x' * 1024 * 1024] * 512"  # 尝试分配 512MB

# 3. 查看 cgroup 内存统计
cat /sys/fs/cgroup/test_mem/memory.current
cat /sys/fs/cgroup/test_mem/memory.stat
# cache 12345678    — 页缓存
# rss 23456789      — RSS
# slab 3456789      — slab
# pgfault 12345     — page fault 次数

# 4. 清理
rmdir /sys/fs/cgroup/test_mem
```

---

## 与旧书差异

| ULK3 / LKD3 | Bootlin 讲义 |
|-------------|-------------|
| 无 cgroup | cgroup v2 内存控制 |
| 全局 LRU | per-cgroup LRU |
| 全局 OOM | per-cgroup OOM 隔离 |
| 无 memory.high | 软限制回收 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** memory.high 和 memory.max 的区别？哪个对 HFT 更有用？

> memory.max 是硬限制，超过触发 OOM killer。memory.high 是软限制，超过后内核积极回收该 cgroup 的页但不杀进程。对 HFT 更有用的是 memory.max——交易进程设明确的内存上限，超过则 OOM（说明有 bug/泄漏），不应用 memory.high（软回收可能引入延迟毛刺）。

**Q2:** cgroup v2 的 per-cgroup LRU 如何提高回收效率？

> 传统全局 LRU 回收时扫描所有页，可能误回收其他 cgroup 的工作集页。per-cgroup LRU 让回收算法只扫描超限 cgroup 的页，精准回收。这减少了不必要的页扫描，也避免了 cgroup 间干扰。

</details>
