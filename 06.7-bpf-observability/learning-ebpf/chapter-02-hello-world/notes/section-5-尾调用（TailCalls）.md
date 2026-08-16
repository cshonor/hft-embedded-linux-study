# 尾调用（Tail Calls）

定义：调用另一个 eBPF 程序并**替换**执行上下文——类比 `execve()` 对进程的作用，成功则**永不返回**，被调程序替换调用者栈帧。

```c
long bpf_tail_call(void *ctx, struct bpf_map *prog_array_map, u32 index)
```

- `ctx`：透传上下文
- `prog_array_map`：`BPF_MAP_TYPE_PROG_ARRAY`，存一组程序 fd
- `index`：选哪个程序
- 失败（如 index 无条目）则调用者继续往下执行——天然当默认分支用

**动机**：eBPF 栈仅 **512 字节**，尾调用串函数不增长栈；还可绕过单程序指令数限制。
**限制**：最多链 **33** 个尾调用；子程序内尾调用需 JIT 支持（写书时仅 x86，ARM 6.0 加）；与 BPF-to-BPF 调用长期互斥，**5.10** 起解除。

### 示例：sys_enter raw tracepoint + 按操作码分发

```c
BPF_PROG_ARRAY(syscall, 300);
int hello(struct bpf_raw_tracepoint_args *ctx) {
   int opcode = ctx->args[1];
   syscall.call(ctx, opcode);                     // BCC 重写为 bpf_tail_call(ctx, syscall, opcode)
   bpf_trace_printk("Another syscall: %d", opcode); // 尾调用失败才走到这 = 默认消息
   return 0;
}
```

用户态往 map 里塞 fd：`prog_array[59] = exec_fn.fd`（59=execve）；高频噪音 syscall（21/22/25…）塞 `ignore_opcode`（空函数静默）；多个 entry 可指向同一程序（222-226 全指向 hello_timer）。尾调用程序类型必须与父程序一致。
