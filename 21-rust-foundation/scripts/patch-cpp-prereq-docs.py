#!/usr/bin/env python3
"""Document C++ 01-06 prerequisite (cpp-learning-notes) before Learn LLVM."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

CPP_PREREQ_05 = """---

## 开 Learn LLVM 前的 C++ 前置（必修）

**LLVM 本体用 C++ 实现**；书里讲的 IR、类型系统、Pass、STL 式容器，默认读者已有 **C++ 语法 + 现代特性 + STL** 底子。  
本仓库 **05 / `04_Learn-LLVM-17`** 的实验虽用 **Rust** 导出 `.ll`，但**读** LLVM 设计与《Learn LLVM 17》仍建议先补 C++。

**姊妹仓（外部，同一维护者）**：[cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes)  
**进入 `04_Learn-LLVM-17/` 之前，请在该仓至少通读 `01`～`06`：**

| # | 目录 | 书名 | 为何 LLVM 需要 |
|---|------|------|----------------|
| **01** | [`01-C++Primer`](https://github.com/cshonor/cpp-learning-notes/tree/main/01-C%2B%2BPrimer) | C++ Primer | 语法、标准库；读示例不卡在指针/引用/类 |
| **02** | [`02-Effective-C++`](https://github.com/cshonor/cpp-learning-notes/tree/main/02-Effective-C%2B%2B) | Effective C++ | 资源管理、三/五/零法则 → 理解 IR 里的 ctor/dtor |
| **03** | [`03-More-Effective-C++`](https://github.com/cshonor/cpp-learning-notes/tree/main/03-More-Effective-C%2B%2B) | More Effective C++ | 进阶惯用法 |
| **04** | [`04-Effective-Modern-C++`](https://github.com/cshonor/cpp-learning-notes/tree/main/04-Effective-Modern-C%2B%2B) | Effective Modern C++ | **移动、lambda、类型推导** — LLVM 代码风格 |
| **06** | [`05-Effective-STL`](https://github.com/cshonor/cpp-learning-notes/tree/main/05-Effective-STL) | Effective STL | 容器/迭代器 — 对照 LLVM ADT 与 Pass 遍历 |
| **06** | [`06-STL-Source-Analysis`](https://github.com/cshonor/cpp-learning-notes/tree/main/06-STL-Source-Analysis) | STL 源码剖析 |  vector/list/算法实现 — 读 IR 与优化直觉 |

**`07`～`09` 不挡 LLVM 入门**（对象模型、并发、C++20），可与 Rust **`04`** 并行；见 [04_Learn-LLVM-17 学习取舍](./04_Learn-LLVM-17/Learn-LLVM-17-学习取舍.md)。

### 推荐总顺序（Rust 仓 + C++ 仓）

```text
本仓库：00-Book → RFR → ER → Nomicon → 05(01-atomic → 02-async_tokio → 03-network)
姊妹仓：cpp-learning-notes 01～06（与 04 后期可并行，但须在 Learn LLVM 17 之前完成）
  ↓
05：01 Crafting Interpreters → 03 自制编译器 → 04 Learn LLVM 17（Rust emit IR）
以后：02 编译器工程（橡书）+ 若读 LLVM C++ 源码再补 cpp 07～09
```

> **分工**：C++ **01～06** = 读懂 LLVM **设计与 API 语境**；Rust **04 + llvm_insight_lab** = **产出并对照 IR**，不要求在本仓写 C++ Pass。
"""

# Replace old optional sister section in 05 README
readme05 = ROOT / "06_Compilers-and-LLVM-Learning/README.md"
t = readme05.read_text(encoding="utf-8")
marker = "## 姊妹仓库 · C++ 对照（可选）"
if marker in t:
    t = t.split(marker)[0].rstrip() + "\n\n" + CPP_PREREQ_05.strip() + "\n"
else:
    t = t.rstrip() + "\n\n" + CPP_PREREQ_05.strip() + "\n"
readme05.write_text(t, encoding="utf-8", newline="\n")
print("patched", readme05.relative_to(ROOT))

# Root README: update step 5 block
root = ROOT / "README.md"
rt = root.read_text(encoding="utf-8")
old_block = """```text
⑤ atomic → async_tokio → rust_network  →  ⑤ Compilers / Learn LLVM 17（IR 对照）
```

| 顺序 | 专题 | 入口 |
|:---:|------|------|
| **5** | 并发 / 异步 / 网络 | [`05-Async-Concurrency-Network/README.md`](05-Async-Concurrency-Network/README.md) |
| **5** | 编译器 / LLVM | [`06_Compilers-and-LLVM-Learning/README.md`](06_Compilers-and-LLVM-Learning/README.md) |"""
new_block = """```text
⑤ atomic → async_tokio → rust_network
⑥a C++ 前置：姊妹仓 cpp-learning-notes 01～06（开 LLVM 前必修，见 05 README）
⑥b Compilers / Learn LLVM 17（Rust 导出 IR 对照）
```

| 顺序 | 专题 | 入口 |
|:---:|------|------|
| **5** | 并发 / 异步 / 网络 | [`05-Async-Concurrency-Network/README.md`](05-Async-Concurrency-Network/README.md) |
| **6a** | **C++ 前置**（外部） | [cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes) **`01`～`06`** → 再开 LLVM |
| **6b** | 编译器 / LLVM | [`06_Compilers-and-LLVM-Learning/README.md`](06_Compilers-and-LLVM-Learning/README.md) |"""
if old_block in rt:
    rt = rt.replace(old_block, new_block)
    root.write_text(rt, encoding="utf-8", newline="\n")
    print("patched", root.relative_to(ROOT))

# Sister repo table in root
old_sister = """| [cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes) | C++ | Primer → Effective → 对象模型 → 并发 → C++20；与本书 **04 / 05** 可对照读（见 [`06_Compilers-and-LLVM-Learning/README.md`](06_Compilers-and-LLVM-Learning/README.md)） |"""
new_sister = """| [cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes) | C++ | **`01`～`06` 为 Learn LLVM 前置（必修）**；`07`～`09` 与 Rust **05 / 06** 并行；详见 [`06/README`](06_Compilers-and-LLVM-Learning/README.md) |"""
if old_sister in rt:
    rt = rt.replace(old_sister, new_sister)
    root.write_text(rt, encoding="utf-8", newline="\n")

# 纯阅读路线
route = ROOT / "docs/纯阅读路线.md"
rr = route.read_text(encoding="utf-8")
rr = rr.replace(
    "⑤ Compilers / Learn LLVM 17（IR 对照，按需）",
    "⑥a C++ 前置 cpp-learning-notes 01～06（必修）\n      ↓\n⑥b Compilers / Learn LLVM 17（IR 对照）",
)
rr = rr.replace(
    "| **5** | **Learn LLVM 17** | [`06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/`](../06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/README.md) | 用 ④ 的代码反查 IR / 优化 |",
    "| **6a** | **C++ 前置**（外部） | [cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes) **`01`～`06`** | LLVM 用 C++ 设计；开 LLVM 前读完 Primer → Effective 系列 → Modern → STL → STL 源码剖析 |\n| **6b** | **Learn LLVM 17** | [`06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/`](../06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/README.md) | 用 ⑤ 的 Rust 代码反查 IR / 优化（不必写 C++ Pass） |",
)
rr = rr.replace(
    "3. **06 LLVM 在 04 之后**：有真实并发/async/网络代码再导出 IR，比空读 LLVM 书高效。",
    "3. **06 LLVM 在 04 之后**：有真实并发/async/网络代码再导出 IR，比空读 LLVM 书高效。\n4. **C++ 01～06 在 Learn LLVM 17 之前（必修）**：LLVM 与教材默认 C++ 读者；姊妹仓 [cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes) 的 `01`～`06` 见 [`06/README`](../06_Compilers-and-LLVM-Learning/README.md)。",
)
rr = rr.replace(
    "RFR → ER → Nomicon → 05(01→02→03) → 06(LLVM/IR)",
    "RFR → ER → Nomicon → 05(01→02→03) → C++(cpp 01→06) → 06(LLVM/IR)",
)
route.write_text(rr, encoding="utf-8", newline="\n")
print("patched", route.relative_to(ROOT))

print("cpp prereq docs done")
