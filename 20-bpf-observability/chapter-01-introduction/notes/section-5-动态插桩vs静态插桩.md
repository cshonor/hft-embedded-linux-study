# 5. 动态插桩 vs 静态插桩

| 类型 | 机制 | 特点 |
|------|------|------|
| **动态 · kprobes** | 内核函数入口/偏移 hook | 灵活；函数名随内核版本可能变 |
| **动态 · uprobes** | 用户态二进制/库函数 hook | 需符号；可追自定义 SO |
| **静态 · Tracepoints** | 内核 **稳定** 插桩点 | ABI 稳定，首选内核事件 |
| **静态 · USDT** | 用户态 **静态定义** 探针（如 Python、MySQL、部分 C++） | 需编译时 `-fno-omit-frame-pointer` 等；零开销未启用时 |

> **不用时零开销（动态）：** probe **未 attach** 则无成本；attach 后成本取决于 **频率 ×  per-event 逻辑** — HFT 热路径上只开 **聚合 map**，避免 per-event 打印。

→ 架构细节：[chapter-02-技术背景.md](../../chapter-02-technology-background/)


### 常见陷阱

1. **优先用 kprobe 而非 tracepoint** — kprobe 依赖内核内部函数名，升级后可能消失；tracepoint 是内核开发者承诺的稳定接口，应优先使用
2. **以为 uprobes 在未 attach 时也有开销** — uprobe 未 attach 时是原始指令，attach 后才插入断点；但 attach 期间高频函数的开销是真实的，HFT 热路径慎用
3. **混淆动态插桩的「零开销」** — 动态 probe 未 attach 时零开销，但 attach 后每次命中都有 trap + BPF 程序执行成本；「零开销」仅指未使用状态

<details>
<summary>📝 自测题（点击展开）</summary>

1. **动态插桩和静态插桩的核心区别是什么？**

   <details>
   <summary>参考答案</summary>

   动态插桩（kprobe/uprobe）在任意指令地址插桩，灵活但无 ABI 保证；静态插桩（tracepoint/USDT）由开发者预埋，名称和字段格式稳定。优先级：tracepoint > kprobe，USDT > uprobe。

   </details>

2. **为什么说「能 tracepoint 就不用 kprobe」？**

   <details>
   <summary>参考答案</summary>

   Tracepoint 是内核开发者维护的稳定接口，有 format 文件描述字段，跨内核版本兼容；kprobe 依赖内部函数名和参数布局，内核升级后可能改名或改签名，导致脚本失效。

   </details>

3. **USDT 在未 attach 时的开销是什么？HFT 如何利用？**

   <details>
   <summary>参考答案</summary>

   USDT 未 attach 时通常编译为 nop 指令，开销几乎为零。HFT 应用可在关键路径（如订单接收、策略执行）预埋 USDT 探针，日常零开销，排障时用 bpftrace attach 即可获取精确时延，无需改代码重新部署。

   </details>

</details>

---
