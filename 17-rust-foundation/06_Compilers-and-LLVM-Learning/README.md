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

**LLVM 本体用 C++ 实现**；本目录 `04_Learn-LLVM-17` 的实验虽用 **Rust** 导出 `.ll`，但**读** LLVM 设计与《Learn LLVM 17》仍需 C++ 底子。

> **更正（2026-09-10）**：此处原要求跳到姊妹仓 `cpp-learning-notes` 通读 `01`～`06` —— **该指引已过时**。
> 那些笔记**早已复制进本仓 `04-cpp`**（按 M0–M5 重组，共 751 篇），见 [`04-cpp/README.md`](../../04-cpp/README.md)。
> 且 LLVM 用的是**非典型 C++**（禁用异常 / RTTI，热路径用自研 ADT），**不必**通读那 6 本。

### 最小子集（本目录只取 P0）

完整清单见 [`20-compilers-llvm/_refs/cpp-minimum-for-llvm.md`](../../20-compilers-llvm/_refs/cpp-minimum-for-llvm.md)。
**本目录只读 IR、不写 C++，取其中 P0 即可**（约 15–20 小时，且是已有笔记的复习）：

| 需要 | 本仓入口 | 为何 |
|------|----------|------|
| 类 · 拷贝控制 | [`M0/…/ch07-classes`](../../04-cpp/M0-entry-syntax/01-C%2B%2BPrimer/ch07-classes/) · [`ch13-copy-control`](../../04-cpp/M0-entry-syntax/01-C%2B%2BPrimer/ch13-copy-control/) | 理解 IR 里的 ctor / dtor |
| 模板基础 | [`M0/…/ch16-templates`](../../04-cpp/M0-entry-syntax/01-C%2B%2BPrimer/ch16-templates/) | `isa<>` / `dyn_cast<>` 的实现基础 |
| 现代 C++ | [`M1/01-Effective-Modern-C++`](../../04-cpp/M1-modern-cpp/01-Effective-Modern-C%2B%2B/) | auto / 移动 / lambda —— LLVM 代码风格 |

**可跳过**：`Effective STL`、`STL 源码剖析`（LLVM 用自研 ADT 而非 `std::` 容器）、并发、C++17/20。

### 推荐总顺序

```text
本仓库：00-Book → RFR → ER → StdLib → Nomicon → 05(01-atomic → 02-async_tokio → 03-network) → 07 Wasm Part I
C++ 前置：04-cpp 最小子集 P0（与 05 后期可并行，但须在 Learn LLVM 17 之前完成）
  ↓
06：01 Crafting Interpreters → 03 自制编译器 → 04 Learn LLVM 17（Rust emit IR）
以后：02 编译器工程（橡书）
```

> **分工**：C++ 最小子集 **P0** = 读懂 LLVM **设计与 API 语境**；Rust **04 + llvm_insight_lab** = **产出并对照 IR**，不要求在本仓写 C++ Pass（写 C++ 去 [`20-compilers-llvm`](../../20-compilers-llvm/README.md)）。

---

## 相关：C++ 侧 LLVM 课程已放到顶层 20

用 **C++ + LLVM 从零实现 C 编译器**（前端 + 自研后端）的视频课，与本目录定位冲突——本仓是 Rust 主线最后一环，且明确「不写 C++ Pass」。
该课已落到顶层模块 **[`20-compilers-llvm`](../../20-compilers-llvm/README.md)**：

| 本目录保留（Rust 侧） | 迁到 `20-compilers-llvm`（C++ 侧） |
|------------------------|-------------------------------------|
| `04_Learn-LLVM-17` Rust 导出 IR 对照 | 用 LLVM C++ API 写 C 编译器前端 |
| `01` Crafting Interpreters · `03` 自制编译器（前端直觉） | 自研后端（**冻结**） |
| `02` 《编译器工程》·橡书（理论） | — |

两处互链、不重复建目录：本目录读 IR，**20** 写编译器。
