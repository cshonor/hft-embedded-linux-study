# 09 · Foreign Function Interface

> **The Rustonomicon** · [03 Rust Nomicon](../README.md) · [全书笔记](../notes.md)

## 状态

- [x] 已读（笔记整理）
- [x] 示例 crate（call C / export / callback / interop / unwind / opaque）

---

## 一句话

**FFI 避坑章** — Rust↔C 互调、`repr(C)`/`CString`、回调与全局变量风险、Option niche、panic 不可跨界、`catch_unwind`、opaque struct。

---

## 专项笔记

| 节 | 主题 | 阅读 |
|:--:|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| 1 | 从 Rust 调用 C | [01-call-c.md](./01-call-c.md) |
| 2 | 从 C 调用 Rust | [02-export-to-c.md](./02-export-to-c.md) |
| 3 | 回调 | [03-callbacks.md](./03-callbacks.md) |
| 4 | 互操作与数据表示 | [04-interop.md](./04-interop.md) |
| 5 | 外部全局变量 | [05-globals.md](./05-globals.md) |
| 6 | 可空指针优化 | [06-nullable.md](./06-nullable.md) |
| 7 | 异常与栈展开 | [07-unwind.md](./07-unwind.md) |
| 8 | 不透明结构体 | [08-opaque.md](./08-opaque.md) |
| — | 速记 · 自测 |

---

## 示例源码

| 文件 | 演示 |
|------|------|
| [src/call_c.rs](./src/call_c.rs) | 调用 C `abs` + Safe 包装 |
| [src/export_to_c.rs](./src/export_to_c.rs) | `extern "C"` + `#[no_mangle]` 导出 |
| [src/callbacks.rs](./src/callbacks.rs) | 函数指针回调 + `*mut` 状态 |
| [src/interop.rs](./src/interop.rs) | `repr(C)` + `CString` |
| [src/nullable.rs](./src/nullable.rs) | `Option<extern "C" fn>` niche |
| [src/unwind.rs](./src/unwind.rs) | `catch_unwind` 边界 |
| [src/opaque.rs](./src/opaque.rs) | 不透明类型 |
| [src/globals.rs](./src/globals.rs) | 外部 `static mut` 说明 |
| [src/main.rs](./src/main.rs) | 运行入口 |

```bash
cd 04-Rust-Nomicon/09_FFI
cargo run
cargo test
```

**导出 cdylib**（供 C 链接）：在 `Cargo.toml` 增加 `crate-type = ["cdylib"]` 后 `cargo build`。

---

## 与仓库其他部分

| 主题 | 对照 |
|------|------|
| RFR FFI | [Ch11](../../02-RFR/Chapter-11-Foreign-Function-Interfaces/README.md) |
| bindgen | [ER Item 35](../../01-ER/Chapter-06-Beyond-Standard-Rust/Item-35-bindgen/README.md) |
| 上一章 | [08_Impl_Vec_Arc](../08_Impl_Vec_Arc/README.md) |
| 下一章 | [10_NoStd](../10_NoStd/README.md) |

---

## 逻辑脉络

Rust→C → C→Rust → 类型/字符串 → 回调与全局 → Option niche → panic 边界 → opaque → no_std。

---

## 速记

## 三句背诵

1. **所有 C 调用都是 `unsafe`；对外暴露 Safe API 须在 Rust 侧包装边界。**
2. **跨边界 struct 用 `repr(C)`；字符串用 `CString`；`Option<fn ptr>` 的 None = null。**
3. **panic 不可跨界 → FFI 出口用 `catch_unwind`；opaque 用私有字段 struct + PhantomData。**

## 自测

- [ ] 能写出 `extern "C"` 声明与 `#[link]` 的基本形式
- [ ] 能说明 `cdylib` + `#[no_mangle]` 导出给 C 的步骤
- [ ] 能解释带状态回调为何需要 `*mut` + 生命周期管理
- [ ] 能说明 `static mut` 外部全局为何读写都 unsafe
- [ ] 能解释 panic 跨入 C 为何 UB
- [ ] 能对照 [src/call_c.rs](./src/call_c.rs) 与 [src/unwind.rs](./src/unwind.rs) 说出边界处理

## 术语表（本章）

| 术语 | 含义 |
|------|------|
| ABI / 调用约定 | `extern "C"` 等函数调用与符号约定 |
| niche | `Option<NonNull<T>>` 用 null 表示 None |
| opaque | C 侧不公开布局，Rust 侧仅持指针 |
| catch_unwind | 在 FFI 边界捕获 panic，防栈展开跨界 |

## 源码索引

| 文件 | 演示 |
|------|------|
| [src/call_c.rs](./src/call_c.rs) | 调用 C |
| [src/export_to_c.rs](./src/export_to_c.rs) | 导出给 C |
| [src/callbacks.rs](./src/callbacks.rs) | 回调 |
| [src/interop.rs](./src/interop.rs) | repr(C) / CString |
| [src/globals.rs](./src/globals.rs) | 外部全局 |
| [src/nullable.rs](./src/nullable.rs) | Option niche |
| [src/unwind.rs](./src/unwind.rs) | catch_unwind |
| [src/opaque.rs](./src/opaque.rs) | 不透明类型 |

