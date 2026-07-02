# Ch 16 页高速缓存和页回写 · The Page Cache and Page Writeback

> **Linux Kernel Development 3rd** · Robert Love · **背景**

> 本章定位：内核用 **页缓存** 减少磁盘 I/O、**写回** 刷脏页；**双 LRU**、`address_space`、**flusher** 线程。理解 **逻辑 I/O ≠ 物理 I/O** — HFT **热路径绕开 FS**，日志/replay 仍要懂 **dirty / fsync**。

---

## 本节结构

| 节 | 主题 | 带走什么 |
|----|------|----------|
| **① 写回策略** | write-back | **脏页** |
| **② 双链表 LRU** | active / inactive | 回收干净页 |
| **③ address_space** | 基数树 | 按偏移找页 |
| **④ buffer cache** | 与页缓存统一 | 2.4+ |
| **⑤ flusher** | 脏页写回触发 | `dirty_*` · `fsync` |
| **⑥ Laptop Mode** | 省电写回 | 搭便车 |
| **⑦ 历史演进** | bdflush → flusher | 每盘一线程 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 缓存策略与写回 | [notes/section-16.1-缓存策略与写回.md](./notes/section-16.1-缓存策略与写回.md) |
| 缓存回收与双链表策略 | [notes/section-16.2-缓存回收与双链表策略.md](./notes/section-16.2-缓存回收与双链表策略.md) |
| address_space 与基数树 | [notes/section-16.3-address_space-与基数树.md](./notes/section-16.3-address_space-与基数树.md) |
| 缓冲区高速缓存 | [notes/section-16.4-缓冲区高速缓存.md](./notes/section-16.4-缓冲区高速缓存.md) |
| flusher 线程 | [notes/section-16.5-flusher-线程.md](./notes/section-16.5-flusher-线程.md) |
| 膝上型计算机模式 | [notes/section-16.6-膝上型计算机模式.md](./notes/section-16.6-膝上型计算机模式.md) |
| 历史演进与避免拥塞 | [notes/section-16.7-历史演进与避免拥塞.md](./notes/section-16.7-历史演进与避免拥塞.md) |
| 读路径与写路径（衔接） | [notes/section-16.8-读路径与写路径衔接.md](./notes/section-16.8-读路径与写路径衔接.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| Linux 写策略？ | **write-back** · **脏页** 延迟落盘 |
| 如何回收？ | **active / inactive** 双 LRU |
| 谁管缓存索引？ | **`address_space` + 基数树** |
| buffer cache？ | **已并入 page cache**（2.4+） |
| 何时写回？ | **内存压力 · 超时 · sync/fsync** |
| flusher 演进？ | **bdflush → pdflush → per-spindle flusher** |
| HFT？ | 热路径 **少 FS 写**；懂 **fsync 尖刺** 与 **cachestat** |

---

## 本章学习目标 · 自检

- [ ] 对比 **write-back vs write-through**
- [ ] 解释 **active/inactive** 双链表解决的一次性读问题
- [ ] 说出 **`address_space`** 与 inode/VMA 的关系
- [ ] 列出 flusher **三种触发** 条件
- [ ] 画 **write → 脏页 → 异步回写** 路径
- [ ] 知 **`O_DIRECT` / mmap`** 与页缓存的关系

---

## 相关章节

- 上一章：[../chapter-15-process-address-space/](../chapter-15-process-address-space/)
- 下一章：[../chapter-17-devices-modules/](../chapter-17-devices-modules/)
- 全书导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
