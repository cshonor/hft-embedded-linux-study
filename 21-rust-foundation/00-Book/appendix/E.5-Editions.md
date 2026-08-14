# 附录 E：Editions

> **The Book** · [Appendix E（英文）](https://doc.rust-lang.org/book/appendix-05-editions.html) · 详表：[Rust Edition Guide](https://doc.rust-lang.org/edition-guide/)

`Cargo.toml` 里的 **`edition`** 指定编译器用哪套 **语法 / 迁移规则** 解析 crate。

```toml
[package]
edition = "2021"   # 或 2015 / 2018 / 2024
```

---

## 1. Edition 是什么

- **Compiler 每 6 周发版**，小步加特性；约 **每 3 年** 打包一个 **Edition**，汇总已落地改动 + 更新文档与工具。
- 现有 Edition：**2015、2018、2021、2024**（新版 The Book 默认 2024 写法）。
- **未写 `edition`** 时默认 **2015**（向后兼容）。

---

## 2. 关键性质

| 性质 | 说明 |
|------|------|
| **按 crate 选择** | 每个 package 独立设置 `edition` |
| **可混链** | 2015 项目可依赖 2021 库，反之亦然 |
| **仅影响解析** | Edition 改变的是**如何解析**代码，不是 Stable/Nightly 渠道 |
| **可能不兼容** | 新 edition 可引入新关键字等；**opt-in** 后才启用 |

> **Edition ≠ Stable/Nightly** — Edition 管语法拼写；Channel 管 feature、std、编译器检查。详见根 [README § 工具链](../../README.md#rust-工具链stablenightly--edition) 与 [G.7](./G.7-Nightly-Rust与发布渠道.md)。

---

## 3. 升级 Edition

```bash
cargo fix --edition            # 自动应用部分迁移
# 然后 Cargo.toml 改 edition，再 fix / 手改剩余项
```

完整差异与步骤见 **Edition Guide**。

---

## 4. 与正文对照

| 位置 | 说明 |
|------|------|
| [1.3 Hello, Cargo!](../01-getting-started/1.3-Hello-Cargo.md) | 首次见 `edition = "2021"` |
| [附录 A `r#`](./A.1-关键字.md) | 跨 edition 关键字差异 |
| [19.5 宏](../19-advanced-features/19.5-宏.md) | 2018+ 宏 import 变化等 |

---

## 记忆卡片

| 要点 | 一句 |
|------|------|
| edition | 语法/迁移包，按 crate 配置 |
| 混链 | 不同 edition crate 可互相依赖 |
| 升级 | `cargo fix --edition` + Edition Guide |
