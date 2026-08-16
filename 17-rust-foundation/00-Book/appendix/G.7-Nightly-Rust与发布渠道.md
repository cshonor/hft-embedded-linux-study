# 附录 G：Rust 如何构建与 “Nightly Rust”

> **The Book** · [Appendix G（英文）](https://doc.rust-lang.org/book/appendix-07-nightly-rust.html) · [附录索引](./README.md)  
> **扩展**（Edition vs Channel、CI 踩坑）：仓库根 [README § Rust 工具链](../../README.md#rust-工具链stablenightly--edition)

---

## 1. 设计目标：Stability Without Stagnation

- **稳定**：升级 Stable 不应轻易破坏已有代码。
- **不停滞**：仍要在 `main` 上试验新特性，成熟后再进入 Stable。
- 原则：**不必害怕升级到新的 Stable**——应更少的 bug、新功能、有时更快的编译。

---

## 2. 发布渠道（Train Model）

**所有开发在 `rust-lang/rust` 的 `main` 分支**；没有 Stable/Nightly 两条长期并行开发线。

| Channel | 来源 | 典型用途 |
|---------|------|----------|
| **Nightly** | 每晚从 `main` 打包 | 试 `#![feature(...)]`、跟踪编译器前沿 |
| **Beta** | 每 6 周从 `main` 切出，功能冻结 | 多数人不日常用；CI 可测回归 |
| **Stable** | 再 6 周后从 Beta 发布 | **默认学习 / 生产** |

节奏（火车模型）：

```
nightly: * - - * - - * - - * …   （每天一版）
              ↓ 6 周
beta:         * - - - - - - - *
                        ↓ 再 6 周
stable:                  *
```

- 每 **6 周** 一版 Stable；知道某一版发布日，下一版 ≈ +6 周。
- 错过一班车不必焦虑：下一班很快。
- **维护**：项目通常只支持**最新 Stable**；旧 Stable 约 6 周后 EOL。

---

## 3. 不稳定 Feature（Feature Flags）

- 新特性先合入 **`main` → Nightly**，默认**关闭**，需源码里 **`#![feature(名字)]`** 显式开启。
- **Beta / Stable**：**不能使用** feature flags——未稳定实现已从编译器移除。
- **The Book 正文（1～19 章）只教 Stable**；书中示例不加 `feature`，按 Stable 即可编译。

```rust
// 仅 Nightly + #![feature(...)] 时可用；Stable 直接报错
#![feature(某未稳定特性)]
```

---

## 4. rustup 与切换渠道

默认安装 **Stable**：

```bash
rustup toolchain install nightly
rustup toolchain list          # 查看已安装 toolchain
```

**按项目**使用 Nightly（目录内 `rustc` / `cargo` 走 nightly）：

```bash
cd ~/projects/needs-nightly
rustup override set nightly
```

与 **`rust-toolchain.toml`**、环境变量 **`RUSTUP_TOOLCHAIN`** 的关系见根 [README § 工具链](../../README.md#rust-工具链stablenightly--edition) 与 [ER-demos WORKSPACE](../../01-ER/ER-demos/WORKSPACE.md)。

---

## 5. 与正文其他章节的关系

| Book 章节 | 与本附录 |
|-----------|----------|
| [1.1 安装](../01-getting-started/1.1-安装.md) | `rustup` 安装，默认 Stable；**未展开**三渠道 |
| [1.3 Hello, Cargo!](../01-getting-started/1.3-Hello-Cargo.md) | `edition = "2021"` — **Edition ≠ Channel**，见 [E.5](./E.5-Editions.md)、根 README |
| 第 19 章 | 高级特性仍按 **Stable** 可编译写法；不稳定试验见本附录 + Nomicon |

---

## 6. RFC 与团队（了解即可）

- 语言演进走 **RFC** 流程；子团队（语言、编译器、文档等）评审。
- 实现合入 `main` 后带 **feature gate**；在 Nightly 上试用足够久，再决定是否 **stabilize** 进 Stable。

---

## 记忆卡片

| 要点 | 一句 |
|------|------|
| 开发线 | 只有 `main`；Nightly/Beta/Stable 是**打包渠道** |
| 周期 | 约 6 周 Beta → 6 周 Stable |
| 本书 | 正文 = Stable；附录 G = 何时用 Nightly |
| feature | Stable 禁止 `#![feature(...)]` |
| rustup | `override set nightly` / `rust-toolchain.toml` 按项目切换 |
