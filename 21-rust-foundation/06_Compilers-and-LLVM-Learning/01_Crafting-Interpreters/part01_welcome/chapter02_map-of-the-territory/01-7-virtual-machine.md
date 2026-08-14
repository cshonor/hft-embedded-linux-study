# §2.1.7 虚拟机（Virtual Machine）
← [§2.1 hub](./01-the-parts-of-a-language.md) · 上一节 · [§2.1.6](./01-6-code-generation.md) · 下一节 · [§2.1.8](./01-8-runtime.md)

---


> **原书编号 §2.1.7** · 仅 **字节码路线**需要；Rust 等原生 AOT 路线**跳过**本节。

| 项目 | 说明 |
|------|------|
| **位置** | **运行期** — OS 已加载含 VM 代码的二进制 |
| **输入** | **字节码 Chunk**（§2.1.6 产出） |
| **输出** | 栈顶结果 / 副作用（print、IO） |
| **核心动作** | **Fetch–Decode–Execute** 循环，模拟假想 CPU |
| **不做** | 词法/语法分析、LLVM 优化（均属编译期） |

**为何需要 VM**

§2.1.6 若只生成 **x86 机器码** → 换 ARM 要重编整个后端。  
**Bytecode + VM** → **同一份 Chunk** 在任何实现了 clox VM 的平台上跑。

```text
编译期:  源码 → … → Chunk 字节码（磁盘上的 .lox 脚本或内嵌）
运行期:  VM.interpret(chunk)  →  fetch-decode-execute 循环
```

---

#### 例子 1 · VM 与真 CPU 的对照

| | **真 CPU** | **clox VM** |
|---|------------|-------------|
| 程序计数器 | **PC / RIP** | **`ip`**（Instruction Pointer） |
| 指令 | x86 机器码字节 | **`OP_ADD`** 等 opcode |
| 寄存器/栈 | 硬件寄存器 + 调用栈 | **软件 `Value stack`** |
| 译码执行 | 硅片电路 | C 里 **`switch(op)`** 或 computed goto |

```c
// clox run() 骨架
for (;;) {
  uint8_t instruction = *vm.ip++;
  switch (instruction) {
    case OP_CONSTANT: /* 读索引 → 常量池 → push */ break;
    case OP_ADD:      /* pop b, pop a → push a+b */ break;
    case OP_RETURN:   return INTERPRET_OK;
  }
}
```

→ [ch15 §15.1 指令执行机](../../part03_clox/chapter15_a-virtual-machine/01-an-instruction-execution-machine.md)

---

#### 例子 2 · 栈式 VM 执行 `1 + 2 * 3`（逐步）

**Chunk（接 §2.1.6）：**

```text
OP_CONSTANT 0   ; 常量池[0]=1
OP_CONSTANT 1   ; 常量池[1]=2
OP_CONSTANT 2   ; 常量池[2]=3
OP_MULTIPLY
OP_ADD
OP_RETURN
```

| 步骤 | opcode | 栈（底→顶） | 说明 |
|:----:|--------|-------------|------|
| 1 | `CONST 0` | `[1]` | push 1 |
| 2 | `CONST 1` | `[1, 2]` | push 2 |
| 3 | `CONST 2` | `[1, 2, 3]` | push 3 |
| 4 | `MULTIPLY` | `[1, 6]` | pop 3,2 → push 6 |
| 5 | `ADD` | `[7]` | pop 6,1 → push 7 |
| 6 | `RETURN` | — | 结束，返回值 7 |

**为何用栈**：二元运算只需「两个操作数在栈顶 + 一条 opcode」— 与 **JVM、Forth、RPN 计算器** 同族。

→ [ch15 §15.2 值栈](../../part03_clox/chapter15_a-virtual-machine/02-a-value-stack-manipulator.md)

---

#### 例子 3 · 反汇编：把 Chunk 当黑盒看清

**Lox 源码：**

```lox
print 1.2;
```

**disassemble 输出（概念）：**

```text
0000  1 OP_CONSTANT    0 '1.2'
0002     OP_PRINT
0003     OP_RETURN
```

| 字段 | 作用 |
|------|------|
| `0000` | **偏移** — 对应 `ip` 指向的位置 |
| `1` | **行号** — 报错定位 |
| `OP_CONSTANT 0` | opcode + 操作数（常量池索引） |

调试 VM：**先 disassemble，再单步看栈** — 与 gdb 看汇编类似。

→ [ch14 §14.4 反汇编](../../part03_clox/chapter14_chunks-of-bytecode/03-disassembling-chunks.md)

---

#### 例子 4 · 函数调用与 CallFrame（VM 变复杂）

**源码：**

```lox
fun add(a, b) {
  return a + b;
}
print add(1, 2);
```

**运行期 VM 结构（概念）：**

```text
VM
├── stack[]          操作数 + 局部变量共用窗口
├── frames[]         CallFrame 栈
│     └── frame: { chunk, ip, slot offset }
└── ip 在当前 frame 的 chunk 内推进
```

**`OP_CALL` 时**：新 `CallFrame` 入栈；参数已在栈上 → **零拷贝** 作为被调函数局部 slot；`ip` 切到函数 Chunk。

→ [ch24 Call Frames](../../part03_clox/chapter24_calling-and-closures/03-call-frames.md)

---

#### 例子 5 · 三条执行路线对照

| 路线 | 执行引擎 | 本书 / 生态 |
|------|----------|-------------|
| **树遍历** | 递归 Visitor 走 AST | **jlox** — 无 VM |
| **字节码解释** | VM `switch` 循环 | **clox** |
| **原生机器码** | CPU 直接跑 | **Rust** 默认 |
| **JIT** | 运行期把热点 bytecode → 机器码 | JVM HotSpot · V8 · LuaJIT |

```text
jlox:   AST ──Visitor──→ 结果
clox:   Chunk ──VM──→ 结果
Rust:   机器码 ──CPU──→ 结果
JVM:    .class ──解释/JIT──→ 结果
```

**JIT**（§2.2.4）：编译期仍产出 bytecode；**运行期**再 codegen — 兼顾移植与速度。

---

#### 例子 6 · VM 性能与 ch30 优化

| 瓶颈 | 原因 | clox ch30 对策 |
|------|------|----------------|
| **`switch` 分发** | 间接分支、难 inline | **computed goto**（GCC 扩展） |
| **Value 装箱** | 类型标签 + 堆分配 | **NaN boxing** |
| **哈希探测** | 全局变量/表查找 | 优化探测序列 |

**要点**：ch30 优化的是 **VM 实现**，不是 Lox 语言语义；字节码 opcode **不变**。

→ [ch30 Optimization](../../part03_clox/chapter30_optimization/README.md)

---

#### 例子 7 · 现代 VM 生态（延伸）

| VM | 「字节码」形态 | 典型宿主 |
|----|----------------|----------|
| **clox** | 自有 `OP_*` | 嵌入 C 程序 |
| **JVM** | `.class` bytecode | `java` 命令 |
| **CPython** | `.pyc` | `python` 解释器 |
| **WASM** | `.wasm` 模块 | 浏览器 / Wasmtime |
| **Lua** | Lua bytecode | `lua` / embedding |

**WASM**：假想栈机 + 沙箱 — 与 clox 同一抽象层，目标从「跑 Lox」变成「浏览器可移植」。

---

#### 例子 8 · 易错边界（自测用）

**1. VM ≠ LLVM** — VM 在 `./clox script.lox` 时跑；LLVM 在 `gcc/clox 编译` 时跑。

**2. 字节码不是机器码** — `OP_ADD` 只是约定数字；CPU 只认 x86/ARM opcode。

**3. jlox 没有 VM** — Part II 树遍历；别把所有「解释器」都叫 VM。

**4. VM 本身是用 C 写的机器码** — VM 程序被 CPU 执行，VM **再模拟** Lox 指令（两层）。

**5. Rust 没有 Lox 式 VM** — `async` 状态机是编译期变换，不是字节码解释 loop。

---

**一句话**：VM = **用 C（或 Rust）写的假 CPU**，在运行期读 Chunk、改栈、调 runtime 服务。

**本书对应**：Part III **ch14～15** 奠基 · **ch17～24** 丰富指令 · **ch30** VM 微优化

→ 下一节：[§2.1.8](./01-8-runtime.md)

---
