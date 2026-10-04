# 5.1 bpftrace 的组件

> 底本：《BPF之巅》第 5 章 bpftrace（印刷 p137–190），5.1 节（印刷 p137–138）

## 内容详解

bpftrace 是**基于 BPF 和 BCC 的开源跟踪器**：自带许多性能工具和文档，同时提供一门**高级编程语言**，用于创建强大的单行程序和小工具。开篇示例：

```bash
# 以直方图统计 vfs_read() 的返回值（读取字节数或错误码）
bpftrace -e 'kretprobe:vfs_read { @bytes = hist(retval); }'
```

输出即 2 的幂次直方图——一行抵 BCC 数十行。

### 这一行背后发生了什么（与前章 BCC 对照）

把这条单行和 BCC 里做同一件事的工具（`biolatency.py` 之流的写法）对照，就能看清 bpftrace 的"翻译"能力：

| 你写的 | bpftrace 替你做的事 | BCC 里你要手写什么 |
|--------|---------------------|--------------------|
| `kretprobe:vfs_read` | kretprobe 挂载、trampoline 处理 | `BPF.attach_kretprobe(fn="vfs_read", ...)` |
| `retval` | kretprobe 返回值读取 helper 展开 | `PT_REGS_RC(ctx)` + BPF 重写器方言 |
| `@bytes = hist(retval)` | map 创建 + log2 分桶 + 逐桶计数 | `BPF_HISTOGRAM(dist)` + `dist.increment(...)` |
| （无） | Ctrl-C 时自动遍历 map 打印直方图 | Python 侧 `dist.print_log2_hist()` |
| （无） | 符号解析、事件循环、清理 | `trace()` 循环 + 信号处理 |

**结论**：bpftrace 不是"另一个 BPF 前端"，而是把 BCC 的**双执行域样板代码**（BPF C + Python）收编成一门声明式语言。语言越短，验证猜想的周转越快——这正是它在排障流程里的定位（假设 → 一行 → 读图 → 下一个假设）。

### 图 5-1：仓库高层结构

```
bpftrace/
├── tools/        # 工具：*.bt 脚本 + *example.txt 示例
├── man/man8/     # 每工具一份 man 8 手册
├── docs/         # 参考手册（reference_guide.md）、单行程序指引（tutorial_one_liners.md）
└── src/
    └── ast/      # 前端：lex/yacc 词法语法分析 → AST → 代码生成 LLVM IR
```

| 组件 | 内容 |
|------|------|
| `tools/*.bt` | 全部工具以 **`.bt` 后缀**命名 |
| `man/man8/*.8` | 每工具手册页 |
| `tools/*_example.txt` | 示例输出与解释 |
| `docs/reference_guide.md` | bpftrace 语言参考手册（持续更新） |
| `docs/tutorial_one_liners.md` | 新手单行程序指引 |
| `src/ast/` | 前端 lex/yacc + Clang 解析结构体；后端编译 LLVM IR → BPF |

### 找东西速查（仓库内三层目标）

| 你要找什么 | 去哪里 |
|-----------|--------|
| 现成工具（opensnoop、runqlat…） | `tools/*.bt` |
| 某工具怎么用、输出怎么读 | `man 8 <tool>` 或 `tools/*_example.txt` |
| 语法/内置变量/函数怎么写 | `docs/reference_guide.md` |
| 想从头学写单行 | `docs/tutorial_one_liners.md` |
| 想改工具/看编译过程 | `src/ast/`（见 5.16 流水线） |

### 与 BCC 的分工（本章学习目标）

- **bpftrace 适合**：临时创造单行程序和短小脚本进行观测；
- **BCC 适合**：编写复杂工具和守护进程；

分工的本质是**复杂度分界**，可展开成一张维度对照：

| 维度 | bpftrace | BCC |
|------|----------|-----|
| 语言 | 一门高级语言（类 awk/C） | 两套 API：BPF C（内核态）+ Python（用户态） |
| 单个观测的代码量 | 1 行 ~ 几十行 | 几十 ~ 几百行（双份） |
| 学习曲线 | 小时级上手 | 天级（要懂 BPF C + 验证器 + Python 绑定） |
| 依赖 | 仍需 LLVM/Clang（解析 C 结构体）、libbcc/libbpf | 同左 + Python + BCC 框架本身 |
| 表达力上限 | 无浮点、无自由循环、512B 栈 | 完整 C（方言），可写守护进程、复杂解析 |
| 部署形态 | 单二进制 + .bt 脚本（脚本即配置） | Python 工具集 |
| 典型角色 | **排障期的探针生成器** | **固化下来的观测工具/服务** |

学习目标包括：了解特性并与其他工具对比、找到工具和文档、读懂后续章节的 bpftrace 工具源码、自己写单行程序和工具、（可选）了解内部实现。

### 历史

Alastair Robertson 于 2016 年 12 月创建（业余项目）；因设计良好、与 BCC/LLVM/BPF 工具链匹配好，作者（Gregg）加入并成为代码/工具/文档主要贡献者；第一版主要特性 2018 年完成。

## HFT 关联

- bpftrace = **排障假设的快速验证器**：一行命令验证一个猜想（见 5.5 与附录 A）；
- 交易机上往往更适合带 bpftrace 而非完整 BCC（无 Python 依赖时），但注意它同样需要 root；
- 周转速度视角：HFT 排障窗口常以分钟计（盘后复盘或盘中急查），"想法→结果"的延迟直接决定一晚能验证多少个假设——bpftrace 把这个循环压到 ~30 秒，BCC 自研要 ~半小时起步。

## 陷阱

- ⚠️ 工具文件后缀是 `.bt`，运行方式 `bpftrace tool.bt` 或 `./tool.bt`（需 shebang + chmod）。
- ⚠️ "基于 BCC"指**复用 libbcc/libbpf 生态库**（插桩/加载/USDT），不是用 BCC 的 Python 框架——依赖关系见 5.16 流水线图，别把两者部署需求混为一谈（但 LLVM/Clang 依赖确实是共享的）。

<details>
<summary>自测题</summary>

1. bpftrace 与 BCC 的定位分工是什么？
   <details><summary>答案</summary>bpftrace 适合临时单行程序和短小脚本；BCC 适合复杂工具和守护进程。</details>

2. bpftrace 仓库中 tools/ 下的工具文件用什么后缀？
   <details><summary>答案</summary>`.bt`。</details>

3. `kretprobe:vfs_read { @bytes = hist(retval); }` 这一行里，map 的创建和直方图打印分别由谁完成？
   <details><summary>答案</summary>都由 bpftrace 隐式完成：map 在语义分析/代码生成阶段自动创建（见 5.16），打印在 Ctrl-C 退出时由用户态异步动作 print_map() 完成。</details>
</details>
