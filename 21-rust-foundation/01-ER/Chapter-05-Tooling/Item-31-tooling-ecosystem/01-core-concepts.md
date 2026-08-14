# Item 31 · 核心知识点

← [Item 31 目录](./README.md)

### 工具生态

- 现代语言 = 编译器 + **维护与质量**配套基础设施。

### 基础环境

| 工具 | 作用 |
|------|------|
| **`cargo`** | 依赖、构建、测试、doc… |
| **`rustup`** | 安装 / 切换 toolchain |
| **rust-analyzer** | IDE 补全、跳转、诊断 |
| **[Rust Playground](https://play.rust-lang.org/)** | 在线试语法、分享片段 |

### 内置 Cargo 子命令（常用）

| 命令 | 作用 |
|------|------|
| `cargo fmt` | 格式化 |
| `cargo check` | 只类型检查，不产二进制（快） |
| `cargo clippy` | Lint（Item 29） |
| `cargo doc` | API 文档（Item 27） |
| `cargo bench` | 基准（Item 30） |
| `cargo update` | 升级 lock 内依赖 |
| `cargo tree` | 依赖树（Item 25） |
| `cargo metadata` | 工作空间 / 依赖 **JSON 元数据** |

---
