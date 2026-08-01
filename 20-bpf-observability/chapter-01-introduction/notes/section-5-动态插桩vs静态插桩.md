# 5. 动态插桩 vs 静态插桩

| 类型 | 机制 | 特点 |
|------|------|------|
| **动态 · kprobes** | 内核函数入口/偏移 hook | 灵活；函数名随内核版本可能变 |
| **动态 · uprobes** | 用户态二进制/库函数 hook | 需符号；可追自定义 SO |
| **静态 · Tracepoints** | 内核 **稳定** 插桩点 | ABI 稳定，首选内核事件 |
| **静态 · USDT** | 用户态 **静态定义** 探针（如 Python、MySQL、部分 C++） | 需编译时 `-fno-omit-frame-pointer` 等；零开销未启用时 |

> **不用时零开销（动态）：** probe **未 attach** 则无成本；attach 后成本取决于 **频率 ×  per-event 逻辑** — HFT 热路径上只开 **聚合 map**，避免 per-event 打印。

→ 架构细节：[chapter-02-技术背景.md](../../chapter-02-technology-background/)

---
