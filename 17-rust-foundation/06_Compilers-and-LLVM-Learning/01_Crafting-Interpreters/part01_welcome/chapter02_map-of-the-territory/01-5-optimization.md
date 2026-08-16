# §2.1.5 优化（Optimization）
← [§2.1 hub](./01-the-parts-of-a-language.md) · 上一节 · [§2.1.4](./01-4-static-analysis.md) · 下一节 · [§2.1.6](./01-6-code-generation.md)

---


> **原书编号 §2.1.5** · 流水线位置：已有 IR（§2.1.3）且理解语义（§2.1.4）**之后**，代码生成 **之前**。

| 项目 | 说明 |
|------|------|
| **输入** | **IR**（或带类型的 AST/HIR） |
| **输出** | 语义等价、**更高效**的 IR |
| **核心动作** | 在**不改变程序含义**前提下改写 |
| **典型 Pass** | 常量折叠、死代码消除、内联、循环展开 |

```text
IR（未优化）  →  Pass₁  →  Pass₂  →  …  →  IR（优化后）  →  §2.1.6 代码生成
```

---

#### 例子 1 · 常量折叠（原书经典）

**源码：**

```c
pennyArea = 3.14159 * (0.75 / 2) * (0.75 / 2);
```

**优化后（编译期算完）：**

```c
pennyArea = 0.4417860938;
```

**clox IR 对照** — `print 2 + 3;`：

```text
未优化:  OP_CONSTANT 2 · OP_CONSTANT 3 · OP_ADD
优化后:  OP_CONSTANT 5
```

→ 已在 [§2.1.3 例子 4](./01-3-intermediate-representations.md)

---

#### 例子 2 · 死代码消除（DCE）

**源码：**

```rust
fn foo() -> i32 {
    let dead = expensive();
    42
}
```

**优化后 IR**：若 `dead` 无副作用，`expensive()` 调用可被 **DCE** 删掉 — 用户仍得到 `42`。

---

#### 例子 3 · 内联（Inlining）

**源码：**

```rust
#[inline]
fn sq(x: i32) -> i32 { x * x }
let y = sq(5);
```

**优化后（概念）**：`let y = 5 * 5;` — 省掉 call 开销，利于后续常量折叠。  
**HFT 热路径**：静态分发 + 内联 → 接近手写 C（RFR ch2 · `impl Trait` / 单态化）。

→ [05 编译与分发](../../../../02-RFR/Chapter-02-Types/05-compilation-dispatch.md)

---

#### 例子 4 · LLVM O0 vs O3

同一 Rust 函数，`llvm-opt` 级别不同 → IR/机器码差异巨大：

| 级别 | 典型行为 |
|------|----------|
| **O0** | 少优化，便于调试，体积大 |
| **O3** | 积极内联、向量化、DCE |

→ [04 Learn LLVM 17 · ch07 IR 优化](../../../04_Learn-LLVM-17/part02_src_to_machine/chapter07_ir_optimize/README.md) · `ir_samples/optimize_compare/`

---

#### 例子 5 · 本书对优化的态度

| 路线 | 优化在哪做 |
|------|------------|
| **Lua / CPython** | 编译期 IR **较朴素**；性能靠 **runtime**（JIT、专用结构） |
| **clox 全书** | 编译期字节码 **几乎不优化**；**ch30** 才做 **VM 实现层**微优化（NaN boxing、哈希探测） |
| **Rust + LLVM** | 大量优化在 **LLVM Pass**（middle end） |

**区分**：

```text
编译期优化  = 改 IR / 机器码（本节）
运行期优化  = clox ch30 · JVM JIT · PGO
```

---

#### 例子 6 · 易错边界（自测用）

**1. 优化必须保持语义** — 不能 `2+3` _fold 成 `6` 若会改变溢出行为（语言规则约束）。

**2. 没 IR 也能做少量优化** — AST 上常量折叠；但工业级 Pass 多针对 **SSA IR**。

**3. `-C opt-level=0` 不是「没编译」** — 仍 codegen，只是 Pass 少。

---

**一句话**：优化 = **等价改写 IR**，让后面的代码生成产出更快/更小的机器码；**仍属编译期**。

**本书对应**：概念本节 · clox **ch30 Optimization**（VM 侧）· LLVM **04 ch07**

→ 下一节：[§2.1.6](./01-6-code-generation.md)

---
