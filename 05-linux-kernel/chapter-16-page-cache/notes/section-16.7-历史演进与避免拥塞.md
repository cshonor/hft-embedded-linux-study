## ⑦ 历史演进与避免拥塞

| 时代 | 机制 |
|------|------|
| 早期 | **`bdflush`** · **`kupdated`** — 单线程后台写 |
| 2.6 | **`pdflush`** — 按 **系统负载** 动态扩展线程数 |
| **2.6.32+** | **flusher 线程** — 取代 pdflush |

#### flusher 改进

| 设计 | 收益 |
|------|------|
| **每个磁盘主轴（spindle）一个专属 flusher 线程** | **同步回写** 各盘 |
| 避免 | 所有写回 **堵在同一拥塞设备队列** |
| 结果 | 多盘系统 **整体 I/O 吞吐** 更好 |

```
旧：一个 pdflush 线程 ──► 盘 A 拥塞 ──► 盘 B 写回也被拖死
新：flusher-A / flusher-B ──► 各管各队列
```

→ **Ch 14** request_queue · **NVMe 多队列** 时代思想仍相关



<details>
<summary>自测题（点击展开）</summary>

**Q1.** pdflush 和 flusher 线程的区别？为什么换？

<details><summary>答案</summary>

pdflush（2.6.6+）：全局线程池，多设备竞争同一池 → 锁竞争。flusher（2.6.32+）：每块设备一个线程（bdi_writeback），无跨设备竞争。NVMe 的 multi-queue 进一步：每 CPU 一个提交队列。演进方向：减少锁竞争、提高并行度。这就是为什么 NVMe + multi-queue 比 SATA 快——不仅是带宽，更是架构并行度。

</details>

</details>
---
