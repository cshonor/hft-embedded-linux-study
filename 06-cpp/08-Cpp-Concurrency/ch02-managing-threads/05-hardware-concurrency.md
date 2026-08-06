# 2.5 硬件并发数

> 第 2 章 · 上一节：[2.4 RAII 守卫](04-raii-guard.md) · 下一章：[第 3 章 共享数据](../ch03-sharing-data/README.md)

## 这节讲什么

`std::thread::hardware_concurrency()` 返回硬件线程数——决定线程池大小的参考。

---

## 核心用法

```cpp
unsigned n = std::thread::hardware_concurrency();
// 可能返回 0（无法检测），实际使用要处理
if (n == 0) n = 4;  // fallback
```

返回值是硬件并发支持的线程数（通常 = 物理核心 × 超线程因子）。它是**参考值**——实际最优线程数还受任务类型（CPU 密集 vs IO 密集）、缓存、NUMA 等影响。

---

## 新手要点

- **不是物理核心数**：`hardware_concurrency()` 返回的是硬件线程数（含超线程），可能大于物理核心数。
- **可能返回 0**：标准允许返回 0（无法检测），要处理这种情况。

---

## HFT 关联

- **HFT 不依赖 `hardware_concurrency`**：HFT 固定线程数 + 绑核，手动指定哪个线程跑在哪个核，不靠运行时检测。

---

## 自测题

1. `hardware_concurrency()` 返回什么？可能返回什么特殊值？
2. 为什么 HFT 不依赖这个函数？

---

## 参考与延伸

- 下一章：[第 3 章 共享数据](../ch03-sharing-data/README.md)
- 回到：[第 2 章](README.md)
