# 10 · Beneath std (no_std)

> **The Rustonomicon** · [03 Rust Nomicon](../README.md) · [全书笔记](../notes.md) · 全书收官

## 状态

- [x] 已读（笔记整理）
- [x] 示例 crate（`#![no_std]` 库 + `panic_handler` + no_main 模板）

---

## 一句话

**no_std 收官章** — 脱离 std 后接管入口、panic、libc/compiler_builtins；stable 可建 no_std 库，裸机 exe 通常需 nightly。

---

## 专项笔记

| 节 | 主题 | 阅读 |
|:--:|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| 1 | 引入 libc 依赖 | [01-libc.md](./01-libc.md) |
| 2 | 无 std 可执行程序 | [02-no-main.md](./02-no-main.md) |
| 3 | 自定义 panic 处理 | [03-panic-handler.md](./03-panic-handler.md) |
| — | no_main 裸机模板 | [templates/no_main_linux.md](./templates/no_main_linux.md) |
| — | 速记 · 自测 |

---

## 示例源码

| 文件 | 演示 |
|------|------|
| [src/lib.rs](./src/lib.rs) | `#![no_std]` + `#[panic_handler]` |
| [src/main.rs](./src/main.rs) | std 宿主调用 no_std 库 |
| [templates/no_main_linux.md](./templates/no_main_linux.md) | `_start` / `eh_personality` 参考 |

```bash
cd 04-Rust-Nomicon/10_NoStd
cargo run                              # std 宿主 demo
cargo build --no-default-features      # 纯 no_std 库
cargo test                             # std 下单元测试
```

**libc 依赖示例**（注释于 `Cargo.toml`）：

```toml
# libc = { version = "0.2", default-features = false }
```

---

## 与仓库其他部分

| 主题 | 对照 |
|------|------|
| ER no_std | [Item 33 demo](../../01-ER/Chapter-06-Beyond-Standard-Rust/Item-33-no-std/demo/) |
| OBRM / panic | [06_OBRM_RAII](../06_OBRM_RAII/README.md) |
| FFI 入口 | [09_FFI](../09_FFI/README.md) |
| 上一章 | [09_FFI](../09_FFI/README.md) |

---

## 逻辑脉络

libc 不拉回 std → no_main 入口 → panic_handler → 内核/嵌入式实战。

---

## 速记

## 三句背诵

1. **`libc` 必须 `default-features = false`，否则隐式拉回 `std`。**
2. **裸机 exe 用 `#![no_main]` + 手动 `_start`；常需 nightly lang items 与 `compiler_builtins`。**
3. **`#[panic_handler] fn(&PanicInfo) -> !` 全局唯一；dev 用 semihosting，release 用 halt。**

## 自测

- [ ] 能解释为何 `default-features = true` 的 libc 破坏 no_std
- [ ] 能列出 `no_main` 与默认 `main` 的区别
- [ ] 能写出 `panic_handler` 的正确签名
- [ ] 能说明 stable no_std 库 vs nightly 裸机 exe 的差异
- [ ] 能对照 [src/lib.rs](./src/lib.rs) 找到 panic 实现

## 术语表（本章）

| 术语 | 含义 |
|------|------|
| no_std | 不链接标准库；仍可用 core/alloc |
| lang item | 编译器/runtime 期望的符号（如 eh_personality） |
| panic_handler | no_std 下 panic 的唯一处理入口 |
| semihosting | 调试时通过主机 I/O 输出 panic |

## 构建速查

```bash
cd 04-Rust-Nomicon/10_NoStd
cargo run                              # std 宿主 demo
cargo build --no-default-features      # 纯 no_std 库
cargo test                             # std 下单元测试
```

