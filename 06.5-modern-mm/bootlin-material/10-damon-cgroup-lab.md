# Bootlin: DAMON 与 cgroup 实验

> **来源:** [Bootlin Kernel Training — Advanced Topics](https://bootlin.com/docs/kernel/)
> **主题:** DAMON 数据访问监控实验 + cgroup v2 内存限制实验
> **对标旧书:** 无

---

## 讲义要点

### DAMON 实验：监控进程内存访问模式

```bash
# 前提: 内核 CONFIG_DAMON=y (5.15+)

# 1. 设置监控目标
echo <pid> > /sys/kernel/debug/damon/target_ids

# 2. 设置监控参数
# 采样间隔 5ms, 聚合间隔 100ms, 更新间隔 1000ms
# 最小区域 10, 最大区域 1000
echo "5 100 1000 10 1000" > /sys/kernel/debug/damon/attrs

# 3. 启动监控
echo on > /sys/kernel/debug/damon/monitor_on

# 4. 读取结果
# DAMON 将结果写入 internal buffer, 通过 dbgfs 暴露
cat /sys/kernel/debug/damon/target_ids

# 5. 使用 DAMOS (自动操作)
# 对 100ms 未访问的页执行 pageout
echo "10 1 0 0 3" > /sys/kernel/debug/damon/schemes
# format: min_sz max_sz min_nr_accesses max_nr_accesses action
# action: 0=willneed, 1=cold, 2=pageout, 3=stat
```

### cgroup v2 内存限制实验

```bash
# 实验: 限制进程内存并观察行为

# 1. 创建 cgroup
mkdir /sys/fs/cgroup/memtest

# 2. 设置 256MB 硬限制
echo 268435456 > /sys/fs/cgroup/memtest/memory.max

# 3. 设置 200MB 软限制 (超过则积极回收)
echo 209715200 > /sys/fs/cgroup/memtest/memory.high

# 4. 将当前 shell 加入 cgroup
echo $$ > /sys/fs/cgroup/memtest/cgroup.procs

# 5. 运行内存测试
python3 << 'EOF'
import gc, time
data = []
for i in range(100):
    data.append(bytearray(10 * 1024 * 1024))  # 10MB 每次
    print(f"allocated {(i+1)*10}MB, cgroup current: ", end="")
    with open("/sys/fs/cgroup/memtest/memory.current") as f:
        print(f.read().strip())
    time.sleep(0.1)
EOF

# 6. 查看事件
cat /sys/fs/cgroup/memtest/memory.events
# high 5       — 超过 memory.high 5 次
# max 1        — 达到 memory.max 1 次
# oom 1        — OOM 1 次
# oom_kill 1   — 杀死 1 个进程

# 7. 清理
rmdir /sys/fs/cgroup/memtest
```

### PSI + cgroup 联合实验

```bash
# 实验: 在 cgroup 内制造内存压力, 观察 PSI

# 1. 创建低内存 cgroup
mkdir /sys/fs/cgroup/pressure
echo 134217728 > /sys/fs/cgroup/pressure/memory.max  # 128MB
echo $$ > /sys/fs/cgroup/pressure/cgroup.procs

# 2. 设置 PSI 触发器
echo "some 30000 1000000" > /proc/pressure/memory  # 30% some, 10s

# 3. 制造压力
stress-ng --vm 2 --vm-bytes 256M --timeout 30s

# 4. 观察 PSI 变化
cat /proc/pressure/memory
# some avg10=45.23 avg60=12.34 avg300=4.56 total=78901234
# full avg10=23.45 avg60=6.78 avg300=2.34 total=34567890

# 5. 清理
rmdir /sys/fs/cgroup/pressure
```

---

## 与旧书差异

| ULK3 / LKD3 | Bootlin 实验 |
|-------------|-------------|
| 无 DAMON | DAMON 低开销监控 |
| 无 cgroup | cgroup v2 内存隔离 |
| 无 PSI | PSI 压力量化 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** DAMON 的 DAMOS pageout 和传统 kswapd 回收有什么区别？

> kswapd 回收基于 LRU/MGLRU 代的粗粒度判断（整个链表扫描）。DAMOS pageout 基于 DAMON 的细粒度访问模式（知道哪些区域真正未被访问）。DAMOS 可以针对特定进程/地址空间区域回收，而不是全局扫描。但 DAMOS 需要额外开销（~1% CPU），适合有明确冷热分离的场景。

**Q2:** cgroup v2 的 memory.high 如何减少 OOM？对 HFT 运维有什么启示？

> memory.high 在达到硬限制 (memory.max) 之前就开始积极回收，给进程"减速"而非直接杀死。这减少了 OOM 的突然性。对 HFT 运维：为非交易进程（日志、监控）设 memory.high 限制其内存增长，防止它们消耗系统内存导致交易进程 OOM。交易进程自身不用 memory.high（软回收引入延迟），直接用 memory.max 设明确上限。

</details>
