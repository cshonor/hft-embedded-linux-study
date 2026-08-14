# §2.1.6 代码生成（Code Generation）
← [§2.1 hub](./01-the-parts-of-a-language.md) · 上一节 · [§2.1.5](./01-5-optimization.md) · 下一节 · [§2.1.7](./01-7-virtual-machine.md)

---


> **原书编号 §2.1.6** · 流水线 **后端下山**：IR → 可执行形式。

| 项目 | 说明 |
|------|------|
| **输入** | 优化后的 **IR** |
| **输出** | **机器码** 或 **字节码** |
| **核心动作** | 把中间表示**映射**到目标指令集 |
| **关键决策** | **真 CPU** vs **假想 VM**（字节码） |

```text
IR  →  Code Generator  →  机器码（ELF）  或  字节码（Chunk）
```

---

#### 例子 1 · 两条后端路线

| 路线 | 产出 | 谁执行 | 本书 / Rust |
|------|------|--------|-------------|
| **AOT 原生** | x86-64 / ARM 机器码 | **OS 加载 → CPU 直跑** | **Rust** · `rustc` + LLVM |
| **字节码 + VM** | `OP_*` 指令流 | **VM 解释**（或 JIT） | **clox** · JVM · CPython |

```text
                    ┌─→ 机器码 ─→ CPU
优化后 IR ── CodeGen ┤
                    └─→ 字节码 ─→ VM（§2.1.7）
```

---

#### 例子 2 · clox：表达式 → 字节码（接 §2.1.3）

**源码：** `1 + 2 * 3`

**CodeGen（ch17 `emit`）产出：**

```text
OP_CONSTANT 0    ; 1
OP_CONSTANT 1    ; 2
OP_CONSTANT 2    ; 3
OP_MULTIPLY
OP_ADD
OP_RETURN
```

编译器 **递归遍历语法结构**，对每个子表达式 emit，再 emit 组合 opcode — 不是 CPU 汇编，但是 **clox 的后端 codegen**。

→ [ch17 Emitting Bytecode](../../part03_clox/chapter17_compiling-expressions/03-emitting-bytecode.md)

---

#### 例子 3 · Rust / LLVM：IR → 机器码（概念）

**LLVM IR：**

```llvm
define i32 @add(i32 %a, i32 %b) {
entry:
  %sum = add i32 %a, %b
  ret i32 %sum
}
```

**Codegen 后（x86-64 概念 · 极度简化）：**

```asm
add:
    lea  eax, [rdi + rsi]   ; 或 add edi, esi 等，视 calling convention
    ret
```

**链接**：多个 `.o` + std/Tokio **已编译的机器码** → 单一 **ELF** 可执行文件。

→ [05 编译期 LLVM vs Runtime](./05-compile-time-llvm-vs-runtime.md)

---

#### 例子 4 · 原生 code gen 的代价

| 优点 | 缺点 |
|------|------|
| **最快**（无 VM 解释层） | 指令集复杂（x86 历史包袱） |
| OS 直接 `exec` | **绑定架构** — x86 二进制不能直接在 ARM 跑 |
| HFT 默认路线 | 编译器后端开发量大 → 故用 **LLVM** |

**1960s 对策**：不生成真机器码 → 生成 **p-code / bytecode**（§2.1.7 VM）。

---

#### 例子 5 · BYOC cbc 对照

```text
cbc IR  →  codegen 包  →  x86 汇编文本  →  汇编器  →  机器码
```

→ [03 BYOC · 四阶段编译](../../../03_Build-Your-Own-Compiler/chapter01_start/02-four-compiler-stages.md)

---

#### 例子 6 · 易错边界（自测用）

**1. CodeGen ≠ Runtime** — 生成代码是**编译期**；GC/async 调度是**运行期**。

**2. 字节码也是 code gen 产物** — 只是目标 ISA 是 **假想栈机**，不是 x86。

**3. `cargo build` 已含 codegen** — 不是只到 IR 为止。

**4. JIT** — 运行期再做 code gen（§2.2.4 捷径）；AOT 在 build 时完成。

---

**一句话**：代码生成 = **把 IR 翻译成能跑的形式** — 要么 CPU 机器码，要么给 VM 的字节码。

**本书对应**：clox **ch17～24**（编译各类语法到 Chunk）· **04 LLVM** 后端 · **03 BYOC** x86 发射

→ 下一节：[§2.1.7](./01-7-virtual-machine.md)

---
