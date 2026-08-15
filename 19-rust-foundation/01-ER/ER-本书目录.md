# Effective Rust — 本书目录

> **书名**：*Effective Rust: 35 Specific Ways to Improve Your Rust Code*  
> **作者**：David Drysdale  
> **定位**：从「能写 Rust」到「写得地道（idiomatic）」；用**类型系统**在编译期表达设计、消除无效状态。  
> **在线全文**：[effective-rust.com](https://www.effective-rust.com/print.html)（亦可按章浏览 [effective-rust.com](https://effective-rust.com/)）

与仓库 **The Book 主线**（[`00-Book/Book-本书目录.md`](../00-Book/Book-本书目录.md)，`00-Book/01-*`～`00-Book/19-*`）的关系：The Book 打语法与所有权基础；本书用 **35 条建议**纠习惯与反模式，建议在过完所有权、错误处理、trait 后再读。

---

## 章节与本地笔记

| 章 | 主题 | Item | 本地目录 |
|----|------|------|----------|
| 1 | Types（类型系统） | 1–9 | [Chapter-01-Types](./Chapter-01-Types/) |
| 2 | Traits（特征） | 10–13 | [Chapter-02-Traits](./Chapter-02-Traits/) |
| 3 | Concepts（核心概念） | 14–20 | [Chapter-03-Concepts](./Chapter-03-Concepts/) |
| 4 | Dependencies（依赖与 crate） | 21–26 | [Chapter-04-Dependencies](./Chapter-04-Dependencies/) |
| 5 | Tooling（工具链） | 27–32 | [Chapter-05-Tooling](./Chapter-05-Tooling/) |
| 6 | Beyond Standard Rust | 33–35 | [Chapter-06-Beyond-Standard-Rust](./Chapter-06-Beyond-Standard-Rust/) |

---

## Item 索引（35 条）

### Chapter 1 — Types

| Item | 标题 | 笔记 |
|------|------|------|
| 1 | Use the type system to express your data structures | [Item-01](./Chapter-01-Types/Item-01-express-data-structures/README.md) |
| 2 | Use the type system to express common behavior | [Item-02](./Chapter-01-Types/Item-02-express-common-behavior/README.md) |
| 3 | Prefer Option and Result transforms over explicit match | [Item-03](./Chapter-01-Types/Item-03-option-result-transforms/README.md) |
| 4 | Prefer idiomatic Error types | [Item-04](./Chapter-01-Types/Item-04-idiomatic-error-types/README.md) |
| 5 | Understand type conversions | [Item-05](./Chapter-01-Types/Item-05-type-conversions/README.md) |
| 6 | Embrace the newtype pattern | [Item-06](./Chapter-01-Types/Item-06-newtype-pattern/README.md) |
| 7 | Use builders for complex types | [Item-07](./Chapter-01-Types/Item-07-builders/README.md) |
| 8 | Familiarize yourself with reference and pointer types | [Item-08](./Chapter-01-Types/Item-08-references-pointers/README.md) |
| 9 | Consider iterator transforms instead of explicit loops | [Item-09](./Chapter-01-Types/Item-09-iterator-transforms/README.md) |

### Chapter 2 — Traits

| Item | 标题 | 笔记 |
|------|------|------|
| 10 | Familiarize yourself with standard traits | [Item-10](./Chapter-02-Traits/Item-10-standard-traits/README.md) |
| 11 | Implement the Drop trait for RAII patterns | [Item-11](./Chapter-02-Traits/Item-11-drop-raii/README.md) |
| 12 | Understand generics vs trait objects trade-offs | [Item-12](./Chapter-02-Traits/Item-12-generics-vs-trait-objects/README.md) |
| 13 | Use default implementations to minimize trait methods | [Item-13](./Chapter-02-Traits/Item-13-default-implementations/README.md) |

### Chapter 3 — Concepts

| Item | 标题 | 笔记 |
|------|------|------|
| 14 | Understand lifetimes | [Item-14](./Chapter-03-Concepts/Item-14-lifetimes/README.md) |
| 15 | Understand the borrow checker | [Item-15](./Chapter-03-Concepts/Item-15-borrow-checker/README.md) |
| 16 | Avoid writing unsafe code | [Item-16](./Chapter-03-Concepts/Item-16-avoid-unsafe/README.md) |
| 17 | Be wary of shared-state parallelism | [Item-17](./Chapter-03-Concepts/Item-17-shared-state-parallelism/README.md) |
| 18 | Don't panic | [Item-18](./Chapter-03-Concepts/Item-18-dont-panic/README.md) |
| 19 | Avoid reflection | [Item-19](./Chapter-03-Concepts/Item-19-avoid-reflection/README.md) |
| 20 | Avoid the temptation to over-optimize | [Item-20](./Chapter-03-Concepts/Item-20-avoid-over-optimize/README.md) |

### Chapter 4 — Dependencies

| Item | 标题 | 笔记 |
|------|------|------|
| 21 | Understand what semantic versioning promises | [Item-21](./Chapter-04-Dependencies/Item-21-semver/README.md) |
| 22 | Minimize visibility | [Item-22](./Chapter-04-Dependencies/Item-22-minimize-visibility/README.md) |
| 23 | Avoid wildcard imports | [Item-23](./Chapter-04-Dependencies/Item-23-avoid-wildcard-imports/README.md) |
| 24 | Re-export dependencies whose types appear in your API | [Item-24](./Chapter-04-Dependencies/Item-24-re-export-api-types/README.md) |
| 25 | Manage your dependency graph | [Item-25](./Chapter-04-Dependencies/Item-25-dependency-graph/README.md) |
| 26 | Be wary of feature creep | [Item-26](./Chapter-04-Dependencies/Item-26-feature-creep/README.md) |

### Chapter 5 — Tooling

| Item | 标题 | 笔记 |
|------|------|------|
| 27 | Document public interfaces | [Item-27](./Chapter-05-Tooling/Item-27-document-public-api/README.md) |
| 28 | Use macros judiciously | [Item-28](./Chapter-05-Tooling/Item-28-macros-judiciously/README.md) |
| 29 | Listen to Clippy | [Item-29](./Chapter-05-Tooling/Item-29-clippy/README.md) |
| 30 | Write more than unit tests | [Item-30](./Chapter-05-Tooling/Item-30-beyond-unit-tests/README.md) |
| 31 | Take advantage of the tooling ecosystem | [Item-31](./Chapter-05-Tooling/Item-31-tooling-ecosystem/README.md) |
| 32 | Set up a continuous integration (CI) system | [Item-32](./Chapter-05-Tooling/Item-32-ci/README.md) |

### Chapter 6 — Beyond Standard Rust

| Item | 标题 | 笔记 |
|------|------|------|
| 33 | Consider making library code no_std compatible | [Item-33](./Chapter-06-Beyond-Standard-Rust/Item-33-no-std/README.md) |
| 34 | Control what crosses FFI boundaries | [Item-34](./Chapter-06-Beyond-Standard-Rust/Item-34-ffi-boundaries/README.md) |
| 35 | Prefer bindgen to manual FFI mappings | [Item-35](./Chapter-06-Beyond-Standard-Rust/Item-35-bindgen/README.md) |

---

## 与 The Book 章节对照（粗线）

| Effective Rust | 可衔接的本仓库章节 |
|----------------|-------------------|
| Ch1 Types（1–9） | `00-Book/06-enums-*`、`00-Book/08-collections`、`00-Book/09-error-handling` |
| Ch2 Traits（10–13） | `00-Book/10-generics-traits-lifetimes` |
| Ch3 Concepts（14–20） | `00-Book/04-ownership`、`00-Book/10.3` 生命周期、`00-Book/16-fearless-concurrency` |
| Ch4–6 | `00-Book/14-cargo-crates`、`00-Book/19-advanced-features` 等 |

---

## 使用说明

- 每个 `Item-NN-slug/` 目录含 `README.md`（笔记正文）及可选 `demo/`（代码）；详见 [目录结构.md](./目录结构.md)。
- **§6 拓展**展开见 [ER-拓展索引.md](./ER-拓展索引.md)。
- **Workspace 配置**：[ER-demos/](./ER-demos/README.md)、[WORKSPACE.md](./ER-demos/WORKSPACE.md)（MSRV、`workspace.dependencies`、`clippy.toml`）。
- 示例 CI：[`.github/workflows/er-study-ci.yml`](../.github/workflows/er-study-ci.yml)（Item 32）。
