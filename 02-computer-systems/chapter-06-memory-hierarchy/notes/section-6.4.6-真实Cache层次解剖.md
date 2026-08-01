## 6.4.6 真实 Cache 层次解剖（Intel 类）

> **Ch6 §6.4.6** · [章导读](../README.md) · 上节 [§6.4.5 ←](./section-6.4.5-有关写的问题.md) · 下节 [§6.4.7 →](./section-6.4.7-Cache参数的性能影响.md)

---

典型桌面/服务器：

```
L1i / L1d  32KB, 8-way, 64B line, ~4 cycles
L2         256KB–1MB per core
L3 LLC     共享，数十 MB
```

- ** inclusive vs exclusive** LLC — 多核一致性协议（MESI）在 LLC 层可见
- **预取器** — 硬件 stride prefetch

---

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

---

← [§6.4.5 ←](./section-6.4.5-有关写的问题.md) · [本章导读](../README.md) · [§6.4.7 →](./section-6.4.7-Cache参数的性能影响.md)
