# 函数调用

- 早期 eBPF 禁止调用 helper 之外的函数 → 只能 `static __always_inline` 强制内联（编译器把函数体复制进调用处，无跳转指令；多处调用 = 多份拷贝）
- **内核 4.16 + LLVM 6.0 起**支持 "BPF to BPF calls"（BPF 子程序）——但 BCC 不支持，libbpf 才能用（第 3 章）
- 内联副作用：内核函数被编译器内联优化后 kprobe 挂不上（第 7 章）
