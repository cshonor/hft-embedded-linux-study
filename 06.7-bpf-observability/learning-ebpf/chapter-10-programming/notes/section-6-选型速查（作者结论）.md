# 选型速查（作者结论）

| 场景 | 选择 |
|---|---|
| 快速收集 trace 信息 | bpftrace |
| Python 快速原型、不在乎运行时编译 | BCC |
| 生产分发、跨内核版本可移植（CO-RE） | C: libbpf；Go: cilium/ebpf 或 libbpfgo；Rust: Aya |
| Rust 全栈（内核+用户态同语言、无 LLVM 依赖） | Aya |
