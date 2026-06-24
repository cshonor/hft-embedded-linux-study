# Ch 16 · 页高速缓存和页回写 · The Page Cache and Page Writeback

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

### ① 缓存策略与写回 · Write-back

Linux 对 **可缓存的页数据** 采用 **写回（write-back）** — 非 no-write、非 write-through。

| 策略 | 行为 |
|------|------|
| **write-back** | 写先进入 **页高速缓存** → 页标 **脏（dirty）** → 入 **脏页链表** → **定期写回磁盘** → 清脏 |

```
应用 write()
    ▼
页缓存（内存）— 立即返回（通常）
    ▼
（稍后）flusher 写回磁盘
```

| 对比 | |
|------|--|
| **write-through** | 每次写都落盘 — 慢、一致性强 |
| **write-back** | 批量异步写 — **快** · 崩溃可能丢未回写数据 |

**HFT：** tick 路径 **不应依赖** 写回完成；关键持久化用 **`fsync`** / 独立日志盘 / **`O_DIRECT`** 自管缓存。

→ [02 SysPerf Ch8 FS](../../02-Systems-Performance-2nd/chapter-08-file-systems/) · [Ch7 `vm.dirty_*`](../../02-Systems-Performance-2nd/chapter-07-memory/notes/section-7.6-调优指南.md)

---

### ② 缓存回收与双链表策略

内存紧张时需 **驱逐缓存页** — 优先换出 **干净** 页。

Linux 使用修改版 **LRU** — **双链表**：

| 链表 | 含义 |
|------|------|
| **active（活跃）** | **热数据** — **不会被回收** |
| **inactive（非活跃）** | **可回收候选** |

| 规则 | 说明 |
|------|------|
| 页在 inactive 被 **再次访问** | 提升到 **active** |
| 仅 inactive 上 **干净页** 可被回收 | |

```
解决什么问题？
  一次性顺序读大文件 — 若传统 LRU，会冲掉真正热的缓存
  双链表：一次扫描的页留在 inactive，不挤 active 热页
```

→ **Ch 12** 物理页回收

---

### ③ address_space 与基数树

页缓存 **通用** — 不只缓存「文件」，还缓存任何 **基于页的对象**（含部分 mmap 路径）。

| 结构 | 角色 |
|------|------|
| **`address_space`** | 管理 **缓存条目 + 页 I/O** — 可视为 VMA 的 **「物理页侧」对应物** |
| 关联 | 常挂在 **inode** 上（`inode->i_mapping`） |

#### 基数树 · Radix Tree

| 用途 | 按 **文件偏移** 快速查 **缓存页是否在内存** |
|------|---------------------------------------------|
| 替代 | 2.6 前 **全局哈希** — 锁争用、开销大 |

```
address_space
    └── radix tree: offset ──► struct page *
```

> 新内核中部分实现演进为 **xarray** — 语义仍为 **索引 → page**。

→ **Ch 15** VMA · **Ch 13** inode

---

### ④ 缓冲区高速缓存 · Buffer Cache

| 历史 | 磁盘块经 **buffer_head** 映射到页（Ch 14） |
|------|---------------------------------------------|
| **2.4+** | 独立 **buffer cache** 与 **page cache** **统一** |
| 效果 | 块 **直接缓存在页缓存** — 无双重拷贝、无重复占用 |

```
今天：read 文件块 ──► 页缓存中的一页 ──► 需要时 bio 写盘（Ch 14）
```

---

### ⑤ flusher 线程 · The Flusher Threads

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

---

### ⑥ 膝上型计算机模式 · Laptop Mode

| 目标 | **硬盘尽量停转** — 省电 |
|------|-------------------------|
| 行为 | 除超时脏页外，在磁盘 **因其他 I/O 已转** 时 **搭便车** 写回 **全部脏缓冲** — 避免 **专为写回再启动** 硬盘 |

| 场景 | 笔记本 · 非 HFT 实盘常态 — 了解即可 |

---

### ⑦ 历史演进与避免拥塞

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

---

### 读路径与写路径（衔接）

```
read(path)
    ▼
VFS（Ch 13）─► 查 address_space / 页缓存
    ├─ 命中 ──► 拷贝到用户缓冲（零拷贝/mmap 可优化）
    └─ 未命中 ──► 读盘（Ch 14 bio）─► 填入页缓存

write(path)
    ▼
页缓存（可能 COW，Ch 3/15）─► 标脏 ──► flusher 异步写回
```

| 绕过页缓存 | **`O_DIRECT`** — DB/自管缓冲 · HFT 大数据文件有时 mmap + mlock |

---

### Ch 16 小结

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

### 检查单

- [ ] 对比 **write-back vs write-through**
- [ ] 解释 **active/inactive** 双链表解决的一次性读问题
- [ ] 说出 **`address_space`** 与 inode/VMA 的关系
- [ ] 列出 flusher **三种触发** 条件
- [ ] 画 **write → 脏页 → 异步回写** 路径
- [ ] 知 **`O_DIRECT` / mmap`** 与页缓存的关系

---

## 相关章节

- 上一章：[chapter-15-进程地址空间.md](./chapter-15-进程地址空间.md)
- 下一章：[chapter-17-设备与模块.md](./chapter-17-设备与模块.md)
- 相关：[chapter-14-块IO层.md](./chapter-14-块IO层.md) · [chapter-13-虚拟文件系统.md](./chapter-13-虚拟文件系统.md)
- 本模块导读：[README.md](./README.md) · [OUTLINE.md](./OUTLINE.md)
