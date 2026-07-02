## ② 性能测试机制 · 量化优化

Day 11 **窗口内高速计数器** 复用为 **benchmark**。

| 设定 | 原因 |
|------|------|
| 启动 **3 秒后** 再 **清零** 计数器 | 避开 **init / 首屏 refresh** 干扰 |
| 跑 **10 秒** 看 **count 最大值** | 优化前后 **同一 workload** 比吞吐 |

```
优化前 10s → count = A
优化后 10s → count = B   (B > A 即 ISR/主循环更「空」)
```

**原则：** 底层改算法 **必须可测** — 与 [03 SysPerf 基准](../../../../13-Systems-Performance-2nd/chapter-12-benchmarking/) 同源。

---
