# Compilers & LLVM Learning（编译器与 LLVM 学习）

> 仓库编号 **06** · 前置专题：[05-Async-Concurrency-Network](../05-Async-Concurrency-Network/README.md) · 建议先读 [07 WebAssembly](../07-Programming-WebAssembly-with-Rust/学习路径与知识链.md) Part I（栈式 VM）  
> Rust 主线：**`00-Book` → `02-RFR` → `01-ER` → `03-DeepRustStdLib` → `04-Rust-Nomicon` → `05-Async` → `07-Wasm` → 本目录**

---

## 我的选用（已确认封面）

| 阶段 | 做什么 | 书目 / 入口 |
|------|--------|-------------|
| **立刻、零成本** | 前端直觉：词法 / 语法 / 解释器 | [**Crafting Interpreters**](./01_Crafting-Interpreters/README.md) · 中文在线 |
| **纸质入门** | 从零做真编译器 + 链接 / 加载全链路 | [**《自制编译器》**](./03_Build-Your-Own-Compiler/README.md)（青木峰郎） |
| **对照 Rust** | IR、优化、与 codegen 对齐 | [**04_Learn-LLVM-17**](./04_Learn-LLVM-17/README.md) · 可与 RFR **第 2、9、10 章** 并行 |
| **以后深入** | SSA、优化、代码生成、读 LLVM Pass | [**《编译器工程》**](./02_Compiler-Principles/README.md) · Cooper/Torczon **第三版**（橡书） |

```text
04 实战：atomic → async_tokio → rust_network
  + 姊妹仓 cpp-learning-notes 01～06（开 Learn LLVM 前必修）
  → 05：01 Crafting Interpreters 中文在线（免费）
  → 买一本：03 《自制编译器》
  → 04 Learn LLVM 17 + RFR 第 2 章（Rust 代码反查 IR）
以后：02 Engineering a Compiler 3e（橡书）+ LLVM Pass / O0 vs O3
```

> **命名**：口头「编译器工程」= Cooper *Engineering a Compiler*（**橡书**），不是 Muchnick **鲸书**《高级编译器设计与实现》。

---

## 目录（四本书）

| # | 目录 | 当前选用 | 状态 |
|---|------|----------|------|
| **1** | [01_Crafting-Interpreters](./01_Crafting-Interpreters/) | *Crafting Interpreters* · **中文在线** | 路线已定 · 笔记待整理 |
| **2** | [02_Compiler-Principles](./02_Compiler-Principles/) | **《编译器工程》** Cooper/Torczon **3e** | **以后** · 占位 |
| **3** | [03_Build-Your-Own-Compiler](./03_Build-Your-Own-Compiler/) | **《自制编译器》**（青木峰郎） | 目录已建 · 逐章笔记待读 |
| **4** | [04_Learn-LLVM-17](./04_Learn-LLVM-17/) | *Learn LLVM 17* · `llvm_insight_lab` | **已有** 笔记 + `ir_samples` |

---

## 与 Rust 笔记的衔接

| 编译器专题 | Rust 主线 |
|------------|-----------|
| 前端 / AST / 解释器 | `00-Book` 语法 · RFR 类型与分发 |
| 中间表示、优化 | **04_Learn-LLVM-17** ↔ RFR **第 2、9、10 章** · [`02-RFR/学习路径与章节对照.md`](../02-RFR/学习路径与章节对照.md) |
| Wasm 栈式 VM / WAT | [07 Part I](../07-Programming-WebAssembly-with-Rust/chapter01_wasm_fundamentals/README.md) ↔ **01 Crafting Interpreters VM** | 两种「可读 IR」并排 |
| IR 对照素材 | [`05-Async-Concurrency-Network/`](../05-Async-Concurrency-Network/README.md) 三书 demo → `ir_samples/` |
| unsafe / 内存布局 | `04-Rust-Nomicon` |

LLVM 可与 RFR **第 2 章**（布局、分发）**并行**精读；**原子 / async IR** 建议在 **05 专题** 有代码后再做 diff。

## 开 Learn LLVM 前的 C++ 前置（必修）

**LLVM 本体用 C++ 实现**；书里讲的 IR、类型系统、Pass、STL 式容器，默认读者已有 **C++ 语法 + 现代特性 + STL** 底子。  
本仓库 **05 / `04_Learn-LLVM-17`** 的实验虽用 **Rust** 导出 `.ll`，但**读** LLVM 设计与《Learn LLVM 17》仍建议先补 C++。

**姊妹仓（外部，同一维护者）**：[cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes)  
**进入 `04_Learn-LLVM-17/` 之前，请在该仓至少通读 `01`～`06`：**

| # | 目录 | 书名 | 为何 LLVM 需要 |
|---|------|------|----------------|
| **01** | [`01-C++Primer`](https://github.com/cshonor/cpp-learning-notes/tree/main/01-C%2B%2BPrimer) | C++ Primer | 语法、标准库；读示例不卡在指针/引用/类 |
| **02** | [`02-Effective-C++`](https://github.com/cshonor/cpp-learning-notes/tree/main/02-Effective-C%2B%2B) | Effective C++ | 资源管理、三/五/零法则 → 理解 IR 里的 ctor/dtor |
| **03** | [`03-More-Effective-C++`](https://github.com/cshonor/cpp-learning-notes/tree/main/03-More-Effective-C%2B%2B) | More Effective C++ | 进阶惯用法 |
| **04** | [`04-Effective-Modern-C++`](https://github.com/cshonor/cpp-learning-notes/tree/main/04-Effective-Modern-C%2B%2B) | Effective Modern C++ | **移动、lambda、类型推导** — LLVM 代码风格 |
| **06** | [`05-Effective-STL`](https://github.com/cshonor/cpp-learning-notes/tree/main/05-Effective-STL) | Effective STL | 容器/迭代器 — 对照 LLVM ADT 与 Pass 遍历 |
| **06** | [`06-STL-Source-Analysis`](https://github.com/cshonor/cpp-learning-notes/tree/main/06-STL-Source-Analysis) | STL 源码剖析 |  vector/list/算法实现 — 读 IR 与优化直觉 |

**`07`～`09` 不挡 LLVM 入门**（对象模型、并发、C++20），可与 Rust **`04`** 并行；见 [04_Learn-LLVM-17 学习取舍](./04_Learn-LLVM-17/Learn-LLVM-17-学习取舍.md)。

### 推荐总顺序（Rust 仓 + C++ 仓）

```text
本仓库：00-Book → RFR → ER → StdLib → Nomicon → 05(01-atomic → 02-async_tokio → 03-network) → 07 Wasm Part I
姊妹仓：cpp-learning-notes 01～06（与 04 后期可并行，但须在 Learn LLVM 17 之前完成）
  ↓
06：01 Crafting Interpreters → 03 自制编译器 → 04 Learn LLVM 17（Rust emit IR）
以后：02 编译器工程（橡书）+ 若读 LLVM C++ 源码再补 cpp 07～09
```

> **分工**：C++ **01～06** = 读懂 LLVM **设计与 API 语境**；Rust **04 + llvm_insight_lab** = **产出并对照 IR**，不要求在本仓写 C++ Pass。
