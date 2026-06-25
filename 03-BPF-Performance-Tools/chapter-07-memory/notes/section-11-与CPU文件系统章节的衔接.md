# 11. 与 CPU / 文件系统章节的衔接

```
Ch 6 CPU          Ch 7 内存           Ch 8 文件系统
llcstat/cache     faults/drsnoop      页缓存、mmap 文件
offcputime        memleak/brk         read/write 路径
```

**延迟排查链：** `runqlat`/`offcputime`（Ch 6）排除调度与阻塞 → **`drsnoop`** 看回收 → **`biolatency`**（Ch 9）看盘。

---
