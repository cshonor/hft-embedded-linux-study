# 附录 · 如何阅读标准库源码

> 所属：[第 2 章](./README.md) · 前：[附录 · 设计哲学](./appendix-design-philosophy.md) · 后：[2.1 核心特性](./2.1-core-features-and-std.md)

---

## 一句话

标准库源码 **能读、值得读** — 用对工具、从 **`libstd` 入口** 和 **`pub` 接口背后的 `unsafe` 实现** 两条线切入，正好指导本仓库后续 **按模块做笔记** 的方向。

---

## 工具

| 工具 | 用途 |
|------|------|
| **[docs.rs](https://docs.rs/std)** | 文档页 **「Source」** 跳转到对应 commit 的 `rust` 仓库 |
| **`rustup component add rust-src`** | 本地 `cargo doc --open` 可进源码；IDE **Go to Definition** 进 `libstd` |
| **`$RUST_SRC`** / `~/.rustup/toolchains/.../lib/rustlib/src/rust/library/` | 直接浏览 `std` / `alloc` / `core` 目录树 |
| **GitHub `rust-lang/rust`** | `library/std/src/lib.rs` 为 `std` crate 根；`library/alloc`、`library/core` 并列 |

```bash
# 安装源码组件（一次）
rustup component add rust-src
```

---

## 推荐路径

```text
library/std/src/lib.rs          ← std 入口：模块树、重导出、rt
        │
        ├── 跟 pub API 跳转      ← 例如 vec/mod.rs → vec 实现
        │
        ├── 复杂类型拆到 alloc/  ← Vec 主体常在 alloc::vec
        │
        └── unsafe 集中在边界    ← push_nonzero、slice::from_raw_parts 等
```

### 阅读顺序（单模块）

1. **文档 + 公开签名** — `docs.rs` 或 `lib.rs` 里 `pub mod` 的接口。
2. **Safe 包装层** — 对外方法如何检查前置条件。
3. **`unsafe` 实现** — 扩容、指针算术、`assume_init` — 对照 [Nomicon 05 Uninit](../../04-Rust-Nomicon/05_Uninit_Mem/README.md)。
4. **跨 crate** — `core` 算法 vs `alloc` 分配 vs `std` OS 调用（见 [第 1 章](../chapter01_std_overview/README.md)）。

---

## 方法（本仓库笔记怎么用）

| 做法 | 说明 |
|------|------|
| **从入口到叶子** | 第 1 章建立三层地图（1.1～1.3）→ 第 2 章认语法 → 后续章按 **Vec / sync / net** 各写一篇「接口 + 关键 unsafe」 |
| **盯住 `pub` 下的 `unsafe`** | 标准库安全叙事 = **少量 `unsafe` 块 + 大量 invariant 注释** |
| **画调用链** | 例：`TcpStream::read` → `Read` trait → `sys` 模块 → `libc` |
| **与 ER/RFR 交叉链接** | 笔记里回链到已学章节，避免重复讲所有权基础 |
| **不必通读 `libstd`** | 按实盘触点深挖：`Vec`、`Arc<Mutex>`、`TcpStream`、`fs::File` |

---

## 本仓库后续章节方向（规划）

| 章 | 源码起点 | 笔记侧重 |
|----|----------|----------|
| 容器 | `alloc/vec` | 容量、增长、drop |
| 智能指针 | `alloc/sync` · `rc` | 引用计数、弱引用 |
| 并发 | `std/sync` · `thread` | 与 [05-atomic](../../05-Async-Concurrency-Network/01-atomic/README-学习区.md) 对照 |
| I/O | `std/io` · `sys` | 与 [05-network](../../05-Async-Concurrency-Network/03-rust_network_programming/README.md) 对照 |

---

## 相关

- [第 1 章 三层架构](../chapter01_std_overview/README.md)
- [附录 · 设计哲学](./appendix-design-philosophy.md)
- [Nomicon 08 实现 Vec](../../04-Rust-Nomicon/08_Impl_Vec_Arc/README.md)（手写对照官方实现）
