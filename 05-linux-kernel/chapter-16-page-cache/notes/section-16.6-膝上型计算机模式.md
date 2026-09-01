## ⑥ 膝上型计算机模式 · Laptop Mode

| 目标 | **硬盘尽量停转** — 省电 |
|------|-------------------------|
| 行为 | 除超时脏页外，在磁盘 **因其他 I/O 已转** 时 **搭便车** 写回 **全部脏缓冲** — 避免 **专为写回再启动** 硬盘 |

| 场景 | 笔记本 · 非 HFT 实盘常态 — 了解即可 |

---

### 机制：三个钩子（v6.6 源码逐条）

这个功能**没有独立子系统**，就是在页缓存写回路径上打了三处补丁：

**① 关掉后台写回的"低阈值"唤醒**

```c
/* mm/page-writeback.c:1752 — v6.6 原文注释 + 代码 */
/*
 * In laptop mode, we wait until hitting the higher threshold
 * before starting background writeout, and then write out all
 * the way down to the lower threshold.  So slow writers cause
 * minimal disk activity.
 */
if (!laptop_mode && nr_reclaimable > gdtc->bg_thresh &&
    !writeback_in_progress(wb))
	wb_start_background_writeback(wb);
```

| | 普通模式 | laptop mode |
|---|---------|-------------|
| 后台写回起点 | 脏页刚过 **低阈值**（10%）就写 | **忍住不写**，一路攒到高阈值（20%） |
| 写回量 | 少量多次 | **一次写到底**，一路降到低阈值 |
| 磁盘动作 | 频繁被唤醒 | 长时间停转 |

**② IO 完成后挂一个"延期全刷"定时器 —— 这就是 LKD 说的"搭便车"**

```c
/* mm/page-writeback.c:2166 — v6.6 原文注释 + 代码 */
/*
 * We've spun up the disk and we're in laptop mode: schedule writeback
 * of all dirty data a few seconds from now.  If the flush is already
 * scheduled then push it back - the user is still using the disk.
 */
void laptop_io_completion(struct backing_dev_info *info)
{
	mod_timer(&info->laptop_mode_wb_timer, jiffies + laptop_mode);
}

/* 定时器到期 → 把这台设备的所有脏数据一起写回 */
void laptop_mode_timer_fn(struct timer_list *t)
{
	struct backing_dev_info *backing_dev_info =
		from_timer(backing_dev_info, t, laptop_mode_wb_timer);
	wakeup_flusher_threads_bdi(backing_dev_info, WB_REASON_LAPTOP_TIMER);
}
```

关键在于 `mod_timer` 的 **"推后"语义**：每次有 IO 完成，就把定时器**重新推后** `laptop_mode` 秒。

```
有 IO 活动   有 IO 活动   有 IO 活动      安静 5 秒
    │            │            │              │
    ▼            ▼            ▼              ▼
 [重置5s]    [重置5s]    [重置5s]      定时器到期 → 全量写回
 ────────────────────────────────────────────────────►
          磁盘转着，脏页一直不写（搭便车）
```

**"盘都转着呢，不写白不写"** —— 只要磁盘因为别的原因在转，就一直延后；等它真要停了，才一次性把所有脏页写下去。

**③ 数值语义：`laptop_mode` 既是开关也是时长**

```c
/* mm/page-writeback.c:115 */
/*
 * Flag that puts the machine in "laptop mode". Doubles as a timeout in jiffies:
 * a full sync is triggered after this time elapses without any disk activity.
 */
int laptop_mode;
```

```c
/* mm/page-writeback.c:2288 — sysctl 表项 */
{ .procname = "laptop_mode", .data = &laptop_mode,
  .mode = 0644, .proc_handler = proc_dointvec_jiffies },
```

`proc_dointvec_jiffies` 意味着你写进去的**秒数**会被换算成 jiffies 存起来 —— 所以
`echo 5 > /proc/sys/vm/laptop_mode` = 开关打开 + 静默 5 秒后全量写回；`echo 0` = 关闭。

---

### 现代评价：为什么它基本已成历史

| 维度 | 机械硬盘时代 | NVMe / SSD 时代 |
|------|-------------|----------------|
| 停转省电 | **核心收益**（启动电机 ~1-2W·秒级） | **不存在**——没有机械部件，空闲功耗本来就极低 |
| 延迟写回的收益 | 避免反复启动电机 | 无 |
| 延迟写回的代价 | 掉电丢失窗口变大 | **一样大**，且写回突发更陡 |
| 尾延迟影响 | 一次大 flush 造成卡顿 | **一样卡**，甚至更明显（NVMe 快但队列被突发塞满） |

> **结论：** laptop mode 在 v6.6 里**代码还在**（`page-writeback.c` 的 115 / 1752 / 2158 行），
> 主流发行版默认 `0`。NVMe 服务器上打开它**没有任何收益，只有更大的数据丢失窗口和更差的尾延迟**——不要开。

### 但"攒批写回"的思想活了下来

laptop mode 死了，它背后的**"把多次小写合并成一次大写"**到处都是：

| 现代继承者 | 作用层 | 机制 |
|-----------|--------|------|
| **blk-mq plugging** | 块层 | 攒一批 request 再一次性提交给驱动（见 Ch 14.5） |
| **io_uring 批量提交** | 系统调用 | 一次 `io_uring_enter` 提交 N 个 SQE，摊薄系统调用开销 |
| **journal commit interval** | 文件系统 | ext4 默认 5s 提交一次事务（`commit=` 挂载参数） |
| **`dirty_expire_centisecs`** | 页缓存 | 让脏页攒 30 秒再写（**这其实就是"温和版 laptop mode"**） |

| 与 HFT 的关系 | |
|---------------|--|
| **不要做** | 为省 IO 次数而延迟写回关键日志——掉电就丢 |
| **可以做** | 非关键数据（行情快照、回放缓存）允许攒批，接受丢失 |
| **必须做** | 把 fsync/写回**移出策略线程**，让攒批的代价不落在 tick 路径上 |



<details>
<summary>自测题（点击展开）</summary>

**Q1.** Laptop Mode 对 HFT 有什么启发？

<details><summary>答案</summary>

Laptop Mode 通过延迟写回让磁盘长时间停转省电。HFT 启发：1) 如果交易日志用 NVMe（无机械部件），Laptop Mode 无意义；2) 但原理可借鉴——批量写回减少 IO 次数。HFT 可以调高 vm.dirty_writeback_centisecs 让 flusher 少运行，减少对交易线程的干扰。

</details>

**Q2.** `echo 5 > /proc/sys/vm/laptop_mode` 里的 5 是什么意思？为什么这个变量"既当开关又当时长"？

<details><summary>答案</summary>

这个 `5` 是**秒数**，而且它同时承担两个职责：

1. **开关**：非 0 就是启用 laptop mode，0 就是关闭；
2. **时长**：磁盘静默多少秒后触发全量写回。

原因是内核把它存成了 **jiffies**（`proc_dointvec_jiffies` 处理，用户按秒写、内核按 jiffies 存）。源码注释写得很清楚（`mm/page-writeback.c:115`）：

```c
/*
 * Flag that puts the machine in "laptop mode". Doubles as a timeout in jiffies:
 * a full sync is triggered after this time elapses without any disk activity.
 */
int laptop_mode;
```

运行时它被用在 `laptop_io_completion()`（`page-writeback.c:2173`）：
```c
mod_timer(&info->laptop_mode_wb_timer, jiffies + laptop_mode);
```
每次有 IO 完成就把定时器**推后** `laptop_mode` 秒，直到真的静默了才到期，触发
`WB_REASON_LAPTOP_TIMER` 的全量写回。

这种"一个变量兼职开关和参数"的写法在内核里不算罕见（省一个变量），但对使用者的心智负担是要记住"0 这个值是保留的"。

</details>

**Q3.** 有同事建议在交易服务器上把 `vm.dirty_expire_centisecs` 调大到 300（5 分钟）"减少写回次数、降低 IO 干扰"。评价这个方案。

<details><summary>答案</summary>

这是把 laptop mode 的思路搬到了不该搬的地方。**方向对，剂量错，风险被低估。**

**表面收益**：脏页攒更久 → 更多写合并 → 写回次数下降。在持续覆盖同一批页的场景（不断重写一个热文件）确实有效。

**三个真实代价**：

1. **数据丢失窗口从 30 秒拉到 5 分钟**。这不是"可能丢 30 秒"变成"可能丢 5 分钟"——对交易日志来说是灾难级。哪怕是行情快照，也要先确认"重建成本 < 丢失成本"。

2. **尾延迟会变差而不是变好**。攒 5 分钟的脏页一次性写回，是一次**巨大的突发 IO**：页缓存要扫出几 GB 脏页，块层队列被塞满，`fsync`、甚至不相关的读 IO 都排在后面。结果是 p50 略降、**p99/p999 明显恶化**——HFT 最在意的恰恰是尾部。

3. **前台限速反而更早触发**。`dirty_expire` 只管"年纪"，不管"总量"。攒得越久，`nr_dirty` 越容易撞上 `vm.dirty_ratio`（20%）的前台限速，到时候**写者自己被睡住**等写回，延迟直接落在策略线程上。

**正确做法**（按优先级）：
1. **关键日志**：不要省，`fsync` 正常做，但**挪到独立的 IO 线程**，策略线程只往无锁队列里投递；
2. **物理隔离**：日志盘与行情/回放盘分开，写回突发不互相影响；
3. **真想减少合并损失**：调 `vm.dirty_background_ratio` 让后台更早开始写（**更早**，不是更晚），摊平突发；
4. **非关键的大块数据**：直接 `O_DIRECT` 或写完 `madvise(MADV_DONTNEED)`，根本不产生脏页，比调参数干净得多。

衡量标准始终是尾部延迟分布（`biolatency` 直方图），不是平均 IO 次数。

</details>

</details>
---
