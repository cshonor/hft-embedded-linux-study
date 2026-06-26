# 7. 工作队列 — `workq`

追踪 **workqueue** 提交与 **handler 执行延迟** 直方图。

```bash
sudo workq-bpfcc 10
```

**场景：** 驱动/子系统 **下半部** 慢 — 网络、块层常见。

---
