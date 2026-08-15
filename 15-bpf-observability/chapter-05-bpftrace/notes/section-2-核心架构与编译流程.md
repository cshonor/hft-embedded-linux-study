# 2. 核心架构与编译流程

```
bpftrace 脚本 (.bt 或 -e '...')
    → lex/yacc 解析语言 → AST
    → Clang 解析 C 结构体（tracepoint 参数等）
    → LLVM IR → BPF 字节码
    → bpf() 加载 + 附加探针
    → 用户态：结束时打印 @map / 实时 printf
```

| 阶段 | 组件 | 作用 |
|------|------|------|
| 前端 | lex / yacc | 解析 `probe /filter/ { actions }` |
| 类型 | Clang | 内核 struct、tracepoint 字段布局 |
| 后端 | LLVM | IR → 验证器可接受的 BPF 字节码 |
| 运行时 | 内核 BPF VM + Map | 探针触发 → 聚合或打印 |

```bash
bpftrace --version
bpftrace -e 'BEGIN { printf("hello\n"); }'
```


### 常见陷阱

1. **混淆 bpftrace 的编译阶段和 BCC 的运行时编译** — bpftrace 自己用 LLVM 把 DSL 编译为 BPF 字节码，不依赖 BCC 的 Clang 运行时；两者编译路径不同
2. **忽视 bpftrace 对 BTF 的依赖** — CO-RE 模式需要内核提供 BTF 信息；无 BTF 的旧内核只能用 kprobe args->arg0 等低级访问方式
3. **以为 bpftrace 脚本修改后可热加载** — bpftrace 脚本修改后需要重新运行（重新编译+加载）；不能修改正在运行的 BPF 程序

<details>
<summary>📝 自测题（点击展开）</summary>

1. **bpftrace 的编译流程有哪几个阶段？**

   <details>
   <summary>参考答案</summary>

   (1) 解析 DSL 语法为 AST；(2) 语义分析（类型检查、探针验证）；(3) LLVM IR 生成 → BPF 字节码；(4) 加载到内核（经 verifier 验证）；(5) JIT 编译为原生指令执行。整个过程在 bpftrace 进程内完成，不需要外部 Clang。

   </details>

2. **bpftrace 和 BCC 的编译模型有什么本质区别？**

   <details>
   <summary>参考答案</summary>

   BCC：运行时调用 Clang 编译 BPF C 源码，需要 kernel-headers + libclang。bpftrace：内置 LLVM 编译器，把 DSL 直接编译为 BPF 字节码，不需要 kernel-headers（CO-RE 模式靠 BTF）。bpftrace 启动更快、部署更轻。

   </details>

3. **bpftrace 在无 BTF 的内核上能工作吗？有什么限制？**

   <details>
   <summary>参考答案</summary>

   可以工作，但有限制：(1) 无法用 CO-RE 访问结构体成员（如 `task->pid`）；(2) 只能用 kprobe 的 `arg0`-`arg5` 位置参数（依赖内核版本和调用约定）；(3) tracepoint 仍可用（有 format 文件）。建议升级到 5.x+ 内核启用 BTF。

   </details>

</details>

---
