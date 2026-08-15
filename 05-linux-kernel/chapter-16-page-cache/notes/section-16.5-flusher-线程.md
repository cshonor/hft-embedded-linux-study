## ⑤ flusher 线程 · The Flusher Threads

**内核线程组** — 负责 **脏页写回**。

#### 三种触发条件

| # | 触发 | 配置/接口 |
|---|------|-----------|
| 1 | **可用内存低于阈值** | `dirty_background_ratio` 等 — 必须写回释内存 |
| 2 | **脏页停留超时** | `dirty_expire_interval` — 防崩溃丢太多 |
| 3 | **用户显式同步** | **`sync()`** · **`fsync()`** / `fdatasync` |

```
脏页积累
    ├─ 内存压力 ──► flusher 后台写回
    ├─ 时间到期 ──► 周期性写回
    └─ fsync()   ──► 该文件脏页刷盘（可能阻塞）
```

| 观测 | **`cachestat`** 命中率 · **`ext4slower`** 慢 fsync |

**HFT：** 突发 **`fsync` 日志** 仍可能造成 **P99 尖刺** — 与策略核隔离、异步批量、专用盘。



<details>
<summary>自测题（点击展开）</summary>

**Q1.** flusher 线程什么时候触发写回？HFT 如何控制？

<details><summary>答案</summary>

触发条件：1) 脏页比例超阈值（vm.dirty_ratio 默认 20%）；2) 脏页存活超时（vm.dirty_expire_centisecs 默认 30s）；3) 用户调用 sync/fsync。HFT 控制：调小 vm.dirty_ratio 减少突发写回；交易日志用 O_SYNC 或 fsync 保证即时落盘。flusher 是后台线程，不影响前台交易线程（除非内存回收时阻塞）。

</details>

</details>
---
