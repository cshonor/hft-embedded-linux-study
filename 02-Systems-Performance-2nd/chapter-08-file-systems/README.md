# Ch 8 文件系统 · File Systems

> **Systems Performance 2nd** · Brendan Gregg · **选读**

> 本章定位：**应用程序感知的往往是文件系统延迟，不是磁盘延迟** — FS 通过 page cache、预取、写回缓冲把多数逻辑 I/O 挡在内存里。Ch 7 的 page cache / file paging 在这里展开；Ch 9 磁盘是更下一层。  
> **HFT：** tick 热路径通常 **不走文件 I/O**；但 **日志、配置、历史 replay、mmap 数据文件** 仍会踩 FS — 懂「逻辑 I/O ≠ 物理 I/O」可避免误调磁盘、误杀 page cache。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 8.1–8.3 核心概念与模型 | [notes/section-8.1-8.3-核心概念与模型.md](./notes/section-8.1-8.3-核心概念与模型.md) |
| 8.4 文件系统架构与特性 | [notes/section-8.4-文件系统架构与特性.md](./notes/section-8.4-文件系统架构与特性.md) |
| 8.5 分析方法论 | [notes/section-8.5-分析方法论.md](./notes/section-8.5-分析方法论.md) |
| 8.6 观测工具 | [notes/section-8.6-观测工具.md](./notes/section-8.6-观测工具.md) |
| 8.7–8.8 实验与调优 | [notes/section-8.7-8.8-实验与调优.md](./notes/section-8.7-8.8-实验与调优.md) |

---

## 大白话 · 本章就五件事

> **应用发的是逻辑 I/O，磁盘收的是物理 I/O — 中间差了一个 cache。**

**① 逻辑 I/O vs 物理 I/O — 本章第一概念。**

- App → FS = **逻辑 I/O**；FS → 磁盘 = **物理 I/O**。
- **缓存命中** → 逻辑多、物理少（通货紧缩）；**元数据更新** → 逻辑少、物理多（通货膨胀）。

**② 读靠 cache + 预取，写靠回写缓冲。**

- 未命中读进 **page cache**；顺序读触发 **read-ahead**。
- 写先进内存 **write-back**，异步刷盘 — 突发 `fsync` 仍可能卡你。

**③ 绕过 FS：O_DIRECT 与 mmap。**

- **`O_DIRECT`**：绕过 page cache，适合 DB/自管缓存。
- **mmap**：文件映射进地址空间，少 syscall；缺页仍走 FS/page cache。

**④ VFS + 三层缓存 + ext4/XFS/ZFS 选型。**

- **VFS** 统一接口 — BPF 追踪 `vfs_read` 等的好挂点。
- **Page cache / Dcache / inode cache** — `free` 里「buff/cache」大半在这里。

**⑤ 工具与调优：cachestat、ext4dist、fio、noatime。**

- BPF：**opensnoop**、**filetop**、**cachestat**、**ext4slower**。
- 基准：**fio** + 注意 WSS；调优：**noatime**、`posix_fadvise` / `madvise`。

下面按原书 8.1–8.8 展开。

---

## 本章 Checklist

- [ ] 能解释 **逻辑 I/O vs 物理 I/O**、通货紧缩/通货膨胀
- [ ] 知道 **page cache / write-back** 与 Ch 7 `free` 的关系
- [ ] 会用 **`cachestat`** 或等价手段看 cache 命中率
- [ ] 跑过 **fio** 且测试集 **大于 RAM** 或 **`direct=1`**
- [ ] 检查挂载选项：**atime**、日志盘是否与数据面分离
- [ ] 热路径确认：**无 sync 写、无冷读大文件**

---

## HFT 精读捷径（Ch 8 在路线中的位置）

```
Ch 7  内存 — page cache 占 RAM
Ch 8  文件系统（本章：逻辑/物理 I/O、VFS、cache、BPF 工具）
  → Ch 9  磁盘（cache 未命中之后）
  → Ch 5  mmap / O_DIRECT 应用用法
  → Ch 12 fio 方法论
```

**HFT 读法：**

| 场景 | 建议 |
|------|------|
| **tick / 发单热路径** | ⚪ 不应依赖 FS — 确认无意外 `open`/`write` |
| **日志、配置、replay** | 🟡 精读 8.1–8.3 + 8.6 `filetop`/`ext4slower` |
| **mmap 历史数据** | 🟡 8.3 + Ch 7 缺页 + `madvise` |

**本章最小行动集（非热路径机器）：**

1. **`mount | grep`** — 记录 `noatime`/FS 类型。
2. **`sudo cachestat-bpfcc 5`** — 看 page cache 命中比例。
3. **`sudo opensnoop-bpfcc`** 启动策略 30 秒 — 有无意外文件访问。
4. 日志盘 **`fio --direct=1`** 一次，留 baseline。

**Gregg 本章金句（HFT 版）：**

> 应用感受到的是 **文件系统延迟**，不是磁盘延迟 — 先查 **cache 命中**，再查磁盘。  
> **逻辑 I/O 多 ≠ 磁盘忙**；元数据和 atime 可以让磁盘 **比你想的更忙**。

---

## 相关章节

- 上一章：[../chapter-07-memory/](../chapter-07-memory/)
- 下一章：[../chapter-09-disks/](../chapter-09-disks/)
- OS / VFS：[../chapter-03-operating-systems/](../chapter-03-operating-systems/)
- 应用 I/O：[../chapter-05-applications/](../chapter-05-applications/)
- 基准测试：[../chapter-12-benchmarking/](../chapter-12-benchmarking/)
- BPF：[../chapter-15-bpf/](../chapter-15-bpf/)
- LKD 页缓存：[05-LKD ch16](../../05-Linux-Kernel-Development/00_Book_3rd_Notes/chapter-16-页高速缓存和页回写/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
