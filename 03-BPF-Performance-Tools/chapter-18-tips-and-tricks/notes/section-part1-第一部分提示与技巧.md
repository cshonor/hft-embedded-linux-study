# 第一部分：提示与技巧 (Tips and Tricks)

### 1. 事件频率与性能开销

生产最大顾虑：**BPF 拖慢系统**。

**三因素：**

```
开销 ∝ (事件频率 × 每事件动作成本) / CPU 数量
```

| 因素 | 说明 |
|------|------|
| **频率** | 每秒触发次数 — 差 **数量级** |
| **动作** | 计数 vs `printf` vs uprobe 读栈 |
| **CPU 数** | 多核分摊 per-CPU buffer，但全局竞争仍在 |

**频率对比（直觉）：**

| 事件 | 约频率 | 开销直觉 |
|------|--------|----------|
| 线程休眠、**exec** | 几次/秒 | 可忽略 |
| **syscall** 聚合 | 千–百万/秒 | 看工具 |
| **每包 kprobe**、每函数 **trace** | 百万–千万/秒 | **极端** — 勿生产常开 |

**探针类型成本（书中测试趋势）：**

| 类型 | 相对成本 |
|------|----------|
| **kprobe** | 较低 |
| tracepoint | 低（稳定） |
| **uprobe / uretprobe** | **最高** — 单事件可 **>1µs** 级 |

**HFT 纪律：**

- 热路径：**聚合 Map**（`count`/`hist`）— [Ch 4](../../chapter-04-bcc/)  
- 禁：**trace` 逐行**、高频 **uprobe**  
- **短窗口、限 PID、限核**

---

### 2. 以 49 或 99 Hz 采样

CPU 剖析（`profile`、perf）— **勿用整数 100 Hz**。

| 问题 | 说明 |
|------|------|
| **锁步采样 (lockstep)** | 应用每 **10ms** 定时任务 + **100Hz** 采样 → 永远采到或永远采不到 → **扭曲** |
| **对策** | **99Hz 或 49Hz** — 与周期 **互质** |

```bash
sudo profile-bpfcc -F 99 30
perf record -F 99 -a -g -- sleep 30
```

→ [Ch 6 § profile](../../chapter-06-cpus/) · [Ch 2 § 火焰图](../../chapter-02-technology-background/)

---

### 3. 黄猪与灰鼠：特殊数字法

**场景：** 知道行为（如「某次 write」）但 **不知内核函数名**。

**步骤：**

1. 写测试程序，**精确执行 N 次**（如 **230,000**）目标操作  
2. N 用 **罕见计数** — 书中用 **23（灰鼠）/ 17（黄猪）** 等质数组合，系统中 **自然出现概率极低**  
3. 同时跑 **`funccount`** 统计内核函数  
4. 在输出中 **搜 `230000`** — 瞬间定位 **目标函数**

```bash
# 终端 A：全函数计数（短跑）
sudo funccount-bpfcc -r '^[a-z_]+$' &
# 终端 B：跑 230000 次 write 的小程序
./my_230000_writes
# 在 funccount 结果里 grep 230000
```

**本质：** **可控实验** + **全局搜索** — 比盲读内核源码快。

---

### 4. 编写目标软件 (Write Target Software)

| 误区 | 对策 |
|------|------|
| 只在庞大生产系统上试 | **先写最小负载生成器** |
| 不懂 struct/参数 | 自己写代码 → 知 **参数含义、返回值** |

**HFT：** 复现 **单 syscall、单 connect** 的 microbench，再挂 BPF — 与 [15-HFT ch10 压测](../15-HFT-Low-Latency-Practice/chapter-10-延迟测量与基准压测.md) 同思路。

---

### 5. 学习系统调用 (Learn Syscalls)

**syscall = 用户态与内核边界** — tracepoint 字段来自 **内核 syscall 入口定义**。

```bash
man 2 read
man 3 getaddrinfo
man 2 setitimer
ls /sys/kernel/debug/tracing/events/syscalls/
```

**收益：** 写 bpftrace 时知道 **`args->` 有什么** — 少猜 struct。

→ [07-TLPI](../07-The-Linux-Programming-Interface/) · [Ch 5 bpftrace](../../chapter-05-bpftrace/)

---

### 6. 保持简单 (Keep It Simple)

| 诱惑 | 后果 |
|------|------|
| 一个脚本挂 **所有 probe** | 高开销、难维护、难解释 |
| **最好工具** | 只追 **回答当前问题** 的最少事件 |

**Gregg 原则：** 与 Unix 单用途工具哲学一致 — [Ch 4 § 单用途](../../chapter-04-bcc/)。

---
