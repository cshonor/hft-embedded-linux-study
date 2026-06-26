# 6. 内存映射与文件生命周期

### `mmapfiles` / `fmapfault`

**mmap** 也是 I/O 路径 — 不一定经过 `read()`。

| 工具 | 作用 |
|------|------|
| `mmapfiles` | 追踪通过 mmap 的文件访问 |
| `fmapfault` | 映射文件的 **缺页** |

→ 与 [Ch 7 `mmapsnoop`](../../chapter-07-memory/) 互补。

### `filelife`

找出 **寿命极短** 的文件（创建后很快删除）— 临时文件 I/O 优化。

```bash
sudo filelife-bpfcc
```

**场景：** 不必要的 `/tmp` 抖动、安装脚本垃圾。

---
