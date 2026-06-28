## 6. 共享内存与 POSIX 消息队列

---

### 一、SysV 共享内存 — 最高效 IPC

数据 **不** 在内核与用户空间 **来回拷贝** — 多进程映射 **同一物理页**。

| 步骤 | 说明 |
|------|------|
| **`shmget()`** | 创建/获取共享区 |
| **`shmat()`** | 附加到进程 **虚拟地址空间** |

→ mmap / VMA：[Ch 9](../chapter-09-process-address-space/)

---

### 二、缺页与 `shmem_nopage()`

**延迟分配：**

```
shmat 建立 VMA — 不立刻分配物理页
    ↓ 首次访问
缺页异常 → shmem_nopage()
    ↓
分配页框 → 挂入 **页高速缓存**（shmem/tmpfs 类 backing）
```

→ 请求调页：[Ch 9 section-5](../chapter-09-process-address-space/notes/section-5-请求调页.md) · 页缓存：[Ch 15](../chapter-15-page-cache/)

---

### 三、持久性与 swap

| 特点 | 后果 |
|------|------|
| **持久** | 无进程 attach 时数据 **仍保留** |
| **无真实磁盘文件** | 不能靠 **写回文件** 回收 |
| **内存紧张** | 页须进 **Swap Cache** → **换出到 swap** |

→ [Ch 17 section-6](../chapter-17-page-reclaim/notes/section-6-交换机制.md)

HFT：**POSIX shm / mmap 大页** 常用于 **ring buffer、订单簿快照**；注意 **NUMA 本地** 与 **mlock**。

---

### 四、POSIX 消息队列 (Linux 2.6+)

相对 SysV 的 **现代** 替代：

| 特性 | 说明 |
|------|------|
| **接口** | 基于 **`mqueue` 虚拟文件系统** — 像操作文件 |
| **优先级** | 原生 **消息优先级** |
| **异步通知** | 消息到达 → **信号** 或 **新线程** 回调 |
| **Timeout** | 阻塞操作可设 **时间限制** |

→ 信号通知：[Ch 11](../chapter-11-signals/) · VFS 特殊 FS：[Ch 12](../chapter-12-VFS/)

---

### 五、本章小结

| 机制 | 拷贝 | 典型 HFT |
|------|------|----------|
| 管道/FIFO | 经内核 | 日志、子进程 |
| SysV msg/sem | 经内核 | legacy |
| **共享内存** | **无**（映射后） | **热路径** |
| POSIX mq | 经内核 | 控制面事件 |

---

### 六、后续章节索引

| Ch 19 主题 | 继续读 |
|------------|--------|
| 程序加载执行 | [Ch 20 程序执行](../chapter-20-program-execution.md) 🟡 |
| mmap / 缺页 | [Ch 9](../chapter-09-process-address-space/) 🔴 |
| 用户态 IPC | [08 TLPI IPC](../../../08-The-Linux-Programming-Interface/) |
| 信号 | [Ch 11](../chapter-11-signals/) 🟡 |
| swap / shmem | [Ch 17](../chapter-17-page-reclaim/) 🟡 |

---

← [5. 消息队列](./section-5-IPC消息队列.md) · 下一章 [Ch 20 程序执行](../chapter-20-program-execution.md)
