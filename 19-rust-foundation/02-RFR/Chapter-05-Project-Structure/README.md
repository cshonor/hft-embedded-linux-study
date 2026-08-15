# 第 5 章：项目结构 (Project Structure)

> **Rust for Rustaceans** · [全书目录](../RFR-本书目录.md)  
> crate 变大、对外发布、依赖复杂时，用 Cargo **组织清楚、构建可控、版本可演进**。

## 本章结构（与原书对齐）

**6 个主节** · 连同二级子节共 **14 个部分**（3 个带子的主节标题 + 2 + 1 + 2 + 1 + 4 + Summary）。

| | 笔记 |
|---|------|
| **基础辨析** | [00 Package vs Workspace](./00-package-vs-workspace.md)（lib+bin · mod vs crate） |

| 主节 | 英文 | 子节 / 笔记 |
|------|------|-------------|
| **1** | Features | [01 定义与包含](./01-defining-including-features.md)（Additive · optional）· [02 crate 内使用](./02-using-features-in-crate.md)（cfg · CI · 反模式） |
| **2** | Workspaces | [03 工作区](./03-workspaces.md)（lock/target 共享 · resolver 2）· [03.1 Cargo.lock](./03-1-cargo-lock.md)（字段 · 提交规范 · [demo](./cargo-lock-demo/)） |
| **3** | Project Configuration | [04 Crate 元数据](./04-crate-metadata.md)（）· [05 构建配置](./05-build-configuration.md)（）· [05.1 Patch+Profile 汇总](./05-1-workspace-patch-profile-汇总.md)（） |
| **4** | Conditional Compilation | [06 条件编译](./06-conditional-compilation.md)（cfg · target deps） |
| **5** | Versioning | [07 MSRV](./07-msrv.md)（）· [08 依赖下界](./08-minimal-dependency-versions.md)（）· [09 Changelog](./09-changelogs.md)（）· [10 未发布版本](./10-unreleased-versions.md)（Unreleased · 预发布） |
| **6** | Summary | [11 小结](./11-summary.md) |

## 与 The Book / ER 对照

| 主题 | 本仓库 |
|------|--------|
| Cargo 工作空间 | [14.3 工作空间](../../00-Book/14-cargo-crates/14.3-Cargo工作空间.md) |
| Feature | [ER Item 26](../../01-ER/Chapter-04-Dependencies/Item-26-feature-creep/README.md) |
| SemVer | [ER Item 21](../../01-ER/Chapter-04-Dependencies/Item-21-semver/README.md) |
| 依赖图 | [ER Item 25](../../01-ER/Chapter-04-Dependencies/Item-25-dependency-graph/README.md) |

## 旧版单文件

见 git 中的 `5-项目结构-Project-Structure-深度解析.md`。

## 建议阅读顺序

```text
00（Package vs Workspace）→ 01 → 02 → 03 → … → 11
```
