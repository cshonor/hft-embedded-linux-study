# 第9章 Java 与 JVM 低延迟系统（索引）

> **原书第 9 章 · Java and JVM for Low-Latency Systems**
> **GC · Tiered JIT · 绑核 · Disruptor · JMH · 异步日志**

← [chapter-08 C++ 微秒征途](../chapter-08-超低延迟核心引擎开发/README.md) · [chapter-07 Disruptor](../chapter-07-无锁数据结构与内存布局/7.6-LMAX-Disruptor.md)

---

## 本章定位

Java 运行在 **JVM** 上，常被认为有 **GC 停顿** 与 **预热** 问题，不适合 HFT。但 Java 拥有 **庞大生态、跨平台**，且能避免 C/C++ **内存管理失误 → Segfault**。

原书 **Ch9** 结论：**Mechanical Sympathy** — 深刻理解 GC、对象分配、JIT 分层，配合 **Disruptor + CPU 绑核**，Java 同样可构建 **μs 级** 稳定系统。

| 主题 | 本章 | 交叉 |
|------|------|------|
| 无锁环 / Disruptor | **9.4** | [Ch7 §6](../chapter-07-无锁数据结构与内存布局/7.6-LMAX-Disruptor.md) |
| 绑核 / NUMA | **9.3** | [Ch5 §2](../chapter-05-操作系统内核极致调优/5.2-CPU隔离与核心绑定.md) |
| 异步日志 | **9.5** | [Ch10 §4](../chapter-10-延迟测量与基准压测/10.4-异步日志.md) |
| C++ 热点对比 | — | [Ch8](../chapter-08-超低延迟核心引擎开发/README.md) |

## 小节索引

| 节 | 主题 | 一句话 |
|----|------|--------|
| [9.1](./9.1-驯服垃圾回收GC.md) | 驯服垃圾回收 (GC) 🔴 | 低停顿 GC 选型 + 零对象创建 + 堆外 |
| [9.2](./9.2-JVM预热与分层编译.md) | JVM 预热与分层编译 | Tier 0–4 · 假流量跑热 C2 · ReadyNow/AOT |
| [9.3](./9.3-线程与核心绑定.md) | 线程与核心绑定 | 线程池 + Affinity + 同 NUMA |
| [9.4](./9.4-Disruptor与环形缓冲.md) | Disruptor 与环形缓冲 | Java 热路径禁锁 — 无锁环替代 |
| [9.5](./9.5-JMH测量与异步日志.md) | JMH 测量与异步日志 | JMH 微基准 + 零 GC 二进制日志 |
| [9.6](./9.6-Java在HFT中的分工.md) | Java 在 HFT 中的分工 | C++ vs Java vs Python 边界 |

## 本章小结

| 原书 Ch9 主题 | 手段 |
|---------------|------|
| **GC** | ZGC/Shenandoah/Epsilon · **零对象创建** · 池 + primitive |
| **JIT** | Tier 0–4 · **假流量预热** · Zing ReadyNow / Graal AOT |
| **线程** | 线程池 · **Affinity 绑核** · 同 NUMA |
| **IPC** | **Disruptor** / Agrona 无锁环 |
| **测量/日志** | **JMH** · Disruptor 异步日志 · **无 String 热点** |

**硬核 Java 之后** → [chapter-14 Python 混合（原书 Ch10）](../chapter-14-python-高性能混合架构/README.md) · 策略：[chapter-13](../chapter-13-高频做市与套利策略/README.md)

## 原书章节对照

| 原书 | 本仓库 |
|------|--------|
| Ch9 §1 GC | **本章 9.1** |
| Ch9 §2 JIT/预热 | **本章 9.2** |
| Ch9 §3 线程/绑核 | **本章 9.3** · Ch5 |
| Ch9 §4 Disruptor | **本章 9.4** · Ch7 §6 |
| Ch9 §5 JMH/日志 | **本章 9.5** · Ch10 |
| Ch10 Python | **Ch14** |
| 做市/套利（本仓库扩展） | **Ch13** |

## Java 热点速查（Do / Don't）

| Do | Don't |
|----|-------|
| **对象池** · primitive 数组 | 热点 `new` · autoboxing |
| **Disruptor** 进程内 IPC | `synchronized` 热路径 |
| **JMH** 微基准 | 手写 nanoTime 循环 |
| **绑核 + 预热** | 依赖默认 GC 与冷 JIT |
| **异步二进制日志** | 热点 `String` 拼接 |
