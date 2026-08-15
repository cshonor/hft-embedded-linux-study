# BPF to BPF 调用（libbpf 才有）

```c
static __attribute((noinline)) int get_opcode(struct bpf_raw_tracepoint_args *ctx) {
   return ctx->args[1];
}
SEC("raw_tp")
int hello(struct bpf_raw_tracepoint_args *ctx) {
   int opcode = get_opcode(ctx);
   bpf_printk("Syscall: %d", opcode);
   return 0;
}
```

字节码里 `call pc+7` 跳到子程序偏移 8 处——**真实函数调用，非内联**。函数调用要把状态压栈以便返回，而栈只有 512 字节 → **嵌套深度受限**。（`noinline` 仅为演示防编译器优化掉；生产代码让编译器自行决定。）

对照第 2 章：尾调用替换栈帧不增长栈，BPF-to-BPF 调用消耗栈但可返回——5.10 起两者可混用。
