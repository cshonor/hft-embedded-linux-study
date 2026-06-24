# Ch 11 · 定时器和时间管理 · Timers and Time Management

> **Linux Kernel Development 3rd** · Robert Love · **精读**  
> 本章定位：**HZ / jiffies**、硬件时钟、**tick 中断**、墙上时间 **`xtime`**、**动态定时器** 与 **延迟执行**。理解 **调度 tick、时间戳、定时回调** 与 HFT **延迟测量 / 节拍开销** 的关系。

---

## 本节结构

| 节 | 主题 | 带走什么 |
|----|------|----------|
| **① HZ** | 节拍率 | 精度 vs 开销 · **Tickless** |
| **② jiffies** | 全局节拍计数 | 溢出 · `time_after` |
| **③ 硬件时钟** | RTC vs 系统定时器 | 墙上时间 vs tick |
| **④ tick 处理** | `tick_periodic` | 调度 · 负载 |
| **⑤ 墙上时间** | `xtime` | seqlock · `gettimeofday` |
| **⑥ 动态定时器** | `timer_list` | **TIMER_SOFTIRQ** |
| **⑦ 延迟执行** | busy / udelay / sleep | 上下文约束 |

---

### ① 内核时间概念与节拍率 · HZ

内核靠硬件 **周期性中断** 管理时间。

| 术语 | 说明 |
|------|------|
| **节拍率（Tick Rate）** | 系统定时器触发频率 |
| **`HZ`** | 编译期宏 — **每秒 tick 次数** |

| x86 常见 HZ | 周期 |
|-------------|------|
| **100** | 10 ms / tick |
| **1000** | 1 ms / tick |

#### HZ 权衡

| 提高 HZ | 收益 | 代价 |
|---------|------|------|
| ✓ | 定时器 **分辨率** ↑ · 时间驱动事件更准 · **调度延迟** ↓ | **中断开销** ↑ · **功耗** ↑ |

#### 无节拍 · Tickless

| 概念 | 说明 |
|------|------|
| **Tickless OS** | 按需调度 tick 中断 — **空闲时少打断** · 省电 |
| 现代名 | **NO_HZ** 等配置 |

**HFT：** 实盘机常 **高 HZ 或 hrtimer** 换时间精度，但要算 **tick 与策略同核** 的开销；隔离核 + tickless 可减 **无关 wake-up**。

→ **Ch 4** `scheduler_tick` · 抢占

---

### ② jiffies 变量

| 变量 | 说明 |
|------|------|
| **`jiffies`** | 自启动以来的 **节拍总数**（对用户可见常为 **低 32 位**） |
| **`jiffies_64`** | **64 位** 真值 — 防 32 位溢出 |

#### 溢出与回绕

| 问题 | 数据 |
|------|------|
| 32 位 `jiffies` @ HZ=1000 | **~49.7 天** 回绕 |

比较超时 **不能** 简单 `if (jiffies > expires)` — 须用 **回绕安全宏**：

| 宏 | 用途 |
|----|------|
| **`time_after(a, b)`** | a 是否在 b **之后**（考虑回绕） |
| **`time_before(a, b)`** | a 是否在 b **之前** |
| **`time_after_eq` / `time_before_eq`** | 含相等 |

#### `USER_HZ`

向用户空间导出时，内核用 **`USER_HZ`** 做 **jiffies ↔ 用户态时间单位** 的稳定转换（常与 `sysconf(_SC_CLK_TCK)` 一致）。

→ **Ch 10** `jiffies` 更新用 **seqlock**

---

### ③ 硬件时钟和定时器

架构通常提供两类设备：

| 设备 | 特点 | 用途 |
|------|------|------|
| **RTC（实时时钟）** | **电池供电**、非易失 | 关机仍走时；**启动时读** → 初始化 **墙上时间** |
| **系统定时器** | 周期性 **可编程中断** | 驱动 **HZ tick**；x86 上常为 **PIT** |

```
启动：RTC ──读──► 校准 xtime（粗）
运行：系统定时器 ──周期 IRQ──► jiffies++、调度、动态定时器…
```

---

### ④ 定时器中断处理程序

分为 **体系结构相关** 入口 + **体系结构无关** 核心逻辑。

#### `tick_periodic()`（概念职责）

每次 tick 大致做：

| 工作 | 说明 |
|------|------|
| **`jiffies_64++`** | 全局节拍推进 |
| **进程资源统计** | 当前进程 CPU 时间等 |
| **到期动态定时器** | 检查并触发 |
| **`scheduler_tick()`** | 时间片、CFS、**可能触发抢占** |
| **更新 `xtime`** | 墙上时间推进 |
| **负载计算** | 系统 load average 等 |

```
timer IRQ
    ▼
tick_periodic()
    ├─ jiffies_64++
    ├─ run timers
    ├─ scheduler_tick()  ──► Ch 4
    └─ update xtime (seqlock)
```

**HFT：** `scheduler_tick` 出现在 **非绑核 / 非 FIFO** 路径上 → 理解 **tick 抖动** 来源之一。

---

### ⑤ 实际时间 / 墙上时间 · Time of Day

| 变量 | 类型 | 含义 |
|------|------|------|
| **`xtime`** | `struct timespec` | 自 **1970-01-01 Epoch** 起的 **秒 + 纳秒** |

#### 并发保护

| 机制 | 说明 |
|------|------|
| **`xtime_lock`（seqlock）** | 读多写少 — **Ch 10 seqlock** |

#### 用户空间

| API | 内核实现 |
|-----|----------|
| **`gettimeofday()`** | **`sys_gettimeofday()`** |

**HFT：** 行情 **UTC 对齐**、日志时间戳 — 用户态更常用 **`clock_gettime(CLOCK_REALTIME/MONOTONIC)`**；懂 `xtime` 即懂 **系统时间从哪来**。

→ [07-TLPI 时间章](../../07-The-Linux-Programming-Interface/)

---

### ⑥ 动态定时器 · Dynamic Timers

**推迟执行** 一段 **jiffies** 后再跑回调 — **一次性**，到期 **自动销毁**。

| 结构 | `struct timer_list` |
|------|---------------------|
| 初始化 | **`init_timer()`**（书中 API） |
| 过期时刻 | **`expires`**（jiffies） |
| 回调 | **`function`** |
| 激活 | **`add_timer()`** |
| 改期 | **`mod_timer()`** |
| 同步删除 | **`del_timer_sync()`** — 等回调跑完 |

#### 执行上下文

| 事实 | 说明 |
|------|------|
| 下半部 | 作为 **`TIMER_SOFTIRQ`** **异步** 执行 |
| 约束 | **不可睡眠**（同 softirq）— 回调须短 |

```c
void my_timer_fn(unsigned long data) { /* 快速完成 */ }

setup: expires = jiffies + HZ;  /* 约 1 秒后 */
       function = my_timer_fn;
add_timer(&timer);
```

→ **Ch 8** softirq

---

### ⑦ 延迟执行 · Delaying Execution

除 **定时器回调**，驱动常需 **「等这么久再继续」**。

#### 忙等待 · Busy Looping

| 做法 | 不断读 **`jiffies`** 直到经过 N 个 tick |
|------|----------------------------------------|
| 缺点 | **浪费 CPU** |
| 适用 | 延迟 **恰好是 tick 整数倍** 且无更好选择 |
| 改善 | 循环内 **`cond_resched()`** — 主动让出（仍不优雅） |

#### 短延迟 · `udelay` / `ndelay` / `mdelay`

| API | 量级 |
|-----|------|
| **`udelay()`** | 微秒级 |
| **`ndelay()`** | 纳秒级（极短） |
| **`mdelay()`** | 毫秒级 |

| 实现 | 启动时按 **BogoMIPS** 校准的 **紧凑忙等循环** |
|------|-----------------------------------------------|
| 特点 | **硬件级忙等** — 不睡眠、占 CPU |

**HFT：** 用户态 **自旋等** 类似；微秒级硬件复位常用 `udelay` 思维，但 **热路径避免**。

#### `schedule_timeout()`

| 属性 | 说明 |
|------|------|
| 行为 | 当前任务 **可中断睡眠** + 内核设定时器 → **至少 N 个 tick** 后唤醒 |
| 优点 | **不空转 CPU** — **最理想** 的较长延迟 |
| 前提 | **进程上下文** · **不能持 spinlock** |

```c
set_current_state(TASK_INTERRUPTIBLE);
schedule_timeout(HZ / 2);   /* 约 0.5 秒 @ HZ=1000 */
```

| 对比 | 上下文 |
|------|--------|
| `udelay` | 任意？忙等 — 中断里也可用（慎用） |
| `schedule_timeout` | **仅进程上下文** |

---

### Ch 11 小结

| 问题 | 答案 |
|------|------|
| HZ？ | **每秒 tick 数** — 精度 vs 开销 |
| jiffies？ | **节拍计数** — 用 **`time_after`** 防回绕 |
| RTC vs 系统定时器？ | **墙上初值** vs **周期 tick** |
| tick 做什么？ | jiffies、定时器、**`scheduler_tick`**、xtime、负载 |
| 墙上时间？ | **`xtime` + seqlock** · `gettimeofday` |
| 动态定时器？ | **`timer_list`** · 一次 · **TIMER_SOFTIRQ** |
| 怎么延迟？ | **忙等 jiffies** / **`udelay`** / **`schedule_timeout`（可睡）** |

---

### 检查单

- [ ] 解释 **提高 HZ** 的利弊
- [ ] 为何比较时间用 **`time_after`** 而非裸比较
- [ ] 列出 **`tick_periodic`** 几项核心工作
- [ ] 区分 **动态定时器回调** 与 **`schedule_timeout` 睡眠** 的上下文
- [ ] 知道 **`del_timer_sync`** 为何需要「同步」
- [ ] HFT：区分 **CLOCK_MONOTONIC 测延迟** vs **墙上时间对时**

---

## 相关章节

- 上一章：[chapter-10-内核同步方法.md](./chapter-10-内核同步方法.md)
- 下一章：[chapter-12-内存管理.md](./chapter-12-内存管理.md)
- 本模块导读：[README.md](./README.md) · [OUTLINE.md](./OUTLINE.md)
