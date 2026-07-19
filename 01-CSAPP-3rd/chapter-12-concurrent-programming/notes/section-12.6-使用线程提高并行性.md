## 12.6 使用线程提高并行性

> **Ch12 §12.6** · [章导读](../README.md) · 上节 [§12.5 ←](./section-12.5-信号量与预线程化.md) · 下节 [§12.7 →](./section-12.7-其他并发问题.md)

---

- **并行 (parallelism)** — 多核上 **同时** 算；**并发 (concurrency)** — 逻辑上同时进展（可单核切换）
- **计算密集** 任务可拆给多线程；注意 **Amdahl** 串行部分上限（→ [Ch 1](../../chapter-01-tour-of-computer-systems/) 1.9、[Ch 5](../../chapter-05-optimizing-performance/)）
- **绑核 (`pthread_setaffinity_np`)** — 减迁移、稳 cache；HFT 标配

---

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

---

← [§12.5 ←](./section-12.5-信号量与预线程化.md) · [本章导读](../README.md) · [§12.7 →](./section-12.7-其他并发问题.md)
