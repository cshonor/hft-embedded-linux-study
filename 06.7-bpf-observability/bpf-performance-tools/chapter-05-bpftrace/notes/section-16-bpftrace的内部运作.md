# 5.16 bpftrace 的内部运作

> 底本：《BPF之巅》第 5 章 bpftrace（印刷 p137–190），5.16 节（印刷 p185–186）

## 内容详解

### 图 5-3：bpftrace 内部流水线

```
bpftrace program
      │
      ▼
解析器（driver: lexer / parser.yy，flex/bison 处理）
      │  parse program into AST
      ▼
抽象语法树 AST
      │  ← 跟踪点解析器 + Clang 解析器（process structs）
      ▼
语法分析器（ast/semantic_analyser.cpp：syntax checks、map creation、printf args、stackid map）
      │  add_probes → 探针名→ID
      ▼
代码生成器（ast/codegen_llvm.cpp：AST 节点→LLVM IR 调用）
      ▼
中间形式构建器（ast/irbuilder.cpp：LLVM IR、BPF 函数调用）
      │  ← libbpf / libbcc
      ▼
BPF 字节码 → bpf_load_prog() → 验证器 → 挂载的探针（bpf_attach_*、bcc_usdt_enable_probe()）
                                   （kprobes / uprobes / tracepoints / perf_events）
      ▼
perf 缓冲区：每事件输出+异步动作（printf、AsyncAction） / 异步统计 print_map()
```

### 要点

| 组件 | 职责 |
|------|------|
| **libbpf + libbcc** | bpftrace 用它们完成探针插桩、程序加载、USDT 支持（复用 BCC 生态，见 4.11） |
| **LLVM** | 把程序编译为 BPF 字节码 |
| **lex/yacc**（flex/bison） | 语言词法/语法定义 → AST |
| **跟踪点解析器 + Clang 解析器** | 对 AST 做结构体处理 |
| **语义分析器** | 检查语言元素使用，出错抛错（所以 bpftrace 报错通常很友好） |
| **codegen + irbuilder** | AST → LLVM IR → BPF 字节码 |

数据面：事件→perf 缓冲区→每事件输出（异步 printf）或异步统计（print_map 拉取映射表）。

## HFT 关联

- 理解流水线才能定位故障环节：语法错在解析/语义分析（错误消息友好）；验证器拒绝在加载（5.17 -v 可看字节码）；挂载失败查 dmesg（同 4.12.6）；
- bpftrace 复用 libbcc/libbpf——与 BCC 同源同版本约束：升级 bpftrace 时留意 BCC 库兼容性。

## 陷阱

- ⚠️ bpftrace "友好报错"主要在语义分析阶段；验证器拒绝时的错误信息依然晦涩（同 BCC）——两段排障手段不同。

<details>
<summary>自测题</summary>

1. bpftrace 的语言前端用什么工具实现？
   <details><summary>答案</summary>lex/yacc 定义语言，经 flex/bison 处理生成 AST。</details>

2. bpftrace 复用了哪些 BCC 生态库、做什么用？
   <details><summary>答案</summary>libbpf/libbcc——探针插桩、程序加载、USDT 支持。</details>
</details>
