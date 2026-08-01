## 与上下章衔接

```
read() / write()
    ▼
VFS（Ch 13）
    ▼
页缓存（Ch 16）— 命中则不经块层
    ▼
未命中 / 回写 ──► bio ──► request_queue ──► I/O 调度器 ──► 驱动
```

---
