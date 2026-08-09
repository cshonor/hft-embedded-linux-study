## ⑥ 时间与页大小

#### jiffies 与 HZ

| 事实 | **`HZ`** 因架构/配置而异 |
|------|--------------------------|
| 错误 | `jiffies + 300` 当「3 秒」而不看 HZ |
| 正确 | **`msecs_to_jiffies(ms)`** · **`jiffies_to_msecs`** |

→ **Ch 11** `HZ` · `time_after`

#### PAGE_SIZE

| 事实 | **不固定** — x86-32 常 4KB，其他或 **8/16/64KB** |
|------|--------------------------------------------------|
| 必须用 | **`PAGE_SIZE`** · **`PAGE_SHIFT`** |

```c
order = get_order(size);
alloc_pages(GFP_KERNEL, order);
```

→ **Ch 12** 页分配 · **Ch 15** 映射

---
