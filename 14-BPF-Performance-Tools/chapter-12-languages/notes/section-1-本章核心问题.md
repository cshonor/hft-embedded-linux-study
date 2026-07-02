# 1. 本章核心问题

| 语言类型 | 机器上跑的是什么 | BPF 主要手段 |
|----------|------------------|--------------|
| **编译型** C/C++/Rust/Go | ELF 机器码 | `uprobe`/`uretprobe`、**USDT** |
| **JIT** Java、Node.js | 运行时生成的代码 | **perf-map**、JVM/ V8 特殊选项 |
| **解释型** Bash、Python、Ruby | 解释器内部 C 函数 | 追 **解释器** + 理想情况 **USDT** |

```
要追语言 X
    → X 的 runtime 是什么？
    → 符号在哪（ELF / perf.map / 无）？
    → 栈 walk 需要 frame pointer 吗？
    → 探针频率是否可承受？
```

---
