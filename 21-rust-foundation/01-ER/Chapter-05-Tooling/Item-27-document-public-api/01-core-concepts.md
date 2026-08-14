# Item 27 · 核心知识点

← [Item 27 目录](./README.md)

### 文档注释

| 语法 | 作用 |
|------|------|
| **`///`** | 说明**紧下方**的项（fn、struct…） |
| **`//!`** | 说明**包含**该注释的模块 / crate（如 `lib.rs` 顶） |

- 内容：**Markdown**。

### 约定板块（Sections）

| 板块 | 用途 |
|------|------|
| **`# Examples`** | 用法示例（可 doc test） |
| **`# Panics`** | 触发 `panic!` 的条件 |
| **`# Errors`** | `Result` 可能的错误 |
| **`# Safety`** | `unsafe` 前置条件 |

### Doc tests

- `# Examples` 里 **fenced code block** → `cargo test` 自动编译运行 → 文档与代码同步。

### 生成与托管

| 命令 / 站点 | 作用 |
|-------------|------|
| `cargo doc --open` | 本地 HTML API 文档 |
| **docs.rs** | 发布后自动构建托管 API 文档 |
| **crates.io** | 展示 crate 顶层 **README.md** |

---
