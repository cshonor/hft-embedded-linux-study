## 5.3 编程语言与垃圾回收

> 章节导航：[本章导读](../README.md) · 下一篇 [5.4 分析方法论](./section-5.4-性能分析方法论.md)

**本节讲什么**：三类语言执行模型的性能特征、C/C++ 编译优化级别的取舍、GC 的四个代价与对策、以及「tick 路径零分配」的工程落地。

### 要点

| # | 要点 | 一句话 |
|---|------|--------|
| 1 | 延迟可预测性：**编译型 > JIT > 解释型** | HFT 要的是尾部不是峰值 |
| 2 | GC 的本质代价是**暂停不可预测** | 不是慢，是不知道什么时候慢 |
| 3 | **分配速率决定 GC 频率** | 少分配少 GC |
| 4 | C++ 热路径也要管分配 | malloc 锁/碎片/缺页 |
| 5 | 验证「无暂停」靠**测**不靠声称 | BPF 抓 malloc/fault/sched |

---

### 一、执行方式对比

| 类型 | 例子 | 性能特征 | 延迟形态 |
|------|------|----------|---------|
| **编译型（AOT）** | C、C++、Rust | 静态优化、`-O3`/LTO/PGO | **可预测**——最坏情况由代码决定 |
| **解释型** | Python、早期 Ruby | 启动快、开发快；峰值慢 | 峰值高且稳定地慢 |
| **VM + JIT** | Java、C#、JS V8 | 预热后接近原生 | **双峰**——预热前慢 + deopt 时回退 |

**JIT 的两个坑**：
1. **预热**：前几千次调用走解释/低优化档——冷启动延迟高（[ch11.5 FaaS](../../chapter-11-cloud-computing/notes/section-11.5-其他云技术.md) 的层 2 成本）。
2. **去优化（deopt）**：分支 profile 变化（比如行情模式突变）触发已编译代码作废、回解释档——**稳态系统里突然出现的延迟台阶**，经典 JIT 陷阱。

**编译优化级别（C/C++）**：

| 级别 | 用途 | 注意 |
|------|------|------|
| `-O0` | 调试 | — |
| `-O2` | 生产默认 | 稳健 |
| `-O3` | 激进内联、向量化 | **需 benchmark**——代码膨胀可能反而 I-cache miss（[5.6](./section-5.6-常见陷阱Gotchas.md)） |
| `-flto` | 链接期跨模块优化 | 构建慢、收益中 |
| PGO | 按 profile 引导布局 | I-cache/分支预测友好——HFT 值得做 |

**HFT**：策略核心 **C++ / Rust**（[18-Rust](../../../18-rust-quant/) 零成本抽象）；研究层 Python 随便——但**不能把解释型路径放上 tick 热路径**。

### 二、垃圾回收（GC）：四个代价

自动内存管理不是免费的，代价有四个维度：

| 问题 | 表现 | 机制 | 对策 |
|------|------|------|------|
| **Stop-the-world 暂停** | **P99/P999 尖刺** | GC 根扫描/整理时冻结所有应用线程 | 低延迟 GC（ZGC/Shenandoah，亚 ms 级）；堆 sizing |
| **GC CPU 开销** | 年轻代频繁 minor GC | 分配速率高 → 回收勤 | 少短命对象、逃逸分析、栈上分配 |
| **内存膨胀** | 堆一直涨 | GC 觉得还有余量就不回收 | 容量上限 + 对象池 |
| **分配速率反馈环** | 负载越高 GC 越勤 → 有效算力下降 | 分配与回收争 CPU | off-heap、buffer 复用 |

**GC 暂停的量级参考**（JVM 系）：

| GC | 典型 pause | 对 HFT 预算（µs 级） |
|----|-----------|---------------------|
| Parallel（吞吐优先） | 数百 ms ~ 秒 | ❌ 不可用 |
| G1（默认） | 数十~数百 ms | ❌ |
| ZGC / Shenandoah（低延迟） | **<1ms（亚 ms）** | 仍超预算 10~100× |

**结论不是「Java 不能做低延迟」**：LMAX/Disruptor 一系证明 JVM 上可以做到 µs 级——但代价是**热路径零分配**（对象池 + off-heap DirectBuffer）+ 低延迟 GC + 精细调优。同样的纪律性投入放 C++ 上更省力——语言选择的本质是**你要花多少精力买回可预测性**。

### 三、tick 路径零分配（无论什么语言）

「无分配、无 GC」的工程清单（C++/Java/Rust 通用思想）：

```
启动阶段（慢没关系）：
  □ 预分配所有 order book 容器（reserve 到上限）
  □ 对象池：消息/事件/迭代器复用
  □ ring buffer 固定槽位（Disruptor 模式）
  □ 预 touch + mlock 全部热数据（[ch7](../../chapter-07-memory/)）
  □ hugepage 映射（TLB 友好，[06-linux-mm THP](../../../06-linux-mm/chapter-03-page-table-management/notes/note-透明大页THP.md)）

稳态 tick 处理（每个包）：
  □ 零 malloc/free（池化对象 checkout/checkin）
  □ 零异常抛出路径（异常 = 分配 + 栈展开不可控）
  □ 零字符串构造（定长 buffer + 视图）
  □ 零日志格式化（二进制日志 + 离线格式化）
  □ 零锁分配路径（无锁队列/seqlock；锁本身不分配但等待不可控）
```

**C++ 特有**：默认 malloc 也不是「免费」的——多线程下 ptmalloc 的 arena 锁竞争、长期运行碎片化（[06-linux-mm slab](../../../06-linux-mm/chapter-08-slab-allocator/) 讲内核侧；用户态 jemalloc/tcmalloc 缓解）；HFT 的答案是**热路径根本没有 malloc 调用**，不是换个更快的分配器。

### 四、监控与验证

| 验证项 | 工具 | 预期 |
|--------|------|------|
| GC/分配暂停 | GC log + 延迟热力图对齐（[ch2](../../chapter-02-methodologies/)） | 尖刺与 GC 时间不重合 |
| 热路径 malloc | BPF uprobe malloc 计数（[ch15](../../chapter-15-bpf/)） | 恒零 |
| 热路径缺页 | perf -e page-faults（[ch7](../../chapter-07-memory/)） | 稳态恒零 |
| 分配速率 | GC 统计 / tcmalloc counters | 平稳 |

**「声称无 GC」不算数——BPF 抓 malloc/fault/sched 时长验证**（热路径线程的非预期事件 = 谎言被拆穿）。

### 衔接

- 下一节：[5.4 分析方法论](./section-5.4-性能分析方法论.md)
- 关联：[ch7 内存](../../chapter-07-memory/)（预 touch/mlock）、[ch13 perf](../../chapter-13-perf/)（验证工具）、[ch15 BPF](../../chapter-15-bpf/)（uprobe 验证）、[06-linux-mm slab](../../../06-linux-mm/chapter-08-slab-allocator/)、[18-Rust](../../../18-rust-quant/)（零成本抽象）

---

### 常见陷阱

1. **GC 语言做 HFT 热路径不设防**——不做零分配纪律的 JVM 热路径，GC pause 直接打穿预算。
2. **C++ 不管分配**——malloc 锁竞争/碎片/缺页；热路径目标是零 malloc 而非「更快的 malloc」。
3. **不测暂停就声称无**——BPF 验证 malloc/fault/sched，热路径线程的非预期事件。
4. **-O3 无脑上**——代码膨胀 → I-cache miss 可能倒退；benchmark 决定（[5.6](./section-5.6-常见陷阱Gotchas.md)）。
5. **JIT 系统不防 deopt**——稳态延迟台阶的经典来源；关键路径考虑 AOT（GraalVM native / C++）。

<details>
<summary>自测题（点击展开）</summary>

1. HFT 热路径为什么通常用 C++ 而非 Java/Go？
   <details><summary>答</summary>GC pause 即使低延迟 GC 也亚 ms 级，超预算 10-100×；C++ 手动管理可预分配/池化——但正确表述是「同等的零分配纪律在 C++ 上更省力」。</details>
2. GC 的四个代价维度？
   <details><summary>答</summary>STW 暂停（P99 尖刺）、GC CPU（回收算力）、内存膨胀（不回收到上限）、分配速率反馈环（负载→分配→GC→算力下降）。</details>
3. tick 路径零分配的清单核心项？
   <details><summary>答</summary>启动期预分配/池化/ring buffer 固定槽 + 稳态零 malloc/零异常/零字符串构造/零日志格式化——分配在启动期一次做完。</details>
4. JIT 的 deopt 为什么是延迟陷阱？
   <details><summary>答</summary>分支 profile 变化触发已编译代码作废回解释档——稳态系统里突现的延迟台阶，难归因（看起来什么都没变）。</details>
5. 怎么验证热路径真的零分配？
   <details><summary>答</summary>BPF uprobe 挂 malloc/free 计数 + perf page-faults + sched 时长——热路径线程上任何非零计数都拆穿声称。</details>

</details>


---

← [本章导读](../README.md)
