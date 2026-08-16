# 动手实验：proc-macro-workshop

> 配套 **Procedural Macros** · [← 章索引](./README.md)  
> 上游仓库：[dtolnay/proc-macro-workshop](https://github.com/dtolnay/proc-macro-workshop)（Rust Latam 2019 · David Tolnay）

**定位**：读完 [04–07](./04-types-of-procedural-macros.md) 理论后，用 **5 个渐进项目** 写 derive / attribute / function-like 过程宏。每个项目来自真实工业场景（作者称其中 3 个自己在工作中写过）。

---

## 前置（仓库 README 要求）

- 熟悉 **struct / enum / trait / impl / 泛型 / trait bounds**
- 本仓库 RFR 对照：[第 1 章](../Chapter-01-Foundations/README.md) · [第 2 章 Types](../Chapter-02-Types/README.md)
- 工具链：`syn` / `quote` · 测试用 **`trybuild`** · 调试 **`cargo expand`** → [第 13 章 tools](../Chapter-13-Rust-Ecosystem/01-tools.md)

---

## 五个项目 ↔ RFR 第 7 章

| 目录 | 宏种类 | 练什么 | RFR 笔记 |
|------|--------|--------|----------|
| **`builder/`** | **derive** `Builder` | 遍历 AST、生成代码、helper 属性 | [04 类型](./04-types-of-procedural-macros.md) · [07 如何工作](./07-how-procedural-macros-work.md) |
| **`debug/`** | **derive** `CustomDebug` | 生命周期/泛型、trait bounds 推断 | 同上 |
| **`seq/`** | **function-like** `seq!` | 自定义语法、TokenStream 底层 | [04 类函数宏](./04-types-of-procedural-macros.md) |
| **`sorted/`** | **attribute** `#[sorted]` | **编译期错误**、visitor 模式 | [07 Span/错误](./07-how-procedural-macros-work.md) |
| **`bitfield/`** | **attribute** `#[bitfield]` | 综合最难；需先完成至少 2 个其它项目 | 全书综合 |

### 推荐顺序（官方）

1. **`builder`** — 第一次写过程宏：**必做起点**
2. 任选 **`debug`**（trait bounds / Serde 类问题）· **`seq`**（自写 parser）· **`sorted`**（诊断 / visitor）
3. **`bitfield`** — 最后；代码量最大，三种宏都要会

---

## 怎么跑

```bash
git clone https://github.com/dtolnay/proc-macro-workshop.git
cd proc-macro-workshop/builder   # 或其它子目录
cargo test
```

### 工作流（官方）

1. 打开 `tests/progress.rs`，**一次 enable 一个**测试
2. 按编号顺序：`tests/01-*.rs` → …；每个测试文件顶部有**实现提示**
3. 两类测试：
   - **应编译通过** — 失败则看 compiler / runtime 输出
   - **应编译失败** — 与 `tests/*.stderr` 比对错误信息（**trybuild**）

更新期望 stderr：删旧 `.stderr` → 跑 test → 从 `wip/` 移到 `tests/`。

---

## 调试技巧（官方）

| 目的 | 做法 |
|------|------|
| 看展开结果 | 根目录 `main.rs` 粘贴用例 → **`cargo expand`** |
| 展开语法非法 | `eprintln!("TOKENS: {}", tokens);` + `cargo check` / `cargo test` |
| 看输入 AST | `eprintln!("INPUT: {:#?}", tree);` — `syn` 开 **`features = ["extra-traits"]`** |

---

## 与本仓库其它资源

| | |
|---|---|
| Book 宏 demo | [19.5-macros-demo](../../00-Book/19-advanced-features/19.5-macros-demo/) |
| RFR 声明宏 | [01–03](./01-when-to-use-declarative-macros.md) |
| 是否真的需要宏 | [06](./06-so-you-think-you-want-a-macro.md) |
| 编译代价 | [05](./05-cost-of-procedural-macros.md) |

**建议路径**：RFR `01→07` 快读 → clone workshop → **`builder` 逐测通关** → 按需 `debug` / `seq` / `sorted` → 最后 `bitfield`。
