# Ch 7 内存 · Memory

> **BPF Performance Tools** · Brendan Gregg · **选读 🟡**

> 本章定位：**内存压力与分配路径** — CPU 扩展快于 DRAM 的时代，**内存 I/O** 常是隐性瓶颈。回顾虚拟/物理内存、缺页与回收机制后，介绍用 BPF 把「RSS 涨、OOM、卡顿」定位到 **具体代码路径** 的工具。  
> **HFT：** 热路径应 **预分配 + 池化**，正常交易时很少触发 `memleak`/`drsnoop`；本章主要用于 **共置机内存争抢、泄漏、OOM、swap 误开** 等 incident。与 [Ch 6](./chapter-06-CPU.md) `llcstat` / cache 衔接。  
> **上一章：** [chapter-06-CPU.md](./chapter-06-CPU.md) · **下一章：** [chapter-08-文件系统.md](./chapter-08-文件系统.md)

---

## 1. 本章要回答的问题

| 宏观现象 | BPF 要定位的 |
|----------|--------------|
| RSS 持续增长 | 谁 `malloc`/`mmap` 了没释放？→ `memleak`、`brkstack`、`mmapsnoop` |
| 首次访问变慢 | 哪些路径触发 **缺页**？→ `faults`、`ffaults` |
| 系统卡顿、分配变慢 | **直接回收**？→ `drsnoop`、`vmscan` |
| 进程被杀 | **OOM** 元凶？→ `oomkill` |
| 换入换出 | 谁在用 Swap？→ `swapin` |

**传统工具** 给 **数字**；BPF + **栈追踪** 给 **路径**。

---

## 2. 内存基础知识 (Background)

### 虚拟内存与分配器

| 概念 | 说明 |
|------|------|
| **虚拟地址** | 每进程独立地址空间 |
| **物理页** | 通常 4 KiB（另有 **大页** Huge Page） |
| **堆 (Heap)** | `malloc`/`free` — libc 等 **用户态分配器** |
| **mmap** | 映射文件、匿名大块、共享区 |

→ 用户态 API：[07-TLPI](../07-The-Linux-Programming-Interface/) · 内核实现：[06-Gorman](../06-Linux-Virtual-Memory-Manager/) · [08-ULK](../08-Understanding-Linux-Kernel/)

### 缺页异常 (Page Faults)

应用程序 **首次访问** 新分配的虚拟页时：

```
访问虚拟页 → MMU 无映射 → 缺页异常
    → 内核分配物理页、建立页表
    → RSS 增加
```

| 类型（直觉） | 说明 |
|--------------|------|
| **Minor fault** | 已有物理页或零页，补映射 |
| **Major fault** | 需从磁盘读（如文件 mmap 冷启动） |

**RSS (Resident Set Size)**：进程当前占用的 **物理内存** 量。

### 页面换出与回收

内存紧张时内核 **回收** 非活跃页：

| 机制 | 说明 | 性能影响 |
|------|------|----------|
| **kswapd** | 后台守护进程扫描、释放缓存页 | 通常较温和 |
| **Direct reclaim** | 分配路径上 **同步** 回收 | **阻塞分配** — 应用可感知停顿 |
| **Page cache shrink** | 回收文件缓存 | 后续读盘变慢 |

```
内存压力 ↑
    → kswapd 后台回收
    → 仍不够 → direct reclaim（drsnoop 可见）
    → 仍不够 → swap 或 OOM
```

### Swap 与 OOM Killer

| 手段 | 说明 |
|------|------|
| **Swap** | 匿名页换到磁盘 — **延迟灾难级**（HFT 生产通常 **关闭**） |
| **OOM Killer** | 选牺牲进程释放内存 — `dmesg` 有记录 |

**HFT：** 交易机 **禁用 swap**、设合理 `vm.overcommit` / cgroup 限额；OOM 用 `oomkill` 追溯 **谁吃满内存**。

---

## 3. 传统内存分析工具

[Ch 3 § 60 秒](./chapter-03-性能分析.md) 已含部分命令；本章补充内存专项：

| 工具 | 看什么 |
|------|--------|
| `dmesg` | **OOM killer** 日志、杀谁、call trace |
| `free -h` | 总量、used、available、buff/cache |
| `vmstat 1` | `si/so` swap、`sc` 扫描、`pgmajfault` |
| `ps aux` / `top` | 进程 **RSS**、`%MEM` |
| `pmap -x PID` | 各 **VMA 段** 大小（heap、stack、mmap） |
| `sar -B 1` | 缺页率、页扫描活动 |
| `perf stat -e cache-misses,page-faults` | PMC 级 fault / cache |

```bash
free -h
vmstat 1
pmap -x $(pidof myapp) | tail -1    # total RSS 行
dmesg | grep -i oom
```

**局限：** 知「RSS 高」，不知 **哪条调用栈在分配** — 交给 BPF。

---

## 4. BPF 相对传统工具的优势

| 传统 | BPF |
|------|-----|
| `ps` 看 RSS 总量 | `memleak` 看 **未释放分配栈** |
| `sar` 看 fault 率 | `faults` 看 **哪段代码 fault** |
| `vmstat` 见 swap | `swapin` 看 **哪个进程** 换入 |
| 猜 kswapd 忙 | `vmscan`、`drsnoop` **量化回收延迟** |

---

## 5. 异常与泄漏排查

### `oomkill`

追踪 **OOM kill 事件**：

| 输出信息 | 价值 |
|----------|------|
| 被杀进程 | 受害者 |
| **触发 OOM 的进程** | 真凶（不一定是受害者） |
| 当时 load average | 系统整体压力 |

```bash
sudo oomkill-bpfcc
```

**HFT：** 共置机某策略把内存打满 → 先 `oomkill` 留证，再 `memleak` 查泄漏。

### `memleak` — 内存泄漏 🔴（本章核心）

追踪 **分配/释放**（如 `malloc`/`free`、`kmalloc`/`kfree`），在采集窗口结束时打印 **仍未释放** 的分配及其 **用户/内核栈**。

```bash
# 用户态 libc 分配（需 uprobes，有开销）
sudo memleak-bpfcc -p $(pidof myapp) -t 60

# 内核分配（示例）
sudo memleak-bpfcc -k -t 120
```

| 注意 | 说明 |
|------|------|
| **开销** | 每次 alloc/free 插桩 — **勿长期挂在热路径** |
| **适用** | 泄漏排查、长时间 RSS 爬升 |
| **HFT** | 开发/压测环境用；生产仅短窗口、限 PID |

**原理直觉：** 记录分配指针 → free 时删除 → 结束时 map 里剩的就是 **疑似泄漏** + 分配栈。

---

## 6. 内存分配与映射

### `mmapsnoop`

系统范围追踪 **`mmap()`** — 保护标志、文件、长度等。

```bash
sudo mmapsnoop-bpfcc
```

**场景：** 谁映射了大文件、匿名大块；与 [Ch 8 文件系统](./chapter-08-文件系统.md) `mmap` 行为衔接。

### `brkstack`

追踪 **`brk()`** 扩展堆 — 打印导致堆增长的 **用户态栈**。

```bash
sudo brkstack-bpfcc -p $(pidof myapp)
```

**场景：** RSS 涨但不像 `mmap` — 往往是 **heap 碎裂/增长**。

### `shmsnoop`

追踪 **System V 共享内存**：`shmget`、`shmat`、`shmdt` 等。

```bash
sudo shmsnoop-bpfcc
```

**HFT：** 多进程共享行情 ring、旧式 IPC — 确认是否意外 `shmget`。

---

## 7. 缺页异常分析

### `faults`

按 **用户态栈** 统计缺页 — 生成 **缺页火焰图** 输入。

```bash
sudo faults-bpfcc -p $(pidof myapp) 30
```

**回答：** **哪些代码路径** 首次触碰内存从而触发物理页分配。

### `ffaults`

按 **文件名** 统计缺页 — 哪类 **共享库/映射文件** 导致大量 fault。

```bash
sudo ffaults-bpfcc 30
```

**场景：** 新部署后冷启动慢 — 是否某 `.so` 被大量 demand paging。

### `hfaults`

按进程统计 **大页 (Huge Page)** 缺页 — 大页是否 **真正生效**。

```bash
sudo hfaults-bpfcc
```

**HFT：** 若配置了 `hugetlbfs` / transparent huge page，用此确认 fault 模式是否符合预期。

---

## 8. 内存回收与交换

### `vmscan`

追踪 VM 扫描器 **kswapd** 等在 shrink / reclaim 上花费的时间（tracepoint）。

```bash
sudo vmscan-bpfcc 10
```

**解读：** 内存压力持续时 kswapd 是否 **长时间运行**。

### `drsnoop` — 直接回收 🔴

追踪 **direct reclaim** 事件及 **延迟** — 量化「内存不足时分配路径被拖慢多久」。

```bash
sudo drsnoop-bpfcc
```

| 现象 | 含义 |
|------|------|
| 频繁 `drsnoop` 输出 | 系统在 **同步回收** — P99 抖动常见元凶 |
| 与行情峰值同相 | 共置机内存争用或 cache 过大 |

**HFT：** 延迟尖刺但 CPU/网都正常 → 查 **`drsnoop` + `vmstat`**。

### `swapin`

显示 **从 swap 换入** 的进程 — 谁受了 swap 影响。

```bash
sudo swapin-bpfcc
```

**HFT：** 生产若见非零输出 → **配置事故**（swap 应关）。

---

## 9. 工具选型速查

| 症状 | 优先工具 |
|------|----------|
| 进程 OOM 被杀 | `dmesg` → `oomkill` |
| RSS 缓慢上涨 | `memleak`（限时） |
| 堆增长 | `brkstack` |
| 大 mmap | `mmapsnoop` |
| 启动后首次慢 | `faults`、`ffaults` |
| 莫名卡顿 | `drsnoop`、`vmscan` |
| swap 活动 | `vmstat si/so` → `swapin` |
| 大页配置验证 | `hfaults` |
| cache/LLC | [Ch 6 `llcstat`](./chapter-06-CPU.md) |

---

## 10. BPF / bpftrace One-Liners（示意）

```bash
# 按 comm 统计 page fault（bpftrace，字段因内核而异）
bpftrace -e 'software:page-faults:1000 { @[comm] = count(); }'

# malloc 探针（短跑验证；生产用 memleak-bpfcc）
# bpftrace -e 'uprobe:/lib/x86_64-linux-gnu/libc.so.6:malloc { @bytes = sum(arg0); }'
```

→ 语法：[Ch 5](./chapter-05-bpftrace.md) · 单行集：[附录 A](./appendix-A-bpftrace单行命令.md)

---

## 11. 与 CPU / 文件系统章节的衔接

```
Ch 6 CPU          Ch 7 内存           Ch 8 文件系统
llcstat/cache     faults/drsnoop      页缓存、mmap 文件
offcputime        memleak/brk         read/write 路径
```

**延迟排查链：** `runqlat`/`offcputime`（Ch 6）排除调度与阻塞 → **`drsnoop`** 看回收 → **`biolatency`**（Ch 9）看盘。

---

## 12. HFT 读者 Takeaway

1. **热路径设计** 应让 Ch 7 工具在常态下 **几乎无事可做** — 池化、预分配、禁 swap。
2. **`memleak` / `drsnoop` 是 incident 工具** — 短窗口、限 PID，勿与低延迟核长期共存。
3. **Direct reclaim** 是「内存够但抖」的常见根因 — `drsnoop` 比 `free` 更 actionable。
4. **OOM** 用 `oomkill` 找 **触发者**，不是只看被杀进程。
5. **缺页火焰图 (`faults`)** 用于冷启动、新库上线 — 非 tick 热路径常态监控。
6. 深入内核 MM：[06-Gorman](../06-Linux-Virtual-Memory-Manager/) · [05-LKD 内存章](../05-Linux-Kernel-Development/)

---

## 相关章节

- 上一章：[chapter-06-CPU.md](./chapter-06-CPU.md)
- 下一章：[chapter-08-文件系统.md](./chapter-08-文件系统.md)
- 磁盘 I/O：[chapter-09-磁盘IO.md](./chapter-09-磁盘IO.md)
- 方法论：[chapter-03-性能分析.md](./chapter-03-性能分析.md)
- SysPerf 内存：[chapter-07-memory](../02-Systems-Performance-2nd/chapter-07-memory/)
- CSAPP 虚拟内存：[chapter-09-virtual-memory](../01-CSAPP-3rd/chapter-09-virtual-memory/)
