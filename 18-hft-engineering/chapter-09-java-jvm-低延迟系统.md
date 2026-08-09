# 第9章 Java 与 JVM 在低延迟系统中的应用

> **原书第 9 章 · Java and JVM for Low-Latency Systems**  
> **GC · Tiered JIT · 绑核 · Disruptor · JMH · 异步日志**

← [chapter-08 C++ 微秒征途](./chapter-08-超低延迟核心引擎开发.md) · [chapter-07 Disruptor](./chapter-07-无锁数据结构与内存布局.md)

---

## 本章定位

Java 运行在 **JVM** 上，常被认为有 **GC 停顿** 与 **预热** 问题，不适合 HFT。但 Java 拥有 **庞大生态、跨平台**，且能避免 C/C++ **内存管理失误 → Segfault**。

原书 **Ch9** 结论：**Mechanical Sympathy** — 深刻理解 GC、对象分配、JIT 分层，配合 **Disruptor + CPU 绑核**，Java 同样可构建 **μs 级** 稳定系统。

| 主题 | 本章 | 交叉 |
|------|------|------|
| 无锁环 / Disruptor | **§4** | [Ch7 §5](./chapter-07-无锁数据结构与内存布局.md#5-lmax-disruptor参考架构) |
| 绑核 / NUMA | **§3** | [Ch5 §2](./chapter-05-操作系统内核极致调优.md) |
| 异步日志 | **§5** | [Ch10 §4](./chapter-10-延迟测量与基准压测.md#4-移出关键路径异步日志) |
| C++ 热点对比 | — | [Ch8](./chapter-08-超低延迟核心引擎开发.md) |

---

## 1. 驯服垃圾回收 (GC)

GC 清理/整理堆时可能 **Stop-The-World (STW)** — **暂停全部应用线程** — 对 HFT **毁灭性**。

### 选型

| GC | HFT 适用性 |
|----|------------|
| **CMS** | 并发标记 — 老年代 · 已逐步淘汰 |
| **G1** | 可预测停顿目标 · 常用基线 |
| **Shenandoah / ZGC** | **超低 STW** · 大堆友好 |
| **Epsilon** | **只分配不回收** — 内存耗尽即退出 · **极端最低延迟**（生命周期完全自控时） |

**原则：** 选 **并发/低停顿** GC；或 **Epsilon + 预分配/池化** 使堆 **永不触发回收**。

### 零对象创建 (Zero Object Creation)

| 热点路径 | 禁止 |
|----------|------|
| **`new` 短生命周期对象** | 分配在 **Eden** → 频繁 **Minor GC** |
| **Autoboxing** | `int` ↔ `Integer` — 隐式 **装箱对象** |
| **String 拼接** | 每次 `+` 产生新 `String` |

### 对象池与基础类型

| 实践 | 说明 |
|------|------|
| **对象池** | 启动预分配 · 运行时 **复用** |
| **Primitive types** | `int` / `long` / `double` — **更小 · cache 友好** |
| **避免包装类** | `Integer` / `Double` 在热路径 **禁用** |
| **堆外 (Off-heap)** | DirectByteBuffer / Chronicle — GC **不可见** 区域（进阶） |

→ C++ 对照：[chapter-08 §3](./chapter-08-超低延迟核心引擎开发.md#3-动态内存分配与异常)

---

## 2. JVM 预热与分层编译 (Tiered Compilation)

Java **解释 + JIT 并存**。启动期 **解释执行** 慢；热点代码达阈值（约 **~10,000 次**）后 JIT 编译为本地码。

### 五级编译（Tier 0–4）

| Tier | 机制 |
|------|------|
| **0** | 解释执行 |
| **1–3** | **C1** 快速编译 + **Profiling** 收集 |
| **4** | **C2** 极限优化 — **最快机器码** |

**HFT 痛点：** 关键路径（如 **发单**）事件 **不够频繁** → 开盘时仍处 **C1/解释** → **延迟尖刺**。

### 预热 (Warm-up)

| 手段 | 说明 |
|------|------|
| **假订单 / 假行情** | 启动期 **跑热** 关键路径 — 触发 **C2** |
| **JVM 参数** | `-XX:CompileThreshold` · `-XX:+PrintCompilation` 观测 |
| **Azul Zing ReadyNow!** | **保存/重用** 已编译配置 — **免每次重启重预热** |
| **GraalVM AOT** | **Ahead-of-Time** 预编译为本地码 — 减少冷启动 JIT 依赖 |

→ C++ 对照：[chapter-08 §6 假数据预热](./chapter-08-超低延迟核心引擎开发.md#6-实战fx-高频交易系统架构清单) · [chapter-10 T2T 环境](./chapter-10-延迟测量与基准压测.md#5-精确性能测量与基准测试)

---

## 3. 高性能 Java 线程与核心绑定

### 线程池

| 原则 | 说明 |
|------|------|
| **复用线程** | 避免频繁 `new Thread` — **内存 + 切换** 开销 |
| **任务分离** | **I/O / 日志 / 报表** 与 **μs 核心路径** 不同池 |
| **少即是多** | 过多线程 → **上下文切换** 恶化 tail latency |

### 线程亲和性 (Thread Affinity)

| 层 | 手段 |
|----|------|
| **OS** | `isolcpus` + `taskset` — [Ch5](./chapter-05-操作系统内核极致调优.md) |
| **Java** | **`OpenHFT/Java-Thread-Affinity`** — 代码层 **绑核** |
| **Spinning** | 热点线程 **忙等** 队列 — 与 C++ Bypass **同哲学** |

### NUMA

共享内存通信的线程 **绑定同一 NUMA 节点** — 避免 **跨节点远程内存** 访问。

→ [chapter-04 §2 NUMA](./chapter-04-硬件选型与服务器配置.md)

---

## 4. 无锁数据结构：Disruptor 与环形缓冲

Java 热路径 **禁用 Lock** — `synchronized` / `ReentrantLock` 引发 **争用 + STW 风险**。

### 环形缓冲区

| 实现 | 说明 |
|------|------|
| **LMAX Disruptor** | Java HFT **最著名** 无锁队列 — **连续内存 · 预分配 Event · 精准内存屏障** |
| **Conversant Disruptor** | 替代实现 |
| **Agrona circular buffer** | 低层 **off-heap** 环 — 常与 Aeron 配合 |

### Disruptor 核心（与 Ch7 对照）

| 概念 | Java / C++ 共通 |
|------|-----------------|
| **Ring buffer** | 预分配 **数组** · **cache line** 对齐 |
| **Sequence** | 全局序号 · **无锁 claim** |
| **Batching** | 消费者 **批量** 降 amortized 开销 |
| **Mechanical Sympathy** | 迎合 **CPU cache + 预取** |

```
Producer ──► Ring Buffer (pre-allocated events)
                    │
                    ▼
              Consumer(s) — 单/多 · 无锁 sequence
```

→ [chapter-07 §5 LMAX Disruptor](./chapter-07-无锁数据结构与内存布局.md#5-lmax-disruptor参考架构) · [chapter-10 §2 mmap IPC](./chapter-10-延迟测量与基准压测.md#2-内存映射文件-mmap-与-ipc)

---

## 5. 性能测量与日志

### JMH 微基准

Java 微基准 **极难** — JVM **DCE（死代码消除）**、**内联**、**GC** 会 **扭曲** 手写 `System.nanoTime()` 循环。

| 工具 | 说明 |
|------|------|
| **JMH (Java Microbenchmark Harness)** | OpenJDK 官方 · **`@Benchmark` + `@Fork` + warmup** |
| **原则** | **Blackhole** 消费结果 · 多 **fork** · 报告 **percentile** |

```java
@BenchmarkMode(Mode.AverageTime)
@OutputTimeUnit(TimeUnit.NANOSECONDS)
@Warmup(iterations = 5)
@Measurement(iterations = 10)
public void ringPublish(RingState s) {
    s.ring.publish(s.event);
}
```

→ C++ 对照：[chapter-10 §5 rdtsc / T2T](./chapter-10-延迟测量与基准压测.md#5-精确性能测量与基准测试)

### 无锁异步日志

| 问题 | 对策 |
|------|------|
| **磁盘 I/O 慢** | **Disruptor 环** → 专用 **Logger 线程** |
| **String 分配** | 热点 **二进制 blob** · 后台格式化 |
| **log4j zeroGC** 等 | **零 GC** 日志框架 — 预分配 appenders |

**原则：** 与 C++ 相同 — [Ch10 §4](./chapter-10-延迟测量与基准压测.md#4-移出关键路径异步日志) — **格式化永不进热路径**。

---

## 6. Java 在 HFT 中的典型分工

| 路径 | 语言 | 原因 |
|------|------|------|
| **Gateway / Strategy μs 路径** | **C++** | 无 GC · 模板/CRTP |
| **Java 引擎**（部分基金） | **Java + Disruptor** | 生态 · 团队技能 · 调优后可达 μs |
| **风控 / 报表 / 配置 / 监控** | **Java** | 非 STW 敏感 · 库丰富 |
| **研究回测** | **Python** | [Ch14](./chapter-14-python-高性能混合架构.md) |

→ [chapter-08 §8](./chapter-08-超低延迟核心引擎开发.md#8-java--python-边界) · [chapter-01 §4](./chapter-01-高频交易基础与生态.md#4-编程语言选择)

---

## 本章小结

| 原书 Ch9 主题 | 手段 |
|---------------|------|
| **GC** | ZGC/Shenandoah/Epsilon · **零对象创建** · 池 + primitive |
| **JIT** | Tier 0–4 · **假流量预热** · Zing ReadyNow / Graal AOT |
| **线程** | 线程池 · **Affinity 绑核** · 同 NUMA |
| **IPC** | **Disruptor** / Agrona 无锁环 |
| **测量/日志** | **JMH** · Disruptor 异步日志 · **无 String 热点** |

**硬核 Java 之后** → [chapter-14 Python 混合（原书 Ch10）](./chapter-14-python-高性能混合架构.md) · 策略：[chapter-13](./chapter-13-高频做市与套利策略.md)

---

## 原书章节对照

| 原书 | 本仓库 |
|------|--------|
| Ch9 §1 GC | **本章 §1** |
| Ch9 §2 JIT/预热 | **本章 §2** |
| Ch9 §3 线程/绑核 | **本章 §3** · Ch5 |
| Ch9 §4 Disruptor | **本章 §4** · Ch7 §5 |
| Ch9 §5 JMH/日志 | **本章 §5** · Ch10 |
| Ch10 Python | **Ch14** |
| 做市/套利（本仓库扩展） | **Ch13** |

---

## Java 热点速查（Do / Don't）

| Do | Don't |
|----|-------|
| **对象池** · primitive 数组 | 热点 `new` · autoboxing |
| **Disruptor** 进程内 IPC | `synchronized` 热路径 |
| **JMH** 微基准 | 手写 nanoTime 循环 |
| **绑核 + 预热** | 依赖默认 GC 与冷 JIT |
| **异步二进制日志** | 热点 `String` 拼接 |
