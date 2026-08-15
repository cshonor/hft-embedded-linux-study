# §2.1.3 中间表示（Intermediate Representations, IR）
← [§2.1 hub](./01-the-parts-of-a-language.md) · 上一节 · [§2.1.2](./01-2-parsing.md) · 下一节 · [§2.1.4](./01-4-static-analysis.md)

---


> **原书编号 §2.1.4**；原书 §2.1.3 静态分析见 [§2.1.4](./01-4-static-analysis.md)。

| 项目 | 说明 |
|------|------|
| **位置** | 流水线**第三步**（接在 Parsing / 语义分析之后） |
| **输入** | **AST**（或已标注类型的 AST / HIR） |
| **输出** | **IR** — 更扁平、更接近执行的中间代码 |
| **核心动作** | **Lowering** — 把语法树「降维」成便于优化与后端的表示 |
| **为何需要** | 脱离源码括号/关键字形状；多 Pass 复用；多后端共享优化 |

**AST vs IR**

| | **AST** | **IR** |
|---|---------|--------|
| 形状 | **树**（嵌套表达式节点） | 常更**线性**（指令序列、基本块、SSA） |
| 贴近 | **源语言语法** | **执行模型**（栈、寄存器、内存） |
| 优化 | 可做，但遍历树较麻烦 | **Pass 流水线**的标准输入（LLVM、clox） |
| 本书例子 | jlox 直接解释 AST | clox **Chunk 字节码** · Rust **LLVM IR** |

```text
AST（语法树）  →  Lowering / 语义分析  →  IR  →  优化 Pass  →  代码生成
```

> **工业编译器常插入一步**：AST 上或 AST→IR 途中做**类型检查、作用域解析、常量折叠**（Rust：`rustc` 还有 **MIR** 再做 borrow check）。CI 原书 §2.1 从 Parsing 直接讲 IR，此处单独点出。

---

#### 例子 1 · 同一表达式：AST → clox 字节码 IR

**源码（接 §2.1.2）：**

```lox
1 + 2 * 3
```

**AST（树）：**

```text
Binary(+)
├── Literal(1)
└── Binary(*)
    ├── Literal(2)
    └── Literal(3)
```

**clox Chunk（线性 IR · 栈式 VM）** — 编译器 **ch17** 产出：

```text
; 常量池: [0]=1  [1]=2  [2]=3
0000  OP_CONSTANT    0    ; push 1
0002  OP_CONSTANT    1    ; push 2
0004  OP_CONSTANT    2    ; push 3
0006  OP_MULTIPLY           ; pop 3,2 → push 6
0007  OP_ADD                ; pop 6,1 → push 7
0008  OP_RETURN
```

**对比**：树变**指令流**；执行顺序由栈操作隐含，不再遍历 `Binary` 节点。

→ [ch14 何为 Bytecode](../../part03_clox/chapter14_chunks-of-bytecode/01-what-is-bytecode.md) · [ch17 发射字节码](../../part03_clox/chapter17_compiling-expressions/03-emitting-bytecode.md)

---

#### 例子 2 · jlox：AST 即「够用的 IR」

jlox **不单独建**字节码 / LLVM IR：

```text
Parser → AST  →  Visitor 解释执行（ch7）
```

| 优点 | 缺点 |
|------|------|
| 实现快、调试直观 | 节点分散、缓存不友好、间接跳转多 |
| 适合教学 Part II | clox Part III 才换紧凑 IR + VM |

**结论**：AST **可以视为一种前端 IR**；工业级编译器通常还会再降一层。

---

#### 例子 3 · Rust：多层 IR（概念）

**源码：**

```rust
fn add(a: u32, b: u32) -> u32 {
    a + b
}
```

**简化流水线：**

```text
Rust 源码
  → AST（语法）
  → HIR（命名解析、类型推断后）
  → MIR（借用检查、关键优化）
  → LLVM IR（SSA，跨 crate 优化）
  → x86-64 机器码
```

**LLVM IR 片段（概念 · SSA）：**

```llvm
define i32 @add(i32 %a, i32 %b) {
entry:
  %sum = add i32 %a, %b    ; 每个 % 名只赋值一次 → SSA
  ret i32 %sum
}
```

**SSA（Static Single Assignment）**：每个「变量」只被赋值一次，便于**常量传播、死代码消除**等 Pass。

→ [04 Learn LLVM 17](../../../04_Learn-LLVM-17/README.md) · [05 编译与分发](../../../../02-RFR/Chapter-02-Types/05-compilation-dispatch.md)

---

#### 例子 4 · 常量折叠（优化发生在 IR 上）

**源码：**

```lox
print 2 + 3;
```

| 阶段 | 表示 |
|------|------|
| AST | `Print(Literal?)` 或 `Print(Binary(+, 2, 3))` |
| **优化后 IR** | 直接 `OP_CONSTANT 5`（编译期算完） |
| 未优化 | 仍 emit `OP_CONSTANT 2` · `OP_CONSTANT 3` · `OP_ADD` |

**要点**：优化 Pass 在 **IR** 上改写成更简单指令，不必回头改 AST 形状。

---

#### 例子 5 · 前端 IR vs 后端 IR

| 种类 | 贴近谁 | 本书 / Rust 例子 |
|------|--------|------------------|
| **前端 IR** | 源语言 | AST · HIR · clox 编译前的语法树 |
| **后端 IR** | 目标机器 / VM | clox **字节码 Chunk** · **LLVM IR** · BYOC **`cbc.ir`** |

```text
         前端 IR              后端 IR
jlox:    AST          →      （无，直接解释）
clox:    编译器内部树  →      Chunk 字节码
Rust:    MIR          →      LLVM IR
cbc:     ast 包       →      ir 包（见 03 BYOC ch2）
```

→ [03 BYOC · cbc 包结构](../../../03_Build-Your-Own-Compiler/chapter02_cflat-cbc/02-cbc-packages.md)

---

#### 例子 6 · IR 为「多后端」服务

```text
        ┌─→ x86-64 代码生成
LLVM IR ┼─→ ARM64 代码生成
        └─→ WebAssembly
```

同一套 **LLVM 优化 Pass** 作用于 IR，再各自 codegen — 前端（Rust）不必为每个 CPU 写优化。

clox 类比：字节码 IR 同一套，VM 在 Windows/macOS/Linux 上解释同一 Chunk（**假想栈机** = 可移植目标）。

---

#### 例子 7 · 易错边界（自测用）

**1. IR ≠ 机器码**

```text
OP_ADD 是 clox 字节码；CPU 读的是 0x48 0x01… 这类 x86 指令
```

**2. IR 仍在编译期存在**

```text
LLVM IR 在 cargo build 时生成并优化；链接后 IR 文件不在运行时读取
```
→ 对照 [05 编译期 LLVM vs Runtime](./05-compile-time-llvm-vs-runtime.md)

**3. 有 IR 不一定有 VM**

```text
Rust → LLVM IR → 机器码（无 VM）
clox → 字节码 IR → VM 解释
```

**4. AST 可以跳过显式 IR（捷径）**

```text
jlox 树遍历 = §2.2 捷径之一
```

---

**一句话**：IR 做「**换坐标系**」— 从语法树换成指令/SSA 流，方便优化 Pass 和后端 codegen；**仍属编译期**，不是 Runtime。

**本书对应**：jlox 以 **AST** 为终点 IR · clox **ch14～17**（Chunk 字节码）· **04 Learn LLVM 17**（LLVM IR / Pass）

**本仓库**：RFR [05 编译与分发](../../../../02-RFR/Chapter-02-Types/05-compilation-dispatch.md) · [03-2 OS/LLVM 内存布局](../../../../02-RFR/Chapter-01-Foundations/03-2-os-memory-layout.md)（`alloca`/heap 常出现在 LLVM IR 层）

→ 下一节：[§2.1.4](./01-4-static-analysis.md)

---

## 速记

- [ ] 从源码到 clox 结果，按顺序写出 **8 站**（Scan → … → Runtime）
- [ ] 说明 jlox 在哪一站**停止**、少走了哪几站
- [ ] 画 `1+2*3` 的 AST 与 clox 栈变化对照
- [ ] 一句话区分 **VM**、**Runtime**、**LLVM**
- [ ] 举 1 个 **编译期**优化 + 1 个 **运行期**优化（clox ch30）

---

