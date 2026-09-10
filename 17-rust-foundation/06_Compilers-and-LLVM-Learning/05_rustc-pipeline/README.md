# 05 · rustc 流水线（MIR → LLVM IR → asm）

> **建这个目录的原因**：`01`～`04` 四本书**没有一本讲 MIR**。
> rustc 的独特性全在 `HIR → MIR → LLVM IR` 这一段，而四本书要么停在它上游（01/03 的前端），
> 要么从它下游讲起（04 的 LLVM IR）。本目录补的就是这个缺口。

---

## 定位：与 20-compilers-llvm 的分工

```text
17/06 · Rust 侧（本目录）              20-compilers-llvm · C++ 侧
Rust → AST → HIR → MIR ──┐             C → AST ─────────────┐
                          ├──→ LLVM IR ←────────────────────┘
                          ↓                                  ↓
                    读 asm（HFT 优化）                  造机器码（后端·冻结）
```

| | 本目录 | `20-compilers-llvm` |
|---|---|---|
| 视角 | **消费者**：Rust 代码怎么变成 IR | **生产者**：怎么用 LLVM API 造 IR |
| 语言 | Rust（**不写 C++**） | C++ |
| 关键层 | **MIR**（Rust 独有） | 无 MIR 层 |
| 产出 | 能读懂 rustc 的三份产物 | 能造一个编译器 |

**共享点**：两条线在 **LLVM IR** 汇合。本目录看 IR 的**输入**，`20` 看 IR 的**生成**。

> **注意**：rustc **默认**后端是 LLVM，但它**可插拔**——另有 Cranelift（debug 加速）、
> GCC/libgccjit（LLVM 不支持的嵌入式目标）、SPIR-V、NVVM。所以「rustc = LLVM」不准确。

---

## 任务：跑通「观察链路」

**要做出什么**：对同一段 Rust 代码，能导出 **MIR / LLVM IR / 汇编** 三份产物，并解释关键差异。

**验收标准**（三选三）：

1. 导出一段含 `Vec` + 索引访问的 Rust 代码的 **MIR**，指出 `drop` 出现在哪个基本块
2. 导出同一代码的 **LLVM IR**，说明边界检查（bounds check）对应的 IR 长什么样
3. 导出 **O0 与 O3 两份汇编**，指出内联、边界检查消除、panic 冷路径三处差异

---

## 产物导出命令

| 产物 | 命令 | 产物位置 |
|------|------|----------|
| **MIR**（未优化） | `cargo +nightly rustc -- -Zunpretty=mir` | 标准输出 |
| **MIR**（优化后） | `cargo +nightly rustc --release -- -Zunpretty=mir-opt` | 标准输出 |
| **LLVM IR** | `cargo rustc -- --emit=llvm-ir` | `target/debug/deps/*.ll` |
| **LLVM IR**（优化） | `cargo rustc --release -- --emit=llvm-ir` | `target/release/deps/*.ll` |
| **汇编** | `cargo rustc -- --emit=asm` | `target/debug/deps/*.s` |
| **汇编**（O3） | `cargo rustc --release -- --emit=asm -C opt-level=3` | `target/release/deps/*.s` |
| 在线对照 | [godbolt.org](https://godbolt.org)（选 Rust） | 可直切 LLVM IR / asm |

> `-Z` 开关需要 **nightly** 工具链。MIR 相关只有 `-Zunpretty=mir*`；LLVM IR 与 asm 用 **stable** 即可。

**常用组合**：

```bash
# 单文件、关掉 codegen 分块，得到干净的一份 .ll
rustc --emit=llvm-ir -C codegen-units=1 -C opt-level=3 demo.rs

# 看单态化：泛型函数会被实例化成几份
cargo +nightly rustc -- -Zunpretty=mir | grep "fn.*::"
```

---

## rustc 各阶段与四本书的对位

| 阶段 | crate | 四本书里有吗 | 本目录要看什么 |
|------|-------|:---:|------|
| 词法 / 语法 → AST | `rustc_lexer` `rustc_parse` | ✅ 01 / 03 | 略过（已会） |
| AST → HIR | `rustc_hir` | ⚠️ 部分（03 有 AST） | 名字解析、宏展开后的形态 |
| **类型检查** | `rustc_hir_analysis` | ⚠️ 部分（03 有类型检查章） | trait 求解、单态化前 |
| **HIR → MIR** | `rustc_mir_build` | ❌ **无** | **重点**：MIR 的基本块结构 |
| **借用检查** | `rustc_borrowck` | ❌ **无** | **重点**：NLL 如何在 MIR 上工作 |
| **MIR 优化** | `rustc_mir_transform` | ❌ **无** | **重点**：drop 消除、常量传播 |
| **单态化** | `rustc_middle` | ❌ **无** | **重点**：泛型实例化几份 |
| MIR → LLVM IR | `rustc_codegen_ssa` + `rustc_codegen_llvm` | ✅ 04 | LLVM IR 语法（04 已覆盖） |
| LLVM IR → 机器码 | LLVM 后端 | ✅ 04（理论）· 20（实践·冻结） | 略过 |

---

## HFT / 性能观察点（本目录的真正价值）

| 观察项 | 在哪一层看 | 关心什么 |
|--------|-----------|----------|
| **drop 时机** | MIR | `drop` 是 MIR 的显式 terminator；早 drop 能缩短临界区 |
| **单态化膨胀** | MIR / asm | 泛型实例化成 N 份 → 代码体积 → **icache 压力** |
| **边界检查** | LLVM IR / asm | `a[i]` 带检查 vs `get_unchecked`；O3 能否消除 |
| **panic 冷路径** | asm | landing pad、`cold` 属性；是否污染热路径布局 |
| **内联决策** | LLVM IR / asm | `#[inline]` / `#[inline(always)]` 的实际效果 |
| **LTO** | asm | `-C lto=fat` + `codegen-units=1` 的收益代价 |
| **布局与对齐** | LLVM IR | `repr(C)` vs 默认；结构体字段重排 |

---

## 与 `04_Learn-LLVM-17` 的关系

`04` 是 **LLVM 全景 + Rust 导出 IR 的实验**（已包含 `ir_samples/`、`optimize_compare/`）。
本目录是它的**上游补充**：`04` 从 LLVM IR 开始看，本目录补上 **IR 之前的 MIR 那一半**。

建议顺序：`04` 的 IR 实验 → 本目录回看 MIR → 再回 `04` 看 O0/O3 差异。

---

## 状态

| 项 | 状态 |
|----|:---:|
| MIR 导出与基本块解读 | 待启动 |
| 借用检查（NLL）在 MIR 上的痕迹 | 待启动 |
| 单态化与代码体积实测 | 待启动 |
| O0 vs O3 汇编对照（HFT 视角） | 待启动 |
