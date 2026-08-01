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

---
