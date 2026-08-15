# 11. Part II 预告（Ch 6+）

从 **第 6 章** 起进入 **「使用 BPF 工具」** — 按资源域展开：

| 章 | 主题 | HFT 关联 |
|----|------|----------|
| [Ch 6 CPU](../../chapter-06-cpus/) | `runqlat`、`profile`、调度 | 绑核、排队延迟 |
| [Ch 7 内存](../../chapter-07-memory/) | `memleak`、`slab` | 较少热路径，OOM 排查 |
| [Ch 10 网络](../../chapter-10-networking/) | `tcpretrans`、`tcpconnect` | 共置机网络栈 |

**学习顺序：** 本章语法 → **Ch 6 CPU**（与 Ch 3 清单衔接最紧）→ Ch 10 网络。


### 常见陷阱

1. **以为 bpftrace 语法学完就够了** — bpftrace 是工具，核心能力在于理解各资源域（CPU/内存/IO/网络）的观测方法论；后续章节按资源域展开工具使用
2. **忽视 Part I（理论）和 Part II（实践）的关系** — Part I 学语法和机制，Part II 按资源域学方法论；跳过 Part I 直接用 Part II 工具会知其然不知其所以然
3. **试图一次性学完所有资源域** — 6 个资源域（CPU/内存/文件系统/磁盘IO/网络/安全）各有深度，应按 HFT 优先级聚焦 CPU 和网络两个核心域

<details>
<summary>📝 自测题（点击展开）</summary>

1. **Part I（Ch1-5）和 Part II（Ch6+）的内容分别是什么？**

   <details>
   <summary>参考答案</summary>

   Part I：BPF/bpftrace 的理论基础——概念、技术背景、BCC 框架、bpftrace 语言。学完后能写 one-liner 但不知道该看什么。Part II：按资源域展开的实践——CPU(Ch6)、内存(Ch7)、文件系统(Ch8)、磁盘IO(Ch9)、网络(Ch10)等。学完后能针对具体问题选对工具和方法论。

   </details>

2. **HFT 学习者应该按什么顺序学习 Part II？**

   <details>
   <summary>参考答案</summary>

   优先级排序：(1) Ch6 CPU——延迟分析的核心（runqlat/offcputime/profile）；(2) Ch10 网络——收发路径分析（tcpretrans/tcpconnlat）；(3) Ch9 磁盘IO——如用本地存储（biolatency/biosnoop）；(4) Ch7 内存——如用大页/NUMA（memleak/kmem）；(5) Ch8 文件系统——HFT 通常最小化文件 IO。

   </details>

3. **从 bpftrace 语法到实际排障，最大的跨越是什么？**

   <details>
   <summary>参考答案</summary>

   从「知道怎么写」到「知道写什么」。语法学会了不代表知道该 attach 哪个 probe、用哪个聚合函数、看什么指标。这个跨越需要：(1) 理解各资源域的观测方法论（USE 方法等）；(2) 积累常见问题的排查路径；(3) 理解工具输出的含义和限制。

   </details>

</details>

---
