# 第 2 章 · A Map of the Territory · 编译全流程分层（上山前端 + 下山后端）

← [§2.1 流水线各站](./01-the-parts-of-a-language.md) · [§2.2 捷径与替代](./02-shortcuts-and-alternate-routes.md) · [ch1 §1.1 Rust/HFT 预告](../chapter01_introduction/01-why-learn-this-stuff.md) · [04 LLVM](../../../04_Learn-LLVM-17/README.md)

---

## 结论（先给你）

> 完整编译器/解释器 = **上山前端**（源码 → 结构化中间表达）+ **下山后端**（中间表达 → 可执行代码 + 运行时）。Rust/HFT 读这条线，是为了看懂**延迟从哪来、优化在哪做、`dyn`/`repr(C)`/`no_std` 落在哪一站**。

---

## 总览

```text
Source Code
    ↑ 上山 · 前端（与平台无关）
    Scanning / Lexing     → Tokens
    Parsing               → AST
    语义分析 / 优化        → （常在此后进入 IR）
    IR                    → 统一中间表示
    ↓ 下山 · 后端（平台相关）
    Code Generation       → 机器码 或 字节码
    Virtual Machine       → 可选（字节码路线）
    Runtime               → GC、分配、内置服务等
    Machine Code / 运行结果
```

---

## 一、上山 · 前端（源码分析，不涉及平台逻辑）

**目标**：把人类可读源码转为**结构化中间表达**，完成语法与语义校验。

### 1. Scanning / Lexing 词法扫描

逐字符切源码，输出 **Token**（关键字、标识符、字面量、符号等最小语法单元）。

```text
let x: u64 = 10;
→ let · x · : · u64 · = · 10 · ;
```

→ 本书 Part II **ch4** · clox **ch16**

### 2. Parsing 语法解析

以 Token 流为输入，构建 **AST**，描述嵌套结构（表达式、语句、函数、类型块等）。

→ 本书 Part II **ch5～6**

### 3. 语义分析 / 优化

在 AST（或前端 IR）上：

- 类型检查、作用域绑定
- **Rust**：所有权 / 生命周期 / borrow 检查
- 常量折叠、简单优化

处理完后常**转入 IR**，便于统一优化。

### 4. IR 中间表示（Intermediate Representation）

脱离源码语法、脱离具体硬件的**统一中间代码**。

| 体系 | IR 形态 |
|------|---------|
| **Rust** | **LLVM IR**（SSA） |
| **clox** | 字节码 **Chunk** / opcode 流 |
| **jlox** | 以 **AST** 为主，不单独建工业级 IR |

**优势**：多语言可共享同一套优化器 — 常量传播、循环展开、死代码消除等；**低延迟性能优化的核心环节**往往在这里。

→ [04 Learn LLVM 17](../../../04_Learn-LLVM-17/README.md) · RFR [05 编译与分发](../../../../02-RFR/Chapter-02-Types/05-compilation-dispatch.md)

---

## 二、下山 · 后端（平台生成与执行）

**目标**：把 IR 译为硬件可执行形式，对接运行时。

### 1. Code Generation 代码生成

IR → 两种常见产物：

| 产物 | 特点 | 例子 |
|------|------|------|
| **原生机器码** | 绑定 CPU，性能高 | Rust → x86-64 汇编 |
| **字节码** | 跨平台，需 VM | clox、Java、Python |

### 2. Virtual Machine 虚拟机（可选）

仅**字节码路线**需要：运行时解释或 JIT 字节码。Rust **默认原生编译，不走 VM**。

→ 本书 Part III **clox** ch14～15

### 3. Runtime 运行时支撑层

程序跑起来后的底层服务：GC、动态类型检查、堆分配、系统调用封装等。

| 语言 | Runtime 形态 |
|------|----------------|
| **Java / Lox** | GC + 动态类型等完整 runtime |
| **Rust** | **无 GC**；`alloc`、panic、极薄标准 runtime |
| **`no_std`** | 剥离大部分 runtime，手动管控内存 |

→ clox **ch26 GC** · RFR 第 1 章内存区域

### 4. 最终产物

- **原生路线**：CPU 直接执行的二进制（如 ELF）
- **字节码路线**：VM 加载 chunk 后执行

---

## 三、Rust 完整编译流程（对照上图）

```text
Rust 源码
  → Lex 词法分析
  → AST
  → 语义检查（所有权 / 生命周期 / 类型）
  → LLVM IR
  → LLVM 优化 Pass
  → x86-64（等）机器码
  → ELF 可执行文件
（无虚拟机 · 极简 runtime）
```

---

## 四、结合 Rust / HFT 的核心价值

| 主题 | 落在流水线哪一段 | 要点 |
|------|------------------|------|
| **降低延迟 / 编译优化** | **IR + 后端** | 循环向量化、常量编译期计算、内联 — 看懂流水线才写得出让 LLVM 好优化的代码 |
| **`dyn Trait` vs 静态分发** | **IR / 代码生成** | 虚表、间接调用是**后端**插入的开销；前端类型阶段已决定静态/动态多态 |
| **FFI / `repr(C)`** | **语义分析 + 代码生成** | 布局约束在语义期检查，后端生成 **C ABI** 兼容排布 |
| **`no_std`** | **Runtime 剥离** | 去掉堆分配、标准 panic 服务等，适配 HFT **杜绝内存抖动** |
| **《Crafting Interpreters》** | **全书对照** | jlox：前端 AST + 树遍历执行；clox：完整「编译到字节码 + VM + 手动内存」 |

→ [ch1 §1.1 jlox/clox/LLVM 对照](../chapter01_introduction/01-why-learn-this-stuff.md)

---

## 五、本书两趟实现落在哪几站

| 路线 | 上山路 | 下山路 | §2.1 对应 |
|------|--------|--------|-----------|
| **jlox** | Scan → Parse → AST | **树遍历执行**（跳过 IR/代码生成/VM） | §2.2.2 捷径 |
| **clox** | Scan → Parse → **编译为字节码（IR）** | **VM 执行** + Runtime/GC | §2.1.7～8 |
| **Rust** | 全线前端 + LLVM IR | 机器码 + 极简 runtime | 工业级完整版 |

→ [03-a-map-of-the-territory.md](./03-a-map-of-the-territory.md)

→ **LLVM 与 Runtime 彻底分开**：[05-compile-time-llvm-vs-runtime.md](./05-compile-time-llvm-vs-runtime.md)

---

## 三句背诵

1. **上山前端**：Token → AST → 语义 → IR，与平台无关。
2. **下山后端**：代码生成 →（可选 VM）→ Runtime → 可执行产物。
3. **Rust/HFT**：优化在 IR；`dyn`/FFI 看后端；`no_std` 砍 runtime。

---

## 自测

- [ ] 用一句话说明 jlox 在「山」的哪一站停下、clox 多走了哪几站
- [ ] 画出 Rust 从源码到 ELF 的 6～7 个阶段
- [ ] 说明 `dyn Trait` 的额外开销主要发生在流水线哪一段
