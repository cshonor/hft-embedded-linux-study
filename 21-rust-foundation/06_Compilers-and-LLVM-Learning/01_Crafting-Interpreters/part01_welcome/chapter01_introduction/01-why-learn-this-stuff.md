# 第 1 章 · Welcome（Introduction） · §1.1 为什么要学这些（Why Learn This Stuff?）

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-how-the-book-is-organized.md](./02-how-the-book-is-organized.md) · 全书速记索引：[速记-大概念对照.md](../../速记-大概念对照.md)

---

### 无处不在的小型语言（Little languages are everywhere）

- 世界不只有 C / Rust / Java 等**通用语言**。
- 大型项目里常有 **DSL**：配置格式、模板、查询语言、脚本层……
- 找不到现成库时，**自己写解析器**是极其实用的技能。

#### 通用语言 vs 小型 DSL

| 类型 | 特征 | 例子 |
|------|------|------|
| **通用编程语言** | 用途广、**图灵完备**，能写任意逻辑程序 | C、Ruby、JavaScript、Rust |
| **小型 DSL** | 只服务**单一领域**，不能写完整程序、无完整流程逻辑 | CSS、配置文件、正则 |

#### Rust 生态联想

| 形态 | 谁解析 |
|------|--------|
| **`Cargo.toml`** | 配置型 DSL → **Cargo 内置**专门处理器 |
| **各类宏**（`macro_rules!` / 过程宏） | **嵌入式 DSL** → **rustc** 宏展开器（编译期处理，非独立运行时） |

**本仓库联想**：`00-Book` 宏、`build.rs`、Cargo.toml、`#[derive]` 背后都是「小语言 + 处理器」；→ [00-Book 第 7 章](../../../../00-Book/07-packages-modules/) · RFR 宏。

---

### 两种解释器路线（本书 jlox → clox 预告）

读完 Part II / III 后应能分清两种实现模型（详见 [ch14 overview](../../part03_clox/chapter14_chunks-of-bytecode/00-overview.md)）：

| | **jlox**（Part II · ch4～13） | **clox**（Part III · ch14～30） |
|---|-------------------------------|----------------------------------|
| **模型** | **语法树遍历解释器**（Tree-walk） | **字节码虚拟机**（Bytecode VM） |
| **执行** | 边遍历 AST 边执行 | 源码 → 编译为精简字节码 → VM 逐条执行 |
| **优点** | 实现简单、概念直观 | 缓存友好、冗余少、**快得多** |
| **缺点** | 反复遍历节点，额外开销大，**很慢** | 实现复杂度高 |

**一句话**：jlox 教「前端 + 直接执行 AST」；clox 教「编译到中间表示（字节码）+ 专用 VM」。

→ §1.3 第一个解释器：[03-the-first-interpreter.md](./03-the-first-interpreter.md)

---

### clox 与 LLVM（与仓库 04 轨衔接）

| 维度 | **clox** | **LLVM** |
|------|----------|----------|
| **共同点** | 都是 **源码 → 中间代码 → 执行/编译** 的分层架构 | 同左 |
| **定位** | **只为 Lox 定制**的精简专用 VM + 字节码 | **全语言通用**的大型编译基础设施 |
| **中间表示** | Lox 专属 opcode / Chunk | **通用 IR**，可接多种前端 |
| **优化 / 后端** | ch30 局部优化（教学向） | 全套优化 Pass + **多平台机器码生成** |
| **记忆** | LLVM 编译架构的 **极简入门原型** | 工业级后端与优化管线 |

```text
clox:   Source → Compiler → Bytecode → VM 解释执行
LLVM:   Source → Frontend → LLVM IR → Opt → Backend → Machine Code
```

读 **04 Learn LLVM 17** 时若概念发怵，可先对照上表：clox 已走过「前端 → 中间层 → 执行」骨架。

→ [04 Learn LLVM 17](../../../04_Learn-LLVM-17/README.md)

---

### 绝佳的编程锻炼（Languages are great exercise）

实现一门语言是对编程能力的**终极综合练习**，逼你真正会用：

| 数据结构 / 技巧 | 在解释器里典型出现 |
|-----------------|-------------------|
| 递归 | 解析、树遍历 |
| 动态数组 | Token 流、字节码 chunk |
| 树 | AST |
| 图 | 控制流、依赖（后文 clox 更多） |
| 哈希表 | 符号表、全局/局部绑定、字符串驻留 |

**与 RFR**：第 2 章 layout、第 1 章内存区域——读 clox 时会反复碰到。

---

### 还有一个原因（One more reason）

- 作者曾把写语言的人当成掌握魔法的「**巫师**」。
- 本章态度：**没有魔法**——底层只是一行行代码；做语言的人也是普通人。
- 目的：破除对编译器 / 解释器 / 「底层」的**胆怯**。

读 **04 LLVM** IR 或 **Nomicon** 时若发怵，可回到 §1.1 这句。

---

## §1.1 三句背诵

1. **通用语言写程序，DSL 写领域；Rust 里 TOML 靠 Cargo，宏靠 rustc。**
2. **jlox 走 AST 慢但好学；clox 走字节码 VM 快但难。**
3. **clox 和 LLVM 同架构不同量级——前者 Lox 专用课，后者通用编译基建。**

---

## §1.1 自测

- [ ] 能举 2 个 DSL、2 个通用语言，并说明区别
- [ ] 能各用一句话描述 jlox 与 clox 的执行路径
- [ ] 能说明 clox 字节码与 LLVM IR 在「通用性」上的差异

→ 本章其余速记：