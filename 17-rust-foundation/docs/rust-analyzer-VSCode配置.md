# rust-analyzer + VSCode 配置指南

**rust-analyzer** 是 Rust 官方主推的**语言服务器（LSP）**，在 VSCode、Neovim 等编辑器里提供：补全、实时诊断、跳转定义/引用、内联类型提示、`rustfmt` 格式化等。

别人问「你怎么配置 rust-analyzer」，通常指：**`settings.json` 里给 rust-analyzer 开了哪些项**，让它又快又准。

本仓库已在 **`.vscode/settings.json`** 放了一份**工作区推荐配置**（打开本仓库文件夹即生效）。

---

## 一、装对扩展

VSCode 扩展市场搜索：**rust-analyzer**（作者 **rust-lang** / 原 matklad 团队维护）。

- ✅ **rust-analyzer**（当前唯一推荐）
- ❌ 不要装已废弃的 **Rust (rust-lang.rust)**

安装后若已打开本仓库，选 **「打开文件夹」** 到 `rs study` 根目录，让 RA 识别各章 `Cargo.toml`。

---

## 二、打开用户/工作区设置

| 作用域 | 路径 | 说明 |
|--------|------|------|
| **工作区** | 本仓库 `.vscode/settings.json` | 只影响本学习仓库，可提交 git |
| **用户** | `Ctrl+,` → 右上角 `{ }` | 所有项目通用 |

修改后执行命令面板：**`Rust Analyzer: Restart Server`**，或重启 VSCode。

---

## 三、推荐配置说明（通俗版）

| 配置项 | 作用 |
|--------|------|
| `procMacro.enable` | **必开**：`#[derive]`、`serde` 等过程宏才能补全/检查 |
| `check.command: "check"` + 保存时检查 | 保存后跑 `cargo check`，红线即编译错误 |
| `inlayHints.*` | 行内显示**变量类型、参数名、链式调用**（新手友好） |
| `completion.autoimport` | 补全时自动插入 `use` |
| `editor.formatOnSave` + `rustfmt` | 保存自动 `rustfmt` 格式化 |
| `lruCapacity` | 大项目增大缓存，减轻卡顿 |
| `check.extraArgs: ["--all-targets"]` | 检查 tests/examples 等目标（略慢但更全） |

> 不同 rust-analyzer 版本里，部分键名可能从 `checkOnSave.*` 迁移到 `check.*`；以扩展设置页的「Rust Analyzer › …」为准。本仓库 `.vscode/settings.json` 会随版本做小幅更新。

---

## 四、常见问题 → 为什么要改配置

- 宏一直报错 → 检查 `procMacro.enable`
- 补全慢、跳定义失败 → 确认用**文件夹**打开含 `Cargo.toml` 的工程；大仓库可调 `lruCapacity`、排除 `target/`
- 内联提示太吵 → 单独关掉某条 `inlayHints.*.enable`
- 多 demo 仓库 → 在根目录打开整个 `rs study`，RA 会扫描各子 crate

---

## 五、非 VSCode 编辑器

- **Neovim**：`nvim-lspconfig` 的 `rust_analyzer.setup { settings = { ... } }` 里写**同结构的 JSON**
- **IntelliJ / CLion**：Rust 插件设置里勾选对应能力（宏、保存时检查、inlay hints）

---

## 六、与本学习仓库配合

```bash
# 在仓库根目录（含多个 chapter demo）
code "c:\Users\12392\Desktop\rs\rs study"
```

编辑某一章 demo 时，也可 **单独打开** 该 demo 目录（如 `00-Book/03-common-concepts/3.3-functions-demo`），减少 RA 索引范围、更快。

---

## 参考

- [rust-analyzer 手册](https://rust-analyzer.github.io/manual.html)
- [VSCode 扩展页](https://marketplace.visualstudio.com/items?itemName=rust-lang.rust-analyzer)
