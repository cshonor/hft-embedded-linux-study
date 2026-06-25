# 2. 内存基础知识 (Background)

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
