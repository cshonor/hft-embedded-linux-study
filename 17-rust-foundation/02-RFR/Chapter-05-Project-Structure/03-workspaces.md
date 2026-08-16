# 2. Workspaces（工作区）

> 所属：**Workspaces** · [← 章索引](./README.md)

← [02 crate 内使用 Feature](./02-using-features-in-crate.md) · 下一节 [04 Crate 元数据](./04-crate-metadata.md)

前置 → [00 Package vs Workspace](./00-package-vs-workspace.md)（单包 lib+bin · 多包辨析）

Book → [14.3 Cargo 工作空间](../../00-Book/14-cargo-crates/14.3-Cargo工作空间.md) · demo → [14.3-workspace-demo](../../00-Book/14-cargo-crates/14.3-workspace-demo/) · [14.3-hft-workspace-demo](../../00-Book/14-cargo-crates/14.3-hft-workspace-demo/)

---

## 核心结论

1. **根 `Cargo.toml` 不负责业务依赖、不负责子包版本号** — 只管「有哪些子包」；第三方依赖约束、包自身 `version`，**各子包自己的 `Cargo.toml` 独立管控**。  
2. **Workspace 统一落地的一件事**：根目录**唯一** `Cargo.lock` — 全工作区同一第三方 crate **只锁一个版本**，不会多版本并存。

```text
子包 TOML 提「版本要求」  →  Cargo 解析  →  lock 写「全局落地版本」
```

---

## 一、核心作用：解决巨石单 crate 痛点

### 巨石单 Package 缺陷

单 Package 内 `lib` 收拢所有 `mod` — 改任意内部模块常触发 **整个 lib 重编**；边界仅 `pub` 可见性，无 crate 级隔离。  
→ 辨析：[00 Package vs Workspace](./00-package-vs-workspace.md)

### Workspace 价值

同一仓库拆成多个独立 **member Package**：

| 收益 | 说明 |
|------|------|
| **增量编译** | 只重编变更 crate + 依赖它的上层 |
| **边界清晰** | 库 / 二进制 / 工具分包 |
| **统一治理** | lock、构建缓存 — 适合 monorepo |

---

## 二、根 `Cargo.toml` 写什么？能干什么？

根文件通常**没有** `[package]`、`[dependencies]`，核心只有 `[workspace]`：

```toml
# 根目录 Cargo.toml
[workspace]
members = [
    "crates/*",
    "apps/*",
]
exclude = []                              # 可选：排除某些路径
default-members = ["apps/server-app"]     # 可选：无 -p 时默认构建谁
resolver = "2"                            # 虚拟工作区建议手写，见 §八
```

### ✅ 根能干

| 职责 | 说明 |
|------|------|
| **登记成员** | `members` / `exclude` — Cargo 知道哪些子目录属于工作区 |
| **统一 lock** | 根目录唯一 `Cargo.lock`，锁定全工作区第三方版本 |
| **批量命令** | 根目录 `cargo build` / `test` / `clippy` 可操作全部或 `default-members` |
| **可选集中配置** | `[workspace.dependencies]`、`[workspace.package]`、`profile`、`patch`（见 §六） |

### ❌ 根不能干

| 误区 | 正解 |
|------|------|
| 在根写 `[package]` 版本、名字给「整个项目」 | **每个子包**各自 `[package].name` / `version` |
| 根写 `[dependencies]` 就自动注入所有子包 | **不能** — 子包须在自己 TOML 声明需要什么；根只能提供 `workspace.dependencies` **供继承** |
| 根统一管控子包 feature、内部编译选项 | 各子包自己的 `[features]`、`[lib]` 等 |

> **根本身不是 Package** — 只有 `[workspace]`、无 `[package]` 时，根只是「管理员」；能编译、有版本、有依赖的包，全是 `members` 里那些路径对应的子文件夹。

---

## 三、`members` 是什么？

`members` 是一个**路径列表**。每一条路径指向**一个独立 Package 所在的文件夹** — 该文件夹下**必须有一份 `Cargo.toml`**。

> **一句话**：`members` 填的是**子 Package 的文件夹路径**，不是 `Cargo.toml` 文件本身；文件夹里若无 `Cargo.toml`，Cargo 直接报错。

### 显式列举

```text
my_workspace/
├── Cargo.toml              # 根：只有 [workspace]
├── crates/core-utils/
│   └── Cargo.toml          # Package 1
└── apps/server-app/
    └── Cargo.toml          # Package 2
```

```toml
[workspace]
members = [
    "crates/core-utils",    # 去此路径找 Cargo.toml，纳入工作区
    "apps/server-app",
]
```

### 通配符简写

```toml
members = ["crates/*", "apps/*"]
```

`crates/*` = `crates/` 下**所有直接子文件夹**，内有 `Cargo.toml` 的自动算作 member — 不必逐个手写 `core-utils`、`database`……

### 三条配套规则

| # | 规则 |
|:-:|------|
| ① | **不在 `members` 里 = 不属于本 Workspace** — 哪怕有 `Cargo.toml`：不共用根 `Cargo.lock`；根 `cargo build`/`test` 不构建它；不能用 `workspace = true` 继承 |
| ② | **一个 Package 只能属于一个 Workspace** — 不能被两个不同根的 `members` 同时包含 |
| ③ | **根不是 Package** — 真正干活的是 `members` 里那些子包 |

### 极简示例

```text
root/
├── Cargo.toml          # 无 [package]
├── pkg_a/Cargo.toml    # 包 A
└── pkg_b/Cargo.toml    # 包 B
```

```toml
[workspace]
members = ["pkg_a", "pkg_b"]
```

根是管理员；`pkg_a`、`pkg_b` 才是有版本、有依赖、能编译的包。

---

## 四、版本号与依赖：每个子包自己管

每个 member（如 `crates/core-utils`、`apps/server-app`）都是**独立 Package**：

```toml
# crates/core-utils/Cargo.toml
[package]
name = "core-utils"
version = "0.1.0"    # 独立迭代，改这里不影响别的子包
edition = "2021"

[dependencies]
tokio = "1.35"
serde = { version = "1.0", features = ["derive"] }
```

| 字段 | 谁管 | 说明 |
|------|------|------|
| **`[package].version`** | 子包自己 | 工具库升版只改自己 |
| **`[dependencies]`** | 子包自己 | 要什么第三方、约束范围、开哪些 feature |
| **工作区内依赖** | 子包 path | `core-utils = { path = "../../crates/core-utils", version = "0.1.0" }` |

```toml
# apps/server-app/Cargo.toml
[dependencies]
core-utils = { path = "../../crates/core-utils", version = "0.1.0" }
axum = "0.7"
```

> `path` 依赖发布到 crates.io 时须带 `version`（与 path 包版本兼容），见 [04 元数据](./04-crate-metadata.md)。

---

## 五、`Cargo.lock` 如何统一第三方版本？

**大前提**：同一 Workspace 全局**只会锁定一个版本** — 不会同时装 `1.0` 和 `1.5` 两份同名 crate。

Cargo 把**所有子包**对同一依赖的约束放在一起求**交集**，再按 SemVer 选 lock 里的落地版本，写入根目录唯一 `Cargo.lock`。

### 例：包 A 要 `third = "1.0"`，包 B 要 `third = "1.5"`

Cargo 默认解读（caret 要求）：

| 声明 | 实际约束 |
|------|----------|
| `"1.0"` | `>=1.0.0, <2.0.0` |
| `"1.5"` | `>=1.5.0, <2.0.0` |

**交集**：`>=1.5.0, <2.0.0` → Cargo 取该区间内**最新稳定版**（如 `1.5.8`）写入 lock — **全 workspace 共用这一版**。

```text
crate1 声明 ≥1.0  →  1.5+ 仍兼容满足
crate2 声明 ≥1.5  →  必须 1.5+
→ 合并后统一升到 1.5+ 最新版，不会装两份
```

> 规范前提：第三方遵守 SemVer — 1.x 小版本向下兼容，crate1 被「抬」到 1.5+ 通常无问题。→ [07 MSRV](./07-msrv.md) · [ER SemVer](../../01-ER/Chapter-04-Dependencies/Item-21-semver/README.md)

### 场景 ①：同大版本（1.0 vs 1.5）→ 正常合并

```toml
# crate1/Cargo.toml
third = "1.0"
# crate2/Cargo.toml
third = "1.5"
```

区间重叠 → 取满足**最高要求**的新版本。

### 场景 ②：跨大版本（1.x vs 2.x）→ 直接冲突

```toml
# crate1
third = "1"    # <2.0.0
# crate2
third = "2"    # >=2.0.0
```

`1.x` 与 `>=2.0.0` **无交集** → Cargo 解析失败，构建报错。

**Workspace 默认不允许同一 crate 多版本共存** — 保证类型统一、避免二进制膨胀、跨子 crate 传参不会出现「1 版类型 vs 2 版类型」的诡异不兼容。

### 完整流程复盘

1. crate1：`third = "1.0"` → 允许 `1.0~1.x`  
2. crate2：`third = "1.5"` → 允许 `1.5~1.x`  
3. Cargo 求交集 → 拉最新发布版 → 写入根 lock  
4. crate1 自动用更高的 1.5+ — SemVer 兼容则编译运行正常  
5. 想主动管控 → 根 `[workspace.dependencies]` 锁定固定版本（见 §六）

→ lock 文件格式 · 提交规范 · `--locked`：[03.1 Cargo.lock](./03-1-cargo-lock.md)

### 与 `target/` 共享

- 编译产物、增量缓存统一输出到根 `target/`  
- 根 `cargo clean` 一次清完  

### 极端情况：必须同时用 1.x 和 2.x

Workspace **标准方案解决不了**，只有：

| 方案 | 说明 |
|------|------|
| **① 重构升级（优先）** | 把仍用 1.x 的 crate 改代码适配 2.x，全局统一大版本 |
| **② 拆 Workspace** | 两个独立仓库 / 两个 lock，各自用 1.x / 2.x；失去统一治理与 path 联动 |
| **③ patch 重命名（少用）** | `[patch]` 强行引入两份、类型割裂；业务项目尽量规避 → [05 构建配置](./05-build-configuration.md) |

---

## 六、公共依赖复用：`[workspace.dependencies]`（推荐）

不要让各子包各自乱写版本 — **在根统一管控**，从根源避免碎片化与 lock 拉扯：

### 1. 根定义工作区依赖

```toml
[workspace]
members = ["crates/*", "apps/*"]

[workspace.dependencies]
third = "1.5.10"   # 全局固定版本；crate1/crate2 都继承这一版
tokio = "1.35.0"
serde = { version = "1.0", features = ["derive"] }
```

### 2. 子包继承 — crate1、crate2 统一写法

```toml
[dependencies]
third = { workspace = true }
tokio = { workspace = true }
serde = { workspace = true }
```

| 好处 | 说明 |
|------|------|
| **强制同版** | 不会出现一个要 1.0、一个要 1.5 的混乱 |
| **一处升级** | 改根 `workspace.dependencies`，全部子包同步 |
| **lock 稳定** | 减少解析拉扯、lock 频繁变动 |

> 本质仍是子包**主动声明**依赖，版本从工作区读取 — **不是**根强制注入。

### `[workspace.package]` 元数据同理

根统一 `version` / `authors` / `license` / `edition`，子包 `edition = { workspace = true }` 继承 — 见 [04 元数据](./04-crate-metadata.md)。

### 两种工程化模式

| 模式 | 写法 | 特点 |
|------|------|------|
| **经典** | 各子包 TOML 各自写依赖版本 | lock 仍全局统一；重复多 |
| **现代** | 根 `[workspace.dependencies]` + 子包 `workspace = true` | 版本一处维护，更整洁 |

### 仅根生效、子包写了会被忽略的配置

| 配置 | 用途 |
|------|------|
| **`[profile.*]`** | dev / release / opt-level |
| **`[patch]` / `[replace]`** | 依赖替换、本地补丁 |

→ patch / profile 详 [05 构建配置](./05-build-configuration.md)

---

## 七、工作流与常用命令

### 全局批量（根目录）

```bash
cargo check --workspace
cargo test --workspace
cargo build --release --workspace
cargo doc --workspace
```

### 单独操作某成员

```bash
cargo build -p foo
cd crates/foo && cargo run   # 自动识别上层 workspace
```

### 增量编译逻辑

修改 `crates/foo`：

1. 仅重编译 `foo`  
2. 直接依赖 `foo` 的上层 crate 重新链接  
3. 不依赖 `foo` 的包 **不参与**编译  

---

## 八、`resolver = "2"`（必考）

1. **区分** 普通依赖 / build 依赖 / proc-macro — feature **不**全局乱合并  
2. **平台条件依赖隔离** — Windows 专属 feature 在 Linux 构建时不被强开  
3. 2021 edition 单 crate 默认 resolver 2；**虚拟工作区须手动声明**，否则仍用 v1  

→ 与 Feature 统一：[01 Feature](./01-defining-including-features.md) · [02 库内使用](./02-using-features-in-crate.md)

---

## 九、推荐目录结构

```text
project-root/
├── Cargo.toml          # [workspace] · 可选 workspace.dependencies
├── Cargo.lock          # 全局唯一
├── target/             # 统一产物
├── crates/
│   ├── core-utils/
│   └── database/
└── apps/
    └── server-app/     # bin 入口
```

---

## 十、整体职责一览

| 维度 | 谁负责 |
|------|--------|
| **成员列表** | 根 `[workspace].members` |
| **子包版本号** | 各子包 `[package].version` |
| **依赖声明** | 各子包 `[dependencies]`；版本可继承 `workspace.dependencies` |
| **最终锁定版本** | 根 `Cargo.lock` |
| **根 TOML 本质** | 管成员 + 可选集中版本/元数据；**不属于任何一个 crate** |

---

## 十一、对照阅读

| 资源 | 路径 |
|------|------|
| Book 14.3 | [Cargo 工作空间](../../00-Book/14-cargo-crates/14.3-Cargo工作空间.md) |
| 入门 demo | [14.3-workspace-demo](../../00-Book/14-cargo-crates/14.3-workspace-demo/) |
| HFT 多包示例 | [14.3-hft-workspace-demo](../../00-Book/14-cargo-crates/14.3-hft-workspace-demo/) |
| ER 多 crate | [ER-demos WORKSPACE](../../01-ER/ER-demos/WORKSPACE.md) |

---

## 十二、核心速记

1. **`members` = 子 Package 文件夹路径列表** — 指向文件夹，非 `Cargo.toml` 文件。  
2. **同大版本**（1.0 vs 1.5）→ 求交集，全局一份更高版；**跨大版本**（1 vs 2）→ 冲突报错。  
3. **规范**：`workspace.dependencies` 一处统一，子包 `workspace = true` 继承。  
4. **真要 1.x + 2.x 并存** → 优先改代码升级；不得已拆 Workspace。  
5. **两大共享**：根 `Cargo.lock` · 根 `target/`。  
6. **`resolver = "2"`** — 虚拟工作区须手写。  
7. **`--workspace`** 全局 · **`-p`** 单成员。

→ 速记：lock 详解：[03.1 Cargo.lock](./03-1-cargo-lock.md) · 下一节：[04 Crate 元数据](./04-crate-metadata.md)
