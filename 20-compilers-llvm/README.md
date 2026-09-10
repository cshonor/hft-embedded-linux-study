# Compilers & LLVM（编译器与工具链）

> 仓库编号 **20** · **语言中立**模块 · 反哺 `01-c` / `04-cpp` / `07-arm` / `15-computer-architecture` / `17-rust-foundation`
> 与 Rust 仓编译器笔记的分工见文末

---

## 为什么单开一个模块

`17-rust-foundation/06_Compilers-and-LLVM-Learning` 的定位是「**Rust 导出 IR 对照**」，其 README 明文写着
「**不要求在本仓写 C++ Pass**」。

而 LLVM 课程是**纯 C++ 工程**（用 LLVM API 写 C 编译器前端 + 自研后端），挂在 Rust 仓下会造成双重错位：

| 错位 | 说明 |
|------|------|
| 语言 | 课程全程 C++；Rust 仓不写 C++ |
| 层级 | `06` 是 Rust 主线**最后一环**（内部已 4 本书 / 415 篇 md），加第 5 本 = 又回到「按书建目录」 |
| 读者 | 后端内容同时服务 ARM / 体系结构 / C 主线，不该被 Rust 仓包住 |

**本模块承接这件事**：用 C++ 做编译器工程，语言中立，成果反哺五条主线。

---

## 任务节点（目录 = 任务，不是 = 书 / = 课）

| # | 任务节点 | 讲次 | 视频时长 | 要做出什么 | 验收标准 | 状态 |
|---|----------|:----:|:--------:|-----------|----------|:---:|
| **01** | [01-frontend-pipeline](./01-frontend-pipeline/README.md) | 1, 3–11 | 7.9 h | 跑通 **词法 → 语法 → AST → IR** 最小闭环 | 对一小段 C 子集 `.c` 输出合法 `.ll`，`lli` 能跑 | 待启动 |
| **02** | [02-c-semantics-to-ir](./02-c-semantics-to-ir/README.md) | 12–37 | 23.7 h<br>（必做 16.5） | C 语义落 IR：控制流 / 指针 / 数组 / 结构体 / 函数 | 覆盖 C 子集全部语义，IR 与 `clang -S -emit-llvm` **逐条对照** | 待启动 |
| **03** | [03-backend-riscv](./03-backend-riscv/README.md) | 38–61 | 17.6 h | 自研 RISC-V 后端：TargetMachine → DAGToDAG → AsmPrinter | 产出可汇编 `.s`，riscv 工具链能链接运行 | **冻结** |

**03 冻结理由**：课程第 **45 讲**确认目标平台是 **RISC-V**，而主线是 HFT + **aarch64**（Pi 5）——
后端细节跨 target 可迁移度很低。24 讲 / 视频 17.6 h（实际 26–35 h）投入产出不划算。
且 `17/06/04_Learn-LLVM-17/Learn-LLVM-17-学习取舍.md` 已把同类内容（TableGen、指令选择、后端扩展）判为「**整段跳过**」。
先冻着，需要时再解冻。

### 真实时间账

| 路线 | 视频 | 实际投入（1.5–2×） |
|------|:----:|:------------------:|
| **01 + 02 必做**（推荐） | 24.4 h | **37–49 h** |
| 01 + 02 全做 | 31.6 h | 47–63 h |
| 全部 61 讲 | 49.2 h | 74–98 h |

---

## 资源（全部降级为 `_refs` 索引）

| 类型 | 入口 |
|------|------|
| LLVM 视频课（61 讲） | [`_refs/c-compiler-llvm-course.md`](./_refs/c-compiler-llvm-course.md) · **讲数 → 任务节点映射表** |

> **原则**：课程与书都只作 `_refs` 里的**工具索引**。学到哪讲，笔记写进对应**任务节点**目录，不按讲数建 61 个文件夹。

---

## 前置：C++ 最小子集（不是「学完 C++」）

LLVM 本体是 C++，但它用的是**非典型 C++**——禁用异常、禁用 RTTI，热路径用自研 ADT
（`SmallVector` / `StringRef` / `DenseMap`），并自研 RTTI（`isa<>` / `dyn_cast<>`）。

所以**不需要**先系统学完 C++：

- ✅ **走 [`_refs/cpp-minimum-for-llvm.md`](./_refs/cpp-minimum-for-llvm.md)**：P0 必做 10 项 + P1 建议 3 项，约 **30–40 小时**
- ✅ 笔记全在**本仓 [`04-cpp`](../04-cpp/README.md)**（751 篇真内容），**无需**去姊妹仓 `cpp-learning-notes`
- ⛔ 明确跳过：Effective STL、STL 源码剖析、C++17/20、并发 —— 对 LLVM 性价比低，理由见清单

```text
开课前 1 周：M0 核心 5 章（类 / 动态内存 / 拷贝控制 / OOP / 模板）
   ↓
刷课讲 1, 3–4（启航 + lexer + parser，不碰 C++ API，有缓冲）
   ↓
讲 5 前补：M1 现代 C++（auto / 智能指针 / 移动 / lambda）
   ↓
01-frontend-pipeline 验收（讲 11 单测）
```

> 讲 **5**（codegen）才第一次碰 `IRBuilder`，**不必等 C++ 全补完再开课**。

---

## 与 `17-rust-foundation/06` 的分工：共享 LLVM IR，方向相反

```text
17/06 · Rust 侧（消费者）              20 · C++ 侧（生产者，本模块）
Rust → AST → HIR → MIR ──┐             C → AST ──────────────┐
                          ├──→ LLVM IR ←─────────────────────┘
                          ↓                                   ↓
                    读 asm（HFT 优化）                   造机器码（03 后端·冻结）
```

| | `17-rust-foundation/06` | 本模块 |
|---|---|---|
| 视角 | **消费者**：Rust 代码怎么变成 IR | **生产者**：怎么用 LLVM API 造 IR |
| 语言 | Rust（不写 C++） | C++ |
| 关键层 | **MIR**（Rust 独有） | 无 MIR 层 |
| 产出 | 读懂 rustc 的 MIR / IR / asm | 能造一个编译器 |

> **纠正一个常见说法**：「rustc 就是 LLVM」**不准确**。rustc **默认**用 LLVM 后端，但后端**可插拔**
> （另有 Cranelift / GCC / SPIR-V / NVVM）。且 rustc 走的是 `MIR → LLVM IR`，
> **MIR 这一层是本模块这条 C 编译器路径没有的**。

### ⚠️ 前端部分与 01/03 重复 —— 请降级处理

本模块 **01 任务**（课程 3–11 讲：词法 → 语法 → AST → codegen）与
`17/06/01_Crafting-Interpreters`、`03_Build-Your-Own-Compiler` **是同一件事**，只是换成 C++ / LLVM 实现。

**所以 01 任务不是「再学一遍编译前端」，而是「把已学到的理论用 LLVM C++ API 落地」：**

| 已有 | 定位 | 与本模块 |
|------|------|----------|
| `17/06/01_Crafting-Interpreters` | 前端直觉（手写解释器） | 理论基础 —— 本模块**不重复学** |
| `17/06/03_Build-Your-Own-Compiler` | 《自制编译器》C♭ → cbc | 同上 |
| `17/06/04_Learn-LLVM-17` | Rust 导出 IR 对照实验 | **留原处不动**；本模块是它的 C++ 侧镜像 |
| `17/06/02_Compiler-Principles` | 《编译器工程》（橡书）理论 | 03 后端（冻结）的理论对照 |
| `17/06/05_rustc-pipeline` | MIR → LLVM IR → asm（新建） | **汇合点**：它看 IR 的输入，本模块看 IR 的生成 |

**结论要想清楚**：如果目标是「理解 rustc 为什么生成这样的机器码」，那门课 **ROI 有限**——
rustc 的独特性在 MIR，而本模块讲 C 编译器（无 MIR），后端又冻结。
本模块真正的独特价值只有一条：**LLVM C++ API 实操 + 造编译器的完整工程体验**。

---

## HFT / 嵌入式视角（真实收益边界）

**说清楚这门课能给你什么、不能给你什么：**

| 项 | 判断 |
|----|------|
| 能 | 彻底搞懂 **C 语义如何一步步落到 IR 再到机器码**；读懂 codegen、栈帧、调用约定 |
| 能 | 建立读汇编的底层直觉 —— 这是 HFT 延迟优化（内联、边界检查消除、panic 冷路径、LTO）的**前置能力** |
| 不能 | 不教延迟优化、cache、lock-free、网络协议 —— 这些在 `14-hft-engineering` |
| 不能 | 不教 Rust —— 它与 `17-rust-foundation` 的关系是「底层原理支撑」，不是语法 |

**性价比更高的取法**：01 全做 + 02 的**必做 15 讲**（合计 25 讲 / 视频 24.4 h，实际约 **37–49 h**）
即可覆盖读 codegen 所需的全部直觉；02 选做 11 讲可只记要点，03 后端默认不动。
省下的时间优先给 `14-hft-engineering` 与 TLPI。
