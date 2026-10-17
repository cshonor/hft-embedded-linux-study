# M3 · C++ Concurrency（并发编程）

> **里程碑定位：** 🔴 硬门槛 · 开 M4 网络编程（muduo）与 HFT 前必过
> **学习顺序：** M2 对象模型之后、M4 网络编程之前
> **难度：** ⭐⭐⭐⭐⭐

## 包含的书

| 目录 | 书 | 状态 |
|------|-----|------|
| [01-Cpp-Concurrency](./01-Cpp-Concurrency/) | C++ 并发编程实战（第 2 版） | 整章 README 已写，小节笔记待拆 |

## 为什么是硬门槛

HFT 是多线程引擎，muduo 是 one loop per thread 架构。不懂内存模型、data race、无锁结构，程序会"时好时坏"调试到崩溃，muduo 的线程封装也会看天书。

## 阅读建议

- **全书精读**，Ch1–Ch5 是核心（线程管理 → 共享数据 → 同步 → 内存模型与原子操作）。
- **Ch7 无锁结构最难且 HFT 必读**：放最后，等前 5 章扎实了再啃。

## 验收

能写 mutex/condition_variable、理解 data race、能解释 6 种内存序、SPSC 无锁队列为什么用 acquire/release。

## 小节笔记结构

每章按小节拆成独立文件，结构同 M1。11 章 + 4 附录，约 87 个小节。

---

← [学习路线](../LEARNING-PATH.md) · [上一站 M2 对象模型](../M2-object-model/) · [下一站 M4 网络编程](../M4-cpp-network-programming/)
