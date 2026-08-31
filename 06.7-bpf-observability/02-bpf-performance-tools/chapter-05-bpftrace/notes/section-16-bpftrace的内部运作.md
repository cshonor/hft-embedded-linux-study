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

### 控制面 vs 数据面：一条命令的完整旅程

流水线上半段（启动期，**只跑一次**）：语言 → AST → 语义检查 → LLVM IR → BPF 字节码 → 验证器 → 挂载。下半段（运行期，**每事件跑**）：探针触发 → BPF 程序执行（过滤/聚合/压 perf 缓冲区）→ 用户态异步动作。

这解释了几个日常现象：

| 现象 | 流水线解释 |
|------|-----------|
| 语法错报错带行号、很友好 | 语义分析器（ast/semantic_analyser）在**你的语言层**检查 |
| 验证器报错晦涩难懂 | 已经编译成字节码，错误以指令/栈偏移描述——离你的源码很远 |
| printf 输出乱序/延迟 | 异步动作经 perf 缓冲区批量上送（`-B full` 时更明显） |
| Ctrl-C 才出直方图 | 聚合在内核 map；打印是退出时的 print_map() 异步动作 |
| `@x = hist(...)` 没写 map 声明也能跑 | 语义分析器隐式创建 map（见 5.1 对照表） |

### 与 4.11 BCC 九步的对照（排障环节映射）

bpftrace 与 BCC 走的是**同一条加载链路**，只是前半段的"编译"来源不同（BCC：BPF C 源码；bpftrace：自建语言）。故障定位按环节对号入座：

| 环节 | BCC（4.11） | bpftrace（本章） | 卡在这的症状 | 定位动作 |
|------|-------------|------------------|-------------|---------|
| 语言/语法检查 | Python 语法 + BPF C 编译错 | 解析器 + **语义分析器** | 报错带行号、信息友好 | 直接读错误消息改脚本 |
| 结构体解析 | Clang 编译 BPF C（头文件路径问题高发） | Clang 解析器（include 路径/BTF） | `unknown struct/field` | `-I` 指路或查 BTF |
| 编译到字节码 | LLVM（改写器方言 C） | codegen_llvm + irbuilder | 少见（内部 bug 才到这） | `-d` 看 IR |
| **验证器** | bpf_load_prog 同款 | 同款 | 晦涩错误（栈深/指针/指令数） | `-v` 看字节码+stack depth |
| 探针挂载 | attach_kprobe 等 | bpf_attach_*（libbcc/libbpf 代劳） | 挂不上、MAXPROBES 超限 | dmesg、`-l` 核对探针名 |
| 运行期数据面 | perf ring + Python 回调 | perf ring + **异步动作**（printf/print_map） | 丢事件、无输出 | ch18 丢事件排障 |

**记忆钩子：报错友好 → 问题在语言层（自己改脚本）；报错晦涩 → 问题在加载/验证层（上 -v/d 看内部）。**

## HFT 关联

- 理解流水线才能定位故障环节：语法错在解析/语义分析（错误消息友好）；验证器拒绝在加载（5.17 -v 可看字节码）；挂载失败查 dmesg（同 4.12.6）；
- bpftrace 复用 libbcc/libbpf——与 BCC 同源同版本约束：升级 bpftrace 时留意 BCC 库兼容性；
- "启动期 vs 运行期"的划分对交易机的意义：Clang 解析头文件是**启动期一次性成本**（可能秒级），运行期每事件成本只有 BPF 程序本身——挂探针前那一下卡顿不是"开销大"，是编译在跑；盘中评估工具扰动时只算运行期。

## 陷阱

- ⚠️ bpftrace "友好报错"主要在语义分析阶段；验证器拒绝时的错误信息依然晦涩（同 BCC）——两段排障手段不同。
- ⚠️ 流水线里 Clang 解析结构体依赖**运行环境的内核头/BTF**——同一脚本在开发机跑得动、在目标机报 unknown field，先查目标机头文件与 BTF 是否齐，别改脚本。

<details>
<summary>自测题</summary>

1. bpftrace 的语言前端用什么工具实现？
   <details><summary>答案</summary>lex/yacc 定义语言，经 flex/bison 处理生成 AST。</details>

2. bpftrace 复用了哪些 BCC 生态库、做什么用？
   <details><summary>答案</summary>libbpf/libbcc——探针插桩、程序加载、USDT 支持。</details>

3. 报错信息友好 vs 晦涩，分别对应流水线哪个阶段？
   <details><summary>答案</summary>友好=语义分析器（语言层，带行号）；晦涩=验证器（字节码层，以指令/栈偏移描述）。排障手段：前者读消息改脚本，后者 -v 看字节码与 stack depth。</details>

4. bpftrace 挂探针前卡了一两秒才出 "Attaching..."，这是每事件开销吗？
   <details><summary>答案</summary>不是——那是启动期一次性成本（Clang 解析结构体+编译到 BPF 字节码）；运行期每事件只有 BPF 程序本身的执行成本。</details>
</details>
