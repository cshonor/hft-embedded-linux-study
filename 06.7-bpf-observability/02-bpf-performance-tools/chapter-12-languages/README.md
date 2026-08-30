# Ch 12 语言 · Languages

> **BPF Performance Tools** · Brendan Gregg · 印刷 p545–619

> 本章定位：**按语言类型决定怎么挂 BPF** — 追踪前先问：底层是 **编译型 / JIT / 解释型**？符号从哪来？栈怎么 walk？参数怎么读？以 C（编译）、Java（JIT）、bash（解释）三个实例走完全程，另有 Node.js/C++/Golang 笔记。
> **HFT：** 热路径 **C/C++** 必读 **帧指针 + 符号** 两节；共置 **Java/Go 辅助服务** 按需；**Go `uretprobe` 禁用**。与 [Ch 2 栈遍历](../chapter-02-technology-background/)、[Ch 13 应用案例](../chapter-13-applications/) 衔接。
> **上一章：** [chapter-11-security/](../chapter-11-security/) · **下一章：** [chapter-13-applications/](../chapter-13-applications/)

---

## 小节笔记（按原书真实小节）

| 原书小节 | 笔记 | 内容 |
|----|------|------|
| 12.1 背景 | [section-1-背景知识](./notes/section-1-背景知识.md) | 三类语言对比 · uprobes×JIT 两障碍 · 六步策略 · 图 12-1 工具 |
| 12.2 C | [section-2-C语言](./notes/section-2-C语言.md) | .symtab/strip · objcopy 轻量符号 · 帧指针栈 · 函数/偏移跟踪 · USDT · 单行 |
| 12.3.1–6 Java 基础 | [section-3-Java-基础与符号](./notes/section-3-Java-基础与符号.md) | libjvm/jnistacks · 线程名 · perf-map-agent（60s 时效）· PreserveFramePointer · hotspot USDT 与扩展探针 |
| 12.3.7–16 Java 工具 | [section-4-Java-BPF工具族](./notes/section-4-Java-BPF工具族.md) | profile 火焰图 · offcputime · stackcount · javastat/threads/calls/flow/gc/objnew · 单行 |
| 12.4–12.5 | [section-5-bash与其他语言](./notes/section-5-bash与其他语言.md) | bash 方法论（bashfunc/bashfunclat/stripped 破局）· Node.js · C++ mangling · Go 三坑 |
| 12.6 小结 | [section-6-小结](./notes/section-6-小结.md) | 分类路径 · 稳定性阶梯 · HFT 军规 |

---

## 本章 Checklist

- [ ] **策略核心（C++/Rust）**— 构建链：**不 strip + frame pointer + debuginfo**；否则 profile 半盲。
- [ ] **USDT > 采样 > uprobe**— 高频路径预埋静态探针；uprobe 依赖版本实现。
- [ ] **Go：禁止 uretprobe**— 栈可移动致内存破坏；时长用入口/出口双 uprobe；tid 键不可靠。
- [ ] **Java/Node 辅助服务**— `PreserveFramePointer` + jmaps（符号 60s 时效）；**勿开** ExtendedDTrace 方法级探针（>10x 减速）。
- [ ] **Python/Bash**— 运维脚本层；BPF 追 bash 内部仅取证/调试。
- [ ] **C++ `this`**— 读 uprobe 参数注意 arg0 偏移与对象结构体（部分结构体法）。

---

## 相关章节

- 上一章：[chapter-11-security/](../chapter-11-security/)
- 下一章：[chapter-13-applications/](../chapter-13-applications/)
- 栈与 USDT：[chapter-02-technology-background/](../chapter-02-technology-background/)
- CPU profile：[chapter-06-cpus/](../chapter-06-cpus/)
- CSAPP 编译：[chapter-05-optimizing-performance](../../../02-computer-systems/chapter-05-optimizing-performance/)
- Rust 工程：[18-rust-quant](../../../18-rust-quant/)
