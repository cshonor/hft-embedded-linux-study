# 第 2 章 · A Map of the Territory（领域地图） · §2.1 语言的组成部分（The Parts of a Language）

> 所属：**§2.1 The Parts of a Language** · [← 本章目录](./README.md)

← [00-overview](./00-overview.md) · 下一节 → [§2.2 捷径与替代](./02-shortcuts-and-alternate-routes.md)

---

完整 **Pipeline**（「编译之山」上的途径点）：

```text
Source Code（源码）
    ↑ 上山 · 前端
    Scanning / Lexing     → Tokens
    Parsing               → AST
    Static Analysis       → 绑定 / 类型检查 / 符号表
    Intermediate Rep (IR) → 便于优化与后端
    Optimization          → 等价改写 IR
    ↓ 下山 · 后端
    Code Generation       → 机器码或字节码
    Virtual Machine       → 可选：跨平台执行字节码
    Runtime               → GC、动态类型、内置服务等
    Machine Code / 运行结果
```

**编号说明（与原书对照）**

| 本仓库 §2.1.x | 原书 §2.1.x | 主题 |
|---------------|-------------|------|
| **01-1** | §2.1.1 | Scanning / Lexing |
| **01-2** | §2.1.2 | Parsing |
| **01-4** | §2.1.3 | Static Analysis |
| **01-3** | §2.1.4 | Intermediate Representations |
| **01-5** | §2.1.5 | Optimization |
| **01-6** | §2.1.6 | Code Generation |
| **01-7** | §2.1.7 | Virtual Machine |
| **01-8** | §2.1.8 | Runtime |

流水线顺序上 **静态分析在 IR 之前**；文件编号 **01-3 / 01-4** 按阅读批次排列，以各文件内「原书编号」为准。

---

## 子节导航

| § | 主题 | 阅读 |
|---|------|------|
| **2.1.1** | 扫描 / 词法分析 | [01-1-scanning-lexing.md](./01-1-scanning-lexing.md) |
| **2.1.2** | 解析（AST） | [01-2-parsing.md](./01-2-parsing.md) |
| **2.1.3** | 中间表示（IR） | [01-3-intermediate-representations.md](./01-3-intermediate-representations.md) |
| **2.1.4** | 静态分析 | [01-4-static-analysis.md](./01-4-static-analysis.md) |
| **2.1.5** | 优化 | [01-5-optimization.md](./01-5-optimization.md) |
| **2.1.6** | 代码生成 | [01-6-code-generation.md](./01-6-code-generation.md) |
| **2.1.7** | 虚拟机 | [01-7-virtual-machine.md](./01-7-virtual-machine.md) |
| **2.1.8** | 运行时 | [01-8-runtime.md](./01-8-runtime.md) |
| — | 速记 · 自测 |

**建议阅读顺序（按流水线）**：`01-1` → `01-2` → `01-4` → `01-3` → `01-5` → `01-6` → `01-7` → `01-8`

---

## 本书两趟实现落在哪几站

| 路线 | 上山路 | 下山路 |
|------|--------|--------|
| **jlox** | Scan → Parse → AST | **树遍历执行**（跳过 IR/VM） |
| **clox** | Scan → Parse → **编译为字节码** | **VM** + Runtime/GC |
| **Rust** | 全线前端 + LLVM IR | 机器码 + 极简 runtime |

→ 详表 [03-a-map-of-the-territory.md](./03-a-map-of-the-territory.md) · [04-rust-hft-编译流水线对照.md](./04-rust-hft-编译流水线对照.md)

---

## 延伸

- **编译期 LLVM vs 运行期 Runtime** → [05-compile-time-llvm-vs-runtime.md](./05-compile-time-llvm-vs-runtime.md)
- **Rust/HFT 分层** → [04-rust-hft-编译流水线对照.md](./04-rust-hft-编译流水线对照.md)

---

## 一句话

> **上山**：源码 → Token → AST → 语义；**下山**：IR → 优化 → 出码 →（VM）→ Runtime。

→ 下一节：[§2.2 捷径与替代方案](./02-shortcuts-and-alternate-routes.md)
