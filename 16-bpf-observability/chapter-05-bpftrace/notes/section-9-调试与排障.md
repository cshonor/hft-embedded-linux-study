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


### 常见陷阱

1. **不使用 -d 调试标志** — bpftrace -d 打印编译后的 BPF 字节码和 AST，是调试语法和逻辑问题的首选工具；不使用 -d 只能靠猜
2. **忽视 verifier 错误信息** — verifier 拒绝时输出详细日志（指令号、寄存器状态、访问偏移），这些信息是修复 BPF 程序的关键线索
3. **混淆 bpftrace 语法错误和 verifier 拒绝** — 语法错误在编译阶段暴露（bpftrace 报错），verifier 拒绝在加载阶段暴露（内核报错）；两者排查方式不同

<details>
<summary>📝 自测题（点击展开）</summary>

1. **bpftrace 调试有哪几个层次？分别用什么工具？**

   <details>
   <summary>参考答案</summary>

   (1) 语法/语义错误：直接看 bpftrace 报错信息（行号+原因）。(2) 编译结果检查：`bpftrace -d script.bt` 打印 AST + BPF 字节码。(3) Verifier 拒绝：看内核日志（`dmesg`）中的 verifier 输出。(4) 运行时行为：在脚本中加 `printf()` 或用 `bpftrace -v` verbose 模式。

   </details>

2. **bpftrace -d 标志输出什么？如何利用？**

   <details>
   <summary>参考答案</summary>

   `-d` 输出两阶段信息：(1) AST（抽象语法树）——验证语法解析是否正确、probe/filter/actions 是否符合预期；(2) BPF 字节码——看生成的指令序列，检查 Map 操作、helper 调用、寄存器使用。调试流程：先看 AST 确认语义，再看字节码确认编译结果。

   </details>

3. **verifier 拒绝时如何排查？**

   <details>
   <summary>参考答案</summary>

   (1) 看 `dmesg` 中的 verifier 日志（拒绝的指令号、寄存器状态、访问偏移）；(2) 常见原因：指针未 bounds check、栈溢出、Map 操作不匹配、循环无上界；(3) 用 `-d` 看字节码定位问题指令；(4) 简化脚本——减少复杂逻辑，逐步增加功能定位问题。

   </details>

</details>

---
