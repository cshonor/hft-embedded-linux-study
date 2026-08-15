# 3.2 Build Configuration（构建配置）

> 所属：**Project Configuration** · [← 章索引](./README.md)

← [04 Crate 元数据](./04-crate-metadata.md) · 下一节 [06 条件编译](./06-conditional-compilation.md)

前置 → [03 工作区](./03-workspaces.md)（根 profile / patch 仅根生效）· [04 Metadata vs profile](./04-crate-metadata.md)

Book → [14.1 发布配置 / Profiles](../../00-Book/14-cargo-crates/14.1-采用发布配置自定义构建.md)

---

## 章节总览

| 模块 | 核心作用 | 关键要点 |
|------|----------|----------|
| **`[patch]`** | **临时**替换依赖解析源 | 上游 bug 本地 fork/path；**仅根生效**；**不**随 `cargo publish` 上传 |
| **`[profile.*]`** | 编译优化策略 | `opt-level` / `lto` / `panic` / `codegen-units` — 平衡**编译速度 vs 运行性能** |
| **Workspace 继承** | 多包统一构建配置 | **`[profile]`、`[patch]` 仅工作区根**；自定义 profile 用 `inherits` |

> 与 [04 Metadata](./04-crate-metadata.md) 分工：metadata 管 crates.io **展示**；本章管 **编译与依赖解析**。

---

## 一、`[patch]` — 依赖临时替换

### 用途

crates.io 依赖有漏洞 / bug，官方尚未发新版 — Fork 修复后，**本地开发**强制 Cargo 用你的源码，而非 registry 原版。

```toml
# 根 Cargo.toml（Workspace 仅根生效）
[patch.crates-io]
serde = { path = "./local-fix-serde" }
# 或
tokio = { git = "https://github.com/you/tokio-fix", branch = "bugfix" }
```

### 替换 Git 来源依赖

```toml
[dependencies]
foo = { git = "https://github.com/orig/foo" }

[patch."https://github.com/orig/foo"]
foo = { path = "./local-fix-foo" }
```

### 注意事项

| # | 规则 |
|:-:|------|
| 1 | **仅本地开发** — `cargo publish` **不会**把 patch 打进发布包 |
| 2 | 只改**依赖解析路径** — lock 里仍记录**原版本约束**；编译时用 patch 源 |
| 3 | **仅工作区根** — 子 crate 内 `[patch]` **被忽略** |
| 4 | 同一包 `path` 与 `git` **互斥** |
| 5 | 上游合并修复后 **删除 patch** — 禁止带 patch 上线 |

---

## 二、`[profile.*]` — 编译优化配置

Profile 控制 rustc 代码生成 — **编译速度 · 运行性能 · 二进制体积** 三者取舍。

### 常用字段

| 字段 | 说明 |
|------|------|
| **`opt-level`** | `0`~`3` · `"s"` · `"z"` — 越高通常运行越快、编译越慢；**dev 默认 0，release 默认 3** |
| **`codegen-units`** | 并行编译单元数 — **越多编译越快、跨单元优化越弱**；`1` = 慢但峰值性能 |
| **`lto`** | 链接时优化 — `false` / `"thin"` / `true`（fat）— release 常开以缩体积、提性能 |
| **`panic`** | `"unwind"`（默认，可 `catch_unwind`）/ `"abort"`（直接终止，体积更小） |
| **`strip`** | 剥离符号 — 如 `"symbols"` 缩小 release 体积 |

### 内置默认 Profile

| Profile | 触发方式 | 典型用途 |
|---------|----------|----------|
| **`dev`** | `cargo build` | 开发调试 — 低优化、保留调试符号 |
| **`release`** | `cargo build --release` | 生产发布 — 高优化 |
| **`test`** | `cargo test` 编译测试二进制 | 测试专用 |
| **`bench`** | `cargo bench` | 基准测试 |

未写 `[profile.*]` 时 Cargo 用内置默认值 → [Book 14.1](../../00-Book/14-cargo-crates/14.1-采用发布配置自定义构建.md)

### Release 生产模板

```toml
[profile.release]
opt-level = 3
codegen-units = 1
lto = true
panic = "abort"
strip = "symbols"
```

> `panic = "abort"` 时 **`cargo test` 等仍常用 unwind** — 团队须统一约定。

→ HFT / 性能向：[Ch02 §05.4](../Chapter-02-Types/05-4-selection-hft.md)

---

## 三、Workspace Profile / Patch 继承

### 核心规则

1. **`[profile.*]`、`[patch]` 仅工作区根生效** — 子 crate 内写的 **整段被 Cargo 忽略**  
2. **`inherits`** — 在**根**定义**新 profile 名**（如 `ci`）时继承已有 profile，再局部覆盖  
3. **单个子 crate 特殊编译参数** — 根目录 **`[profile.release.package.包名]`**，不是 member 里写 `[profile]`

```text
workspace-root/Cargo.toml   ← patch、profile、inherits、package 覆盖
crates/my-lib/Cargo.toml    ← 不要写 [profile] / [patch]
```

### `inherits`（根 · 新 profile 名）

```toml
[profile.release]
opt-level = 3
lto = true
panic = "abort"

[profile.ci]
inherits = "release"   # 全盘复制 release，再覆盖下面字段
lto = false
panic = "unwind"
```

### 单包微调（根 · `package` 覆盖）

```toml
[profile.release]
opt-level = 3

[profile.release.package.my-lib]
opt-level = 2   # 仅 my-lib；其他 member 仍用 3
```

> ❌ 误区：在 `crates/my-lib/Cargo.toml` 写 `[profile.release] inherits = "release"` — **无效**。  
> → 完整汇总：[05.1 Workspace+Patch+Profile](./05-1-workspace-patch-profile-汇总.md)

---

## 四、完整 Workspace 模板（可直接复制）

```toml
# workspace-root/Cargo.toml
[workspace]
members = ["crates/my-lib", "apps/my-bin"]
resolver = "2"

[workspace.package]
version = "0.1.0"
edition = "2021"
license = "MIT OR Apache-2.0"

[workspace.dependencies]
serde = { version = "1.0", features = ["derive"] }

# 临时修上游 — 发布前删除
[patch.crates-io]
# broken-crate = { path = "patches/broken-crate" }

[profile.release]
opt-level = 3
codegen-units = 1
lto = "thin"
panic = "abort"

[profile.dev]
opt-level = 0

[profile.ci]
inherits = "release"
lto = false
panic = "unwind"

[profile.release.package.my-lib]
opt-level = 2
```

```toml
# crates/my-lib/Cargo.toml
[package]
name = "my-lib"
version.workspace = true
edition.workspace = true

[dependencies]
serde = { workspace = true }
```

---

## 五、章内导航

| | 节 |
|---|-----|
| 上一节 | [03 工作区](./03-workspaces.md) · [04 Crate 元数据](./04-crate-metadata.md) |
| **当前** | patch · profile · workspace 继承 |
| 下一节 | [06 条件编译](./06-conditional-compilation.md) |

---

## 六、核心速记

1. **`[patch]`** — 仅根 · 临时 · 不 publish · 修完删。  
2. **`opt-level`** — 0 调试 · 3 速度 · s/z 体积。  
3. **`codegen-units=1` + `lto`** — 换编译时间换运行性能。  
4. **`panic=abort`** — 小体积；测试常仍 unwind。  
5. **Profile / patch** — **根统一管理** · `inherits` 复用自定义 profile。

→ 速记：下一节：[06 条件编译](./06-conditional-compilation.md)
