# 9. 调试与排障

| 方法 | 命令/做法 | 何时用 |
|------|-----------|--------|
| **`printf` 调试** | 在动作里 `printf("hit %d\n", pid);` | 最快确认探针是否触发 |
| **`-d`** | 打印 AST、LLVM IR | 语法/类型/编译问题 |
| **`-v`** | 打印最终 BPF 字节码指令 | 验证器拒绝、与预期不符 |
| **`-l 'pattern'`** | 列出可用探针 | 找 tracepoint 全名 |
| **`--btf`** | 使用 BTF（若可用） | 结构体字段、CO-RE 路径 |

```bash
bpftrace -dl 'kprobe:*read*'
bpftrace -dv -e 'BEGIN { @ = count(); }'
```

| 常见问题 | 排查 |
|----------|------|
| 无输出 | 过滤器太严？探针名错？用 `printf` 确认触发 |
| 验证器失败 | 减栈深度、减 map 大小、避免非法指针解引用 |
| 字段不存在 | 内核版本差异 — `bpftrace -l` + 查 tracepoint format |

→ BCC 侧调试：[Ch 4 § 调试](../../chapter-04-bcc/) · SysPerf 单行：[appendix-A](../../appendix-A-bpftrace单行命令.md)

---
