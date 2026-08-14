# 3.1 Crate Metadata（Crate 元数据）

> 所属：**Project Configuration** · [← 章索引](./README.md)

← [03.1 Cargo.lock](./03-1-cargo-lock.md) · [03 工作区](./03-workspaces.md) · 下一节 [05 构建配置](./05-build-configuration.md)

Book → [14.1 发布到 crates.io](../../00-Book/14-cargo-crates/14.1-发布到-crates-io.md) · ER → [Item 27 公开 API 文档](../../01-ER/Chapter-05-Tooling/Item-27-document-public-api/README.md)

---

## 一、核心定义

**Crate Metadata** = 写在 `Cargo.toml` 里、专门用于**包发布、平台展示、生态检索、开源合规**的元信息 — 与 **编译构建配置**（`[profile]`、`[features]` 等）是**两套逻辑**。

| | Metadata | 构建配置 |
|---|----------|----------|
| **典型块** | `[package]` · `[workspace.package]` | `[profile.*]` · `[patch]` |
| **主要影响** | crates.io / docs.rs **展示与分发** | **二进制产物**（优化、LTO 等） |
| **本地改字段** | 多数**不改变**编译结果 | **直接改变**编译行为 |

> **例外**：`edition` 写在 `[package]` 下，但**会影响编译** — 不是纯展示字段 → [Book 附录 E Editions](../../00-Book/appendix/E.5-Editions.md)

---

## 二、作用边界

### ✅ 主要用途

- 发布库到 **crates.io**
- 仓库页展示：介绍、作者、版本、分类
- 开发者**检索、筛选**第三方库（`keywords` / `categories`）
- **许可证、版权** — 开源合规溯源

### ❌ 不生效范围

**不会驱动本地编译逻辑** — 和 `[profile]` 最大区别：

```toml
[profile.release]
opt-level = 3          # ← 改这里：编译产物变

[package]
description = "…"      # ← 改这里：仅 crates.io 展示变
license = "MIT"
```

Workspace 公共元数据 → 根 `[workspace.package]`，子包 `workspace = true` 继承 → [03 工作区 §六](./03-workspaces.md#六公共依赖复用workspacedependencies推荐)（`[workspace.package]` 与 dependencies 同理）

```toml
# 根 Cargo.toml
[workspace.package]
version = "0.2.0"
authors = ["Team <team@example.com>"]
license = "MIT OR Apache-2.0"
edition = "2021"

# crates/foo/Cargo.toml
[package]
name = "foo"
version.workspace = true
authors.workspace = true
license.workspace = true
edition.workspace = true
description = "foo 专属说明"   # 子包可覆盖或单独写
```

---

## 三、关键字段对照表

| 字段 | 核心用途 |
|------|----------|
| **`name`** | crates.io **唯一**包名 |
| **`version`** | SemVer → [07 MSRV](./07-msrv.md) · [08 依赖下界](./08-minimal-dependency-versions.md) |
| **`authors`** | 作者列表 — 展示与溯源 |
| **`edition`** | Rust edition — **影响编译**（非纯展示） |
| **`description`** | 一句话摘要 — 搜索列表 |
| **`documentation`** | 自定义文档 URL；缺省 → docs.rs |
| **`license`** | 开源协议 — **发布必填** |
| **`repository`** | 源码仓库 — 溯源 |
| **`keywords`** | 检索标签（**最多 5 个**） |
| **`categories`** | crates.io **官方固定分类** |
| **`readme`** | README 路径 — crate 主页 |
| **`include`** | **白名单** — 仅打包列表内文件 |
| **`exclude`** | **黑名单** — 排除路径 |

---

## 四、标准模板（可直接复制）

### 开源发布包

```toml
[package]
name = "my-crate"
version = "0.1.0"
authors = ["Your Name <you@example.com>"]
edition = "2021"
description = "简短功能介绍"
license = "MIT OR Apache-2.0"
repository = "https://github.com/you/my-crate"
keywords = ["network", "async"]
categories = ["network-programming"]
readme = "README.md"
exclude = [".env", "tmp/"]
```

### 私有 / 内部包（可简略）

```toml
[package]
name = "internal-tool"
version = "0.1.0"
edition = "2021"
# description / license / repository 可省略 — 不 publish 即可
```

---

## 五、实操规范

### 发布前检查打包内容

```bash
cargo package --list
```

| 方式 | 用法 |
|------|------|
| **`exclude`** | `exclude = [".env", "secret/"]` |
| **`.cargo_vcsignore`** | 同 `.gitignore` 语法，**专管发布打包** |

---

## 六、学习建议

| 场景 | 建议 |
|------|------|
| **私有内部包** | `name` + `version` + `edition` 即可；不必凑齐 keywords |
| **开源发布** | 补全 `description`、`license`、`repository`、`keywords` — 检索与合规 |
| **Workspace** | 根 `[workspace.package]` 统一 `version` / `authors` / `license` / `edition`；子包只写差异项 |

---

## 七、配套参考

| 资源 | 说明 |
|------|------|
| **Book 14.1** | 登录 · `cargo publish` · 权限 |
| **ER Item 27** | README → crates.io；API 文档 → docs.rs — 与 metadata 配套 |
| **SemVer** | [ER Item 21](../../01-ER/Chapter-04-Dependencies/Item-21-semver/README.md) |
| **章内位置** | Project Configuration · 上一节 [03 工作区](./03-workspaces.md) · 下一节 [05 构建配置](./05-build-configuration.md) |

---

## 八、核心速记

1. Metadata **服务** 发布/展示/合规 — **不**驱动编译（`edition` 除外）。  
2. **`[profile]`** 管编译；**`[package]`** 管身份与平台信息。  
3. Workspace → **`[workspace.package]`** 一处维护，子包 `*.workspace = true`。  
4. 发布前 **`cargo package --list`** 必跑。

---

## 速记

## 本质

发布 / 展示 / 检索 / 合规 — **多数不影响编译**

## vs 构建配置

| Metadata `[package]` | 构建 `[profile]` |
|---------------------|------------------|
| crates.io 展示 | 优化 / 编译开关 |

**例外**：`edition` 在 `[package]` 但**影响编译**

## 身份 & 合规

`name` · `version` · `authors` · `license` · `repository`

## 发现

`description` · `keywords` (≤5) · `categories` · `readme`

## Workspace

根 `[workspace.package]` + 子包 `version.workspace = true` 等

## 场景

私有：name+version+edition · 开源：补 license/repo/keywords

## 发布前

```bash
cargo package --list
```

## 自测

- [ ] metadata 和 `[profile]` 谁影响二进制？  
- [ ] `edition` 算纯展示吗？  
- [ ] 工作区如何统一 authors/license？

