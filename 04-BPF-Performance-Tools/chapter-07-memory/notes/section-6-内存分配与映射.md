# 6. 内存分配与映射

### `mmapsnoop`

系统范围追踪 **`mmap()`** — 保护标志、文件、长度等。

```bash
sudo mmapsnoop-bpfcc
```

**场景：** 谁映射了大文件、匿名大块；与 [Ch 8 文件系统](../../chapter-08-file-systems/) `mmap` 行为衔接。

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
