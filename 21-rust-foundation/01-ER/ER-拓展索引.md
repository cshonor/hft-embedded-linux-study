# Effective Rust — §6 拓展索引

各 Item 笔记 §6 的展开版；demo 见 [ER-demos/README.md](./ER-demos/README.md)。

---

## Item 01 {#item-01}

- [x] **Option / Result 链** → [Item 3](./Chapter-01-Types/Item-03-option-result-transforms/README.md)；Book [9.2](../00-Book/09-error-handling/9.2-Result-与可恢复的错误.md)
- [ ] **类型状态机实战**：HTTP 解析、协议状态用 enum 消无效组合；对照 [17.3 博客工作流](../00-Book/17-oop/17.3-状态模式与博客工作流.md)

## Item 02 {#item-02}

- [x] **单态化 vs trait 对象** → [Item 12](./Chapter-02-Traits/Item-12-generics-vs-trait-objects/README.md)
- [ ] **闭包 + Send/Sync**：`move` 闭包跨线程；Book [16.4](../00-Book/16-fearless-concurrency/16.4-Send与Sync.md)

## Item 03 {#item-03}

- [x] **Demo**：[item-03-option-result](./Chapter-01-Types/Item-03-option-result-transforms/demo/) — `ok_or`、`transpose`、`and_then` 链
- [x] **Option ↔ Result 拓扑**：`ok_or` / `ok_or_else`、`transpose`、`and_then`；Book [9.2](../00-Book/09-error-handling/9.2-Result-与可恢复的错误.md)
- [x] **`?` + `From`** → [Item 4](./Chapter-01-Types/Item-04-idiomatic-error-types/README.md)

## Item 04 {#item-04}

- [x] **Demo**：[item-04-error-types](./Chapter-01-Types/Item-04-idiomatic-error-types/demo/) — 库 `thiserror` + 应用 `anyhow`
- [x] **`core::error::Error` + `no_std`**：[item-04-core-error](./Chapter-01-Types/Item-04-idiomatic-error-types/demo-core-error/) — 手动 `impl Error` + `alloc`

## Item 05 {#item-05}

- [x] **Demo**：[item-05-06-newtype](./Chapter-01-Types/Item-05-type-conversions/demo/) — `Deref` 强制转换 + `print_len(&Wrapper)`
- [x] **`AsRef` vs `Into`**：同 demo 模块 `as_ref_vs_into.rs` — 只读 `AsRef<str>` vs 存盘 `Into<String>`

## Item 06 {#item-06}

- [x] **Demo**：[item-05-06-newtype](./Chapter-01-Types/Item-05-type-conversions/demo/) — `derive_more` 的 `From`/`Add`/`Display`
- [x] **过度 Deref**：同 demo 模块 `over_deref.rs` — `LeakyDb` 绕过校验 vs `UserDb` 受控 API

## Item 07 {#item-07}

- [ ] **`derive_builder`**：`#[builder(setter(into))]`、字段级 `validate`、可选字段默认

## Item 08 {#item-08}

- [ ] **`UnsafeCell`**：`RefCell` / `Cell` 底层；`Sync` 为何需要额外保证
- [ ] **Async**：任务内 `Rc` 非 `Send` → `Arc` + `tokio::sync::Mutex`

## Item 09 {#item-09}

- [x] **迭代器性能** → Book [13.4](../00-Book/13-iterators-closures/13.4-性能对比-循环-vs-迭代器.md)
- [ ] **`itertools`**：`chunk_by`、`unique`、`collect_vec`
- [ ] **Godbolt**：对比 `for i in 0..n` vs `(0..n).map` 的 LLVM IR

## Item 10 {#item-10}

- [ ] **`derivative` crate**：部分字段跳过 `Hash`/`Eq`
- [ ] **`std::ops` 组合 impl**：`Add<&T>` 等可用 `derive_more` 或 small macro

## Item 11 {#item-11}

- [ ] **Drop 中 panic**：double panic → abort；避免在 `Drop` 里再 panic
- [ ] **Async shutdown**：无 `AsyncDrop` 稳定 API 前用显式 `close().await` + 文档契约

## Item 12 {#item-12}

- [x] **trait upcasting (1.76+)**：`dyn SubTrait` → `dyn SuperTrait`；`Any` 仍无 supertrait
- [ ] **bench**：同逻辑 `impl Trait` vs `dyn Trait` → [item-30-black-box](./Chapter-05-Tooling/Item-30-beyond-unit-tests/demo/) + criterion

## Item 13 {#item-13}

- [ ] **默认方法 + `Self: Sized`**：`dyn Trait` 不能调用带默认 impl 且 `Self` 未约束的方法
- [ ] **Serde 模式**：小 trait 核心 + 大量默认方法在 extension trait

## Item 14 {#item-14}

- [ ] **HRTB** `for<'a> fn(&'a str) -> &'a str`：高阶生命周期；Fn trait 与 GAT
- [x] **图结构** → [Item 15](./Chapter-03-Concepts/Item-15-borrow-checker/README.md) + Book [10.3](../00-Book/10-generics-traits-lifetimes/10.3-生命周期与引用有效性.md)

## Item 15 {#item-15}

- [x] **Demo**：[item-15-borrow-checker](./Chapter-03-Concepts/Item-15-borrow-checker/demo/)
- [ ] **`Pin` + async**：自引用状态机；`!Unpin` 堆固定
- [ ] **Polonius**：NLL 后继；减少「活得太久」误报

## Item 16 {#item-16}

- [x] **unsafe demo** → Book [19.1](../00-Book/19-advanced-features/19.1-不安全Rust.md)
- [x] **Miri CI**：`cargo +nightly miri test -p item-16-miri`；Strict Provenance 违规见 crate 内 `#[ignore]` 测试
- [ ] **C/C++ 迁移**：`-sys` + safe 层 → [Item 34–35](./Chapter-06-Beyond-Standard-Rust/)

## Item 17 {#item-17}

- [x] **Demo** → Book [16.3 mutex-arc](../00-Book/16-fearless-concurrency/16.3-共享状态并发.md)
- [ ] **`parking_lot::deadlock`** / TSan 检测锁顺序
- [ ] **`RwLock` / `Atomic*`**：读多写少、计数器热点

## Item 18 {#item-18}

- [x] **Demo**：[item-18-dont-panic](./Chapter-03-Concepts/Item-18-dont-panic/demo/)
- [ ] **`no_panic` crate** + CI（[Item 32](./Chapter-05-Tooling/Item-32-ci/README.md)）
- [ ] **`AssertUnwindSafe`**：`catch_unwind` 与 exception safety

## Item 19 {#item-19}

- [x] **type_name / Any** → Book [19.3](../00-Book/19-advanced-features/19.3-高级类型.md)
- [ ] **插件架构**：trait 注册 vs `inventory` / `linkme` 编译期表

## Item 20 {#item-20}

- [x] **Demo**：[item-20-tlv](./Chapter-03-Concepts/Item-20-avoid-over-optimize/demo/)
- [x] **black_box / bench** → [item-30-black-box](./Chapter-05-Tooling/Item-30-beyond-unit-tests/demo/)
- [ ] **`Cow<'_, [u8]>`**：解析时借或拥有

## Item 21 {#item-21}

- [x] **`cargo-semver-checks`**：CI `semver-checks` job（PR vs base）+ [item-24 README](./Chapter-04-Dependencies/Item-24-re-export-api-types/demo/README.md)
- [x] **MSRV**：[WORKSPACE.md](./ER-demos/WORKSPACE.md) + CI `msrv` job（`rust-version = "1.70"`）

## Item 22 {#item-22}

- [x] **Demo**：[item-22-visibility](./Chapter-04-Dependencies/Item-22-minimize-visibility/demo/) + Book [7.2](../00-Book/07-packages-modules/7.2-定义模块来控制作用域与私有性.md)
- [ ] **`mod.rs` vs `foo.rs`**：2018 edition 模块树；[7.5](../00-Book/07-packages-modules/7.5-将模块分割进不同文件.md)
- [ ] **门面 `pub use`** → [Item 24](./Chapter-04-Dependencies/Item-24-re-export-api-types/README.md)

## Item 23 {#item-23}

- [x] **Clippy `wildcard_imports`** → [Item 29](./Chapter-05-Tooling/Item-29-clippy/README.md)
- [ ] **crate prelude 设计**：`pub mod prelude { pub use ... }` 精选导出

## Item 24 {#item-24}

- [x] **Demo**：[item-24-re-export](./Chapter-04-Dependencies/Item-24-re-export-api-types/demo/) — `pub use rand` 与 **newtype** 两方案
- [x] **诊断**：`cargo tree -p consumer -d rand`
- [x] **`cargo-public-api`**：CI `public-api` job + README 用法
- [x] **newtype 隐藏**：`dep-lib-newtype` / `consumer-newtype`

## Item 25 {#item-25}

- [x] **Dependabot + `cargo deny check`**：[dependabot.yml](../.github/dependabot.yml) + [deny.toml](../deny.toml) + CI `cargo-deny`
- [x] **`[workspace.dependencies]`**：[Cargo.toml](./Cargo.toml) + [WORKSPACE.md](./ER-demos/WORKSPACE.md)

## Item 26 {#item-26}

- [x] **Demo**：[item-26-feature-creep](./Chapter-04-Dependencies/Item-26-feature-creep/demo/) + README 中 `cargo hack --feature-powerset`
- [x] **docs.rs**：demo `Cargo.toml` 注释 `[package.metadata.docs.rs] all-features = true`

## Item 27 {#item-27}

- [x] **doc test** → Book [14.2](../00-Book/14-cargo-crates/14.2-将crate发布到Crates.io.md)
- [ ] **`#[doc(cfg(feature = "std"))]`**：docs.rs 按 feature 标注
- [ ] **crate 级 `//!` 指南**：Quick start + 模块地图

## Item 28 {#item-28}

- [x] **宏 demo** → Book [19.5](../00-Book/19-advanced-features/19.5-宏.md)
- [ ] **`cargo expand`**：调试 `macro_rules!`
- [ ] **`syn` + `quote`**：最小 derive 宏 crate

## Item 29 {#item-29}

- [x] **`clippy.toml`**：[ER-demos/clippy.toml](./ER-demos/clippy.toml)
- [x] **CI `-Dwarnings`** → [`.github/workflows/er-study-ci.yml`](../.github/workflows/er-study-ci.yml)

## Item 30 {#item-30}

- [x] **Demo**：[item-30-black-box](./Chapter-05-Tooling/Item-30-beyond-unit-tests/demo/)
- [x] **CI matrix**：`matrix-demo` job — `os × rust × features`（Item 26 / 30）
- [ ] **`cargo-fuzz` corpus**：`fuzz/corpus/` 入仓

## Item 31 {#item-31}

- [ ] **Kani / Prusti**：安全关键域形式化验证入门
- [ ] **`cargo-public-api`** → Item 24

## Item 32 {#item-32}

- [x] **CI 示例**：[`.github/workflows/er-study-ci.yml`](../.github/workflows/er-study-ci.yml)
- [x] **rust-toolchain.toml**（可选）钉 toolchain

## Item 33 {#item-33}

- [x] **Demo**：[item-33-no-std](./Chapter-06-Beyond-Standard-Rust/Item-33-no-std/demo/)（`cargo test -p item-33-no-std`）
- [ ] **`no_global_oom_handling`** + `#![cfg(all(not(feature = "std")))]`
- [ ] **`embedded-hal`** 分层

## Item 34 {#item-34}

- [x] **Demo**：[item-34-ffi-box](./Chapter-06-Beyond-Standard-Rust/Item-34-ffi-boundaries/demo/)
- [x] **bindgen** → [Item 35](./Chapter-06-Beyond-Standard-Rust/Item-35-bindgen/README.md)
- [ ] **`cxx`**：Rust ↔ C++ schema

## Item 35 {#item-35}

- [x] **Demo**：[item-35-bindgen](./Chapter-06-Beyond-Standard-Rust/Item-35-bindgen/demo-bindgen/) — 单 crate 内 bindgen + safe 封装
- [x] **`-sys` / safe 双层**：[item-35-sys-workspace](./Chapter-06-Beyond-Standard-Rust/Item-35-bindgen/demo-sys-workspace/) — `er-add-sys` + `er-add`
- [x] **rerun-if-changed**：见各 crate `build.rs`
