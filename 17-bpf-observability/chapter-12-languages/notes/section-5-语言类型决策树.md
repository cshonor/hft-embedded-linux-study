# 5. 语言类型决策树

```
编译型 (C/C++/Rust)
  ├─ 符号：debuginfo / 勿 strip
  ├─ 栈：-fno-omit-frame-pointer
  └─ 探针：USDT > uprobe >> uretprobe(Go 禁用)

JIT (Java/Node)
  ├─ perf-PID.map + jmaps（实时）
  ├─ Java：-XX:+PreserveFramePointer
  └─ 禁 ExtendedDTraceProbes 高频 method 探针

解释型 (Bash/Python)
  ├─ 追解释器内部 或 USDT
  └─ 生产优先语言自带 profiler
```

---
