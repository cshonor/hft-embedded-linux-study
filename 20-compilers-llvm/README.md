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

| # | 任务节点 | 要做出什么 | 验收标准 | 状态 |
|---|----------|-----------|----------|:---:|
| **01** | [01-frontend-pipeline](./01-frontend-pipeline/README.md) | 跑通 **词法 → 语法 → AST → IR** 最小闭环 | 对一小段 C 子集 `.c` 输出合法 `.ll`，`lli` 能跑 | 待启动 |
| **02** | [02-c-semantics-to-ir](./02-c-semantics-to-ir/README.md) | C 语义落 IR：指针 / 数组 / 结构体 / 联合体 / 函数 / 可变参数 / switch | 覆盖 C 子集全部语义，IR 与 `clang -S -emit-llvm` **逐条对照** | 待启动 |
| **03** | [03-backend-riscv](./03-backend-riscv/README.md) | 自研 RISC-V 后端：TargetMachine → DAGToDAG → AsmPrinter | 产出可汇编 `.s`，riscv 工具链能链接运行 | **冻结** |

**03 冻结理由**：后端约占课程 24 讲 / 20+ 小时，与 HFT、嵌入式（aarch64）主线无关；
`17/06/04_Learn-LLVM-17/Learn-LLVM-17-学习取舍.md` 已把同类内容（TableGen、指令选择、后端扩展）判为「**整段跳过**」。
先冻着，需要时再解冻。

---

## 资源（全部降级为 `_refs` 索引）

| 类型 | 入口 |
|------|------|
| LLVM 视频课（61 讲） | [`_refs/c-compiler-llvm-course.md`](./_refs/c-compiler-llvm-course.md) · **讲数 → 任务节点映射表** |

> **原则**：课程与书都只作 `_refs` 里的**工具索引**。学到哪讲，笔记写进对应**任务节点**目录，不按讲数建 61 个文件夹。

---

## 前置（硬门槛）

LLVM 本体是 C++，课程示例默认读者已有 **C++11/14 + STL + 现代特性（移动、lambda、类型推导）**。
姊妹仓 [cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes) 至少 `01`～`06`：

```text
01-C++Primer → 02-Effective-C++ → 03-More-Effective-C++ → 04-Effective-Modern-C++ → 05-Effective-STL → 06-STL-Source-Analysis
  ↓
20-compilers-llvm / 01-frontend-pipeline
```

---

## 与既有编译器笔记的关系（不重复建设）

| 已有 | 定位 | 与本模块 |
|------|------|----------|
| `17-rust-foundation/06/01_Crafting-Interpreters` | 前端直觉（手写解释器） | **01 任务的同构参照**，三边互证 |
| `17-rust-foundation/06/03_Build-Your-Own-Compiler` | 《自制编译器》C♭ → cbc | 同上 |
| `17-rust-foundation/06/04_Learn-LLVM-17` | Rust 导出 IR 对照实验 | **留原处不动**；本模块是它的 C++ 侧镜像 |
| `17-rust-foundation/06/02_Compiler-Principles` | 《编译器工程》（橡书）理论 | 本模块是它的**工程实践对照** |

---

## HFT / 嵌入式视角（真实收益边界）

**说清楚这门课能给你什么、不能给你什么：**

| 项 | 判断 |
|----|------|
| 能 | 彻底搞懂 **C 语义如何一步步落到 IR 再到机器码**；读懂 codegen、栈帧、调用约定 |
| 能 | 建立读汇编的底层直觉 —— 这是 HFT 延迟优化（内联、边界检查消除、panic 冷路径、LTO）的**前置能力** |
| 不能 | 不教延迟优化、cache、lock-free、网络协议 —— 这些在 `14-hft-engineering` |
| 不能 | 不教 Rust —— 它与 `17-rust-foundation` 的关系是「底层原理支撑」，不是语法 |

**性价比更高的取法**：01 + 02 做完（约 35 讲 / 30 小时）即可覆盖读 codegen 所需的全部直觉；
03 后端默认不动。省下的时间优先给 `14-hft-engineering` 与 TLPI。
