# Bootlin: OOM killer 与 PSI

> **来源:** [Bootlin Kernel Training — Memory Management](https://bootlin.com/docs/kernel/)
> **主题:** OOM killer、PSI 压力监控
> **对标旧书:** 无 (ULK3/LKD3 未涉及 PSI)

---

## 讲义要点

### OOM Killer 流程

```c
// 源码路径: mm/oom_kill.c

// 1. 内存耗尽, 触发 OOM
// out_of_memory() → oom_kill_process()

// 2. 选择受害者 (oom_badness)
// 分数 = RSS + 页表 + swap 占用
// oom_score_adj 调整 (-1000 ~ 1000)

// 3. 发送 SIGKILL
// 4. 回收受害者内存

int oom_badness(struct task_struct *p, unsigned long totalpages)
{
    long points;
    points = get_mm_rss(p->mm) + get_mm_counter(p->mm, MM_SWAPENTS) +
             mm_pgtables_bytes(p->mm) / PAGE_SIZE;
    // oom_score_adj 调整
    return points;
}
```

### PSI (Pressure Stall Information)

```bash
# /proc/pressure/{cpu,memory,io}
cat /proc/pressure/memory
# some avg10=1.50 avg60=0.85 avg300=0.50 total=12345678
# full avg10=0.80 avg60=0.40 avg300=0.20 total=6789012

# some: 至少一个任务在等待
# full: 所有任务都在等待
# avg10/60/300: 10秒/60秒/300秒平均
# total: 累计微秒
```

### PSI 阈值监控

```c
// 用户态可以通过 poll() 监控 PSI 超阈值
#include <poll.h>

int fd = open("/proc/pressure/memory", O_RDWR);
char trigger[] = "some 50000 1000000";  // 50% some 压力, 10 秒窗口
write(fd, trigger, sizeof(trigger));

struct pollfd pfd = { .fd = fd, .events = POLLPRI };
poll(&pfd, 1, -1);  // 阻塞直到超阈值
// 触发后可以: 扩容、kill 进程、告警
```

### cgroup v2 内存 OOM

```bash
# cgroup v2 内存限制 + OOM 控制
mkdir /sys/fs/cgroup/myapp
echo 1073741824 > /sys/fs/cgroup/myapp/memory.max  # 1GB 限制

# cgroup OOM 时不杀进程, 只通知
echo 1 > /sys/fs/cgroup/myapp/memory.oom.group  # 组内全部杀

# 或使用 memory.oom.event (poll 通知)
```

---

## 动手实验

```bash
# 1. 查看 PSI
watch -n 1 cat /proc/pressure/{cpu,memory,io}

# 2. 模拟内存压力
stress-ng --vm 2 --vm-bytes 4G --timeout 30s

# 3. 观察 OOM
dmesg -w | grep -i oom

# 4. 查看进程 OOM 分数
for pid in $(ps -e -o pid=); do
    score=$(cat /proc/$pid/oom_score 2>/dev/null)
    name=$(cat /proc/$pid/comm 2>/dev/null)
    echo "$score $pid $name"
done | sort -rn | head -10

# 5. PSI 触发器测试
echo "some 10000 1000000" > /proc/pressure/memory
# 然后制造内存压力观察触发
```

---

## 与旧书差异

| ULK3 / LKD3 | Bootlin 讲义 |
|-------------|-------------|
| 无 PSI | PSI (4.20+) 精细压力监控 |
| `badness()` | `oom_badness()` + oom_score_adj |
| 无 cgroup OOM | cgroup v2 memory.oom 控制 |
| 全局 OOM | per-cgroup OOM 隔离 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** HFT 交易进程如何配置 OOM 保护？

> `echo -1000 > /proc/<pid>/oom_score_adj` 使该进程永不被 OOM killer 选中。oom_score_adj=-1000 会让 oom_badness 返回 0，OOM killer 跳过此进程。但注意：如果系统所有进程都被保护，OOM killer 可能 panic。HFT 系统应确保有足够的非关键进程可被 OOM，或预留足够内存避免 OOM 触发。

**Q2:** PSI 的 poll() 触发器如何用于 HFT 运维？

> 设置 memory PSI 阈值（如 some 10% over 10s），当内存压力超阈值时 poll() 返回。运维系统收到通知后可以：(1) 检查是否有进程内存泄漏；(2) 扩容或迁移交易进程；(3) 清理非关键进程释放内存。注意：触发器应在独立的监控进程中运行，不在交易线程中。

</details>
