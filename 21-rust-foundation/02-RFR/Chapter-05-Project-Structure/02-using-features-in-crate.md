# 1.2 Using Features in Your Crate（crate 内使用 Feature）

> 所属：**Features** · [← 章索引](./README.md)

← [01 定义与包含 Feature](./01-defining-including-features.md) · 下一节 [03 工作区](./03-workspaces.md)

前置 → [01 Additive 原则](./01-defining-including-features.md) · [06 条件编译](./06-conditional-compilation.md)

> ER → [Item 26 · Feature Creep](../../01-ER/Chapter-04-Dependencies/Item-26-feature-creep/README.md) · demo：[Item 26 demo](../../01-ER/Chapter-04-Dependencies/Item-26-feature-creep/demo/)

---

**底层规则**：Cargo Feature 是**加法模型（additive）** — **Feature Unification**：依赖图任意一处开启某 feature，对该 crate **全局生效**，**无法局部关闭**。

```text
代码 #[cfg]  ·  Cargo.toml 绑定  ·  文档 & CI  ·  反模式避坑
```

---

## 一、代码侧：`#[cfg(feature = "...")]` 条件编译

### 1. 完整可选模块隔离（推荐）

```rust
#[cfg(feature = "serde")]
mod serde_impl {
    use serde::{Deserialize, Serialize};
    // impl Serialize / Deserialize for MyData …
}

#[cfg(feature = "serde")]
pub use serde_impl::*;
```

### 2. 单个函数隔离

```rust
#[cfg(feature = "serde")]
pub fn to_json(&self) -> String {
    /* … */
}
```

### 3. 派生属性条件启用

```rust
#[derive(Debug)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Config;
```

### 编码硬性规范

| # | 规范 |
|:-:|------|
| 1 | 所有依赖可选库的代码**必须** `cfg` 守卫 — 无守卫 → 无 feature 时编译失败 |
| 2 | 同类 feature 逻辑**收拢到独立 mod**，勿零散散布 |
| 3 | 组合判断用 `all` / `any` / `not` |

```rust
#[cfg(all(feature = "serde", feature = "std"))]
fn json_std() {}
```

→ [06 条件编译](./06-conditional-compilation.md)

---

## 二、Cargo.toml 依赖侧规范

### 现代 `dep:` 模板

```toml
[dependencies]
serde = { version = "1.0", optional = true }

[features]
serde = ["dep:serde"]
full = ["serde", "async", "compression"]
std = []                                    # 纯标记，仅 cfg
async = ["dep:tokio", "tokio/macros"]       # 传递开启依赖的 feature
```

### 关键约束

| 约束 | 说明 |
|------|------|
| **禁止冲突语义** | A、B 同时开会破坏不变量 → 设计缺陷 |
| **`default` 设计** | 底层库常 `default = []`；改 default 属 **SemVer 破坏性** |
| **显式 `dep:xxx`** | 社区标准，可读性强 |
| **用途边界** | 仅功能模块化 — 不用 feature 控版本降级 / 全局开关 |

→ 定义侧：[01 定义与包含](./01-defining-including-features.md)

---

## 三、文档与 CI 工程规范

### 文档

1. README / crate 级 rustdoc **列出所有 feature** — 作用、依赖、组合关系  
2. 标注 `default` / `full` 包含内容  
3. 条件 API 文档写 **`Requires feature "x"`**

### CI 矩阵（强制两类）

| 命令 | 验证 |
|------|------|
| `cargo check --no-default-features` | 最小核心可编译 |
| `cargo check --all-features` | 多 feature 并集无冲突 |
| （可选）逐个 `--features foo` | 单特性不断裂 |

```bash
cargo check --no-default-features
cargo check --all-features
cargo test --features serde
```

→ ER [Item 32 CI](../../01-ER/Chapter-05-Tooling/Item-32-ci/README.md)

---

## 四、核心反模式

### 互斥后端 A / B（本章重点）

**问题**：依赖图不同分支分别开 A、B → Cargo **并集**同时启用 → 符号冲突 / 逻辑错乱；下游无法控制。

**解决方案（优先级）**：

| # | 方案 |
|:-:|------|
| 1 | **拆独立 crate** — 主库统一 trait，用户按需引后端 |
| 2 | **运行时选择** — 允许双 feature 共存，配置选后端 |
| 3 | **兜底** — 编译期 `compile_error!`（临时兼容） |

```rust
#[cfg(all(feature = "backend_a", feature = "backend_b"))]
compile_error!("backend_a and backend_b are mutually exclusive; enable only one");
```

### 其他反模式

- Feature 粒度过细、碎片化  
- `default` 塞重型依赖  
- 跨 feature 耦合、`--all-features` 未进 CI  
- 不写文档  

→ ER [05-pitfalls](../../01-ER/Chapter-04-Dependencies/Item-26-feature-creep/05-pitfalls.md)

---

## 五、Feature Unification 原理

1. **加法模型** — feature 只有开启，无关闭；依赖树**统一合并**  
2. 可选依赖仅在对应 feature 开启时下载 / 链接  
3. `#[cfg(feature)]` 守卫的代码，关闭时**不进入二进制**  
4. **无法**在局部依赖中关闭上层已开的 feature → 互斥 feature 设计天然有缺陷  

→ 依赖图：[ER Item 25](../../01-ER/Chapter-04-Dependencies/Item-25-dependency-graph/README.md)

---

## 六、核心总结

1. Feature **加法合并**，全局生效，无法局部关闭。  
2. 代码：`#[cfg(feature)]` 收拢到独立 mod。  
3. TOML：`optional = true` + `dep:xxx`。  
4. CI：`--no-default-features` + `--all-features`。  
5. 最大反模式：互斥后端 → **拆包 / 运行时选择**。  
6. 条件 API 文档标注所需 feature。


```bash
cd 01-ER/Chapter-04-Dependencies/Item-26-feature-creep/demo && cargo test --all-features
```

---

## 速记

## 底层

**加法模型 · Feature Unification · 全局合并 · 无法局部关闭**

## 代码模板

```rust
#[cfg(feature = "serde")]
mod serde_impl { /* … */ }

#[cfg_attr(feature = "serde", derive(Serialize))]
```

收拢 mod · 全 guarded · `all/any/not`

## TOML 模板

```toml
serde = { optional = true }
[features]
serde = ["dep:serde"]
full = ["serde", "async"]
std = []
```

## CI 必跑

```bash
cargo check --no-default-features
cargo check --all-features
```

## 互斥后端 → 别用 feature

1. 拆 crate  
2. 运行时选  
3. `compile_error!` 兜底  

## 背诵六点

加法合并 · cfg mod · dep:xxx · 双 CI · 禁互斥后端 · 文档标 feature

## 自测

- [ ] 为何下游无法「只关 serde 不关 tokio 的 serde feature」？  
- [ ] `--all-features` 测什么失败场景？  
- [ ] `std = []` 空 feature 有什么用？

