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

### 常见陷阱

1. **记混 L1/L2/L3 的容量和延迟** — L1 ~32KB/~4 cycles，L2 ~256KB-1MB/~10-15 cycles，L3 ~数 MB/~40 cycles（共享）。记住数量级即可，具体值看 CPU 型号。
2. **忽略 inclusive/exclusive LLC 的影响** — inclusive LLC 意味着 L3 包含 L1/L2 的副本（MESI 一致性简化）；exclusive 意味着 L3 不包含 L1/L2 的数据（容量利用率高但一致性复杂）。影响多核性能。
3. **不知道硬件预取器的存在** — 现代 CPU 有 stride prefetcher，自动检测顺序访问模式并预取。但随机访问模式不会被预取。HFT 可用软件预取补充硬件预取的不足。

### 自测题

<details>
<summary>1. 典型 x86 服务器 CPU 的 L1/L2/L3 参数是什么？</summary>

L1i/L1d：32KB，8-way，64B line，~4 cycles（私有）；L2：256KB-1MB，~10-15 cycles（私有）；L3/LLC：数 MB-数十 MB，~40 cycles（多核共享）。具体值因 CPU 型号而异，用 `lscpu` 或 `cat /sys/devices/system/cpu/cpu0/cache/` 查看。
</details>

<details>
<summary>2. inclusive 和 exclusive LLC 有什么区别？</summary>

**Inclusive**：L3 包含 L1/L2 中所有数据的副本——一致性协议简单（L3 可作为 snoop filter），但浪费 L3 容量。**Exclusive**：L3 不包含 L1/L2 的数据——L3 容量利用率高，但一致性管理复杂。Intel 常用 inclusive，AMD 常用 exclusive。
</details>

<details>
<summary>3. 硬件预取器如何工作？HFT 如何利用？</summary>

硬件 stride prefetcher 自动检测**顺序访问模式**（如每次 stride=64B），提前预取下几条 cache line。HFT 中：①顺序扫描数组时硬件预取通常有效；②随机访问模式不会被预取——需要软件 `__builtin_prefetch` 补充；③预取太多可能污染 cache（踢出有用数据）。
</details>

---

← [§6.4.5 ←](./section-6.4.5-有关写的问题.md) · [本章导读](../README.md) · [§6.4.7 →](./section-6.4.7-Cache参数的性能影响.md)
