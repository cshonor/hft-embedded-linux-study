# 3.2.1 Workspace + Patch + Profile 完整汇总

← [05 构建配置](./05-build-configuration.md) · [03 工作区](./03-workspaces.md)

> 把 Workspace、patch、profile、inherits 规则一次性讲清 · 含**常见误区纠正**

---

## 四、整体架构一句话

1. **Patch** — **仅根目录**；本地临时换依赖；按 crates-io / Git **来源精准匹配**。  
2. **Profile 基准** — **仅根目录**写 dev/release/自定义 profile；全 workspace 统一。  
3. **单包微调** — 根目录 `[profile.release.package.包名]` 覆盖字段；**不是**在子 crate 里写 `[profile]`。  
4. **`inherits`** — 在**根**定义**新 profile 名**时继承已有 profile；不必重复抄写全部字段。

---

## 一、`[patch]` — 本地临时替换依赖

### 核心作用

上游 crate 有 bug、官方未发修复版 — **仅本地开发**临时换源；**不随 `cargo publish` 上传**；新人若无补丁目录则 patch 失效。

### 两种写法（按来源一一对应）

| 块 | 替换对象 |
|----|----------|
| **`[patch.crates-io]`** | 来自 **crates.io** 的依赖 |
| **`[patch."Git完整URL"]`** | 与 `[dependencies]` 里 **Git 地址完全一致** 的依赖 |

```toml
[patch.crates-io]
serde = { path = "./local-fix-serde" }

[patch."https://github.com/tokio-rs/tokio"]
tokio = { path = "./local-fix-tokio" }
```

多个来源 → **多个独立 `[patch.xxx]` 块**，互不干扰。

### Workspace 铁则

| # | 规则 |
|:-:|------|
| 1 | **只有根 `Cargo.toml` 的 patch 生效** — 子 crate 写的 **Cargo 忽略** |
| 2 | 只改解析路径；lock 仍记录原版本约束 |
| 3 | 长期：PR 合并 → 删 patch → 升级依赖版本 |

---

## 二、`[profile.*]` — 编译配置

### 是什么

控制 **opt-level · lto · panic · codegen-units · strip** 等 — 平衡编译速度 vs 运行性能 vs 体积。

### 内置四 profile

| Profile | 典型触发 |
|---------|----------|
| **`dev`** | `cargo build` |
| **`release`** | `cargo build --release` |
| **`test`** | `cargo test` 编译测试二进制 |
| **`bench`** | `cargo bench` |

可自定义如 **`ci`** — 在**根**定义，`cargo build --profile ci`。

### Workspace 铁则

**`[profile.*]` 仅根目录生效** — 子 crate 内写的 profile **整段被忽略**（Cargo 官方规则）。

→ 工程规范：基准 dev/release/ci **全部收敛在根**，保证多包编译标准一致。

---

## 三、`inherits` 与单包微调（重点 · 纠正误区）

### `inherits` 是什么

在**根目录**定义**新的 profile 名字**时：

```toml
# 根 Cargo.toml
[profile.release]
opt-level = 3
lto = true
panic = "abort"

[profile.ci]
inherits = "release"   # 全盘复制 release 的所有字段
lto = false            # 只覆盖这一项；opt-level、panic 仍沿用 release
panic = "unwind"
```

```text
inherits = "父名"  →  先复制父 profile 全部参数  →  同块内写的字段覆盖同名项
```

**不需要**把 release 每一行再抄一遍。

### ❌ 常见误区：在子 crate 里写 `[profile]` + `inherits`

```toml
# crates/my-lib/Cargo.toml  ← 这样写无效！
[profile.release]
inherits = "release"
opt-level = 2
```

Workspace 下 **member 的 `[profile]` 会被忽略** — 不会按你想象只给 my-lib 生效。

### ✅ 正确：单个子 crate 微调（根目录 package 覆盖）

```toml
# 根 Cargo.toml
[profile.release]
opt-level = 3
lto = true
panic = "abort"

# 只有 my-lib 这个 package 的 release 构建用 opt-level 2，其余 member 仍用 3
[profile.release.package.my-lib]
opt-level = 2
```

| 需求 | 写法位置 |
|------|----------|
| 全 workspace 统一 release | 根 `[profile.release]` |
| 全 workspace 用 CI 配置 | 根 `[profile.ci]` + `inherits` |
| **某一个 crate** 特殊参数 | 根 `[profile.release.package.包名]` |

### 设计总结

```text
统一基准     →  根 profile（dev / release / ci…）
全 workspace 继承新 profile  →  根 [profile.ci] inherits = "release"
单 crate 微调  →  根 [profile.xxx.package.crate-name]
patch          →  仅根，无例外
```

---

## 五、完整根 Cargo.toml 模板

```toml
[workspace]
members = ["crates/my-lib", "apps/my-bin"]
resolver = "2"

[workspace.dependencies]
serde = "1.0"

# ── patch：仅根 · 发布前删 ──
[patch.crates-io]
# serde = { path = "patches/serde" }

# ── profile：仅根 ──
[profile.release]
opt-level = 3
lto = "thin"
panic = "abort"

[profile.dev]
opt-level = 0

[profile.ci]
inherits = "release"
lto = false
panic = "unwind"

# 单包微调示例
[profile.release.package.my-lib]
opt-level = 2
```

```toml
# crates/my-lib/Cargo.toml — 不写 [profile] / [patch]
[package]
name = "my-lib"
version = "0.1.0"

[dependencies]
serde = { workspace = true }
```

---

## 六、与分节笔记对照

| 主题 | 详读 |
|------|------|
| Workspace / lock / members | [03 工作区](./03-workspaces.md) |
| Metadata vs profile | [04 元数据](./04-crate-metadata.md) |
| patch / profile 展开 | [05 构建配置](./05-build-configuration.md) |

---

## 速记

## 一句话

Patch/Profile **仅根** · inherits **根上新 profile** · 单包微调 **`profile.xxx.package.包名`**

## [patch]

| 块 | 替换 |
|----|------|
| `[patch.crates-io]` | registry 依赖 |
| `[patch."git-url"]` | 同 URL 的 git 依赖 |

不 publish · 子 crate 无效 · 多块分来源

## [profile]

dev / release / test / bench · 自定义 `ci` 等 — **仅根**

## inherits

```toml
[profile.ci]
inherits = "release"
lto = false
```

根上定义 · 复制父 profile 再局部覆盖

## 单包微调（不是子 crate profile）

```toml
[profile.release.package.my-lib]
opt-level = 2
```

## 误区

❌ 子 crate `[profile.release] inherits = …` → **被忽略**  
✅ 根 `[profile.release.package.my-lib]`

## 自测

- [ ] patch 写子 crate 会生效吗？  
- [ ] 只让 my-lib release 用 opt-level 2 怎么写？  
- [ ] `inherits` 能写在 member Cargo.toml 吗？

