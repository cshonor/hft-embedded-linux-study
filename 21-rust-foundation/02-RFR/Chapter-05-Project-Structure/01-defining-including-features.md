# 1.1 Defining and Including Features（定义与包含 Feature）

> 所属：**Features** · [← 章索引](./README.md)

← [章索引](./README.md) · 下一节 [02 crate 内使用 Feature](./02-using-features-in-crate.md)

> ER → [Item 26 · Feature Creep](../../01-ER/Chapter-04-Dependencies/Item-26-feature-creep/README.md)

---

## 一、核心定义

**Feature** = 传给 Cargo **依赖解析器** + Rust **编译器**的**构建开关**，用于：

1. 控制是否编译**可选依赖**  
2. 控制代码内 **`#[cfg(feature = "xxx")]`** 条件编译分支  
3. 对外提供功能子集 — 用户按需开启，减少编译体积  

---

## 二、依赖 `features` 与自有 `[features]`（别混）

### 写法 1：写在 `[dependencies]` 里（最常用）

```toml
[dependencies]
serde = { version = "1.0", features = ["derive", "json"] }
```

引入 `serde`，并开启其 **`derive`**、**`json`** 两个 feature — **完全合法**。

**适用**：你是**使用者**，固定需要某些第三方能力，直接写在依赖上。

### 写法 2：自有 `[features]` 中转引

```toml
[dependencies]
serde = { version = "1.0", optional = true }

[features]
serde-full = ["dep:serde", "serde/json", "serde/derive"]
```

`serde` 为可选依赖；下游开你包的 **`serde-full`** → 自动启用 `serde` 并打开其内部 feature。

**适用**：你是**库作者**，对外封装一键能力组合。

| 写法 | 角色 |
|------|------|
| `dependencies` + `features = [...]` | 使用者 / 固定开启 |
| 自有 `[features]` + `dep:` / `serde/xxx` | 库作者 / 对外封装 |

### 分清两种完全不同的 Feature

| | **第三方 crate 的 feature** | **你自己包的 `[features]`** |
|---|------------------------------|-------------------------------|
| 例子 | `serde/json`、`tokio/macros` | 本文件 `[features]` 段 |
| 开启方式 | `dependencies` 里 `features = []` 或 `serde/json` 语法 | 下游 `features = ["your_feat"]` |
| 代码侧 | 第三方库内部 `#[cfg]` | 你方 `#[cfg(feature = "xxx")]` |

> **使用者**可随意增减依赖的 feature — **无**附加性约束。  
> **互斥禁止**只约束**库作者设计自己包的 `[features]`** → 见下节。

---

## 三、附加性 Additive 原则（硬性规范）

所有 Feature 只能**叠加新增功能**，**不能**做破坏性修改。

### 禁止行为

1. 删掉原有模块、结构体、函数  
2. 替换已公开的类型、修改函数入参 / 返回签名  
3. 两个 Feature **互斥**（开 A 就不能开 B）  

### 底层原因

Cargo 对同一库的所有 Feature 做**并集合并**，只编译**一份**代码。  
互斥 Feature 或修改同一段代码 → 合并后冲突 → 编译失败。

> **设计思路**：任意多个 Feature 同时开启，代码必须**合法可编译**。

→ 详 [02 crate 内使用](./02-using-features-in-crate.md) · demo：[Item 26 demo](../../01-ER/Chapter-04-Dependencies/Item-26-feature-creep/demo/)

---

## 四、可选依赖写法

### 1. 声明可选依赖

```toml
[dependencies]
# 默认不自动安装、编译
serde = { version = "1.0", optional = true }
```

`optional = true` — 标记可选，默认不拉取，缩短基础编译时间。

### 2. 把依赖绑定成 Feature

```toml
[features]
# 开启 serde 特性 = 启用可选依赖 serde
serde = ["dep:serde"]
```

`dep:xxx` — Cargo 专用语法：「该 feature 启用名为 `xxx` 的可选依赖」。

### 3. `default` 默认特性

```toml
[features]
default = ["serde"]
serde = ["dep:serde"]
```

下游引用时**默认**开启 `serde`。轻量化编译：

```toml
# 下游 Cargo.toml
mylib = { version = "0.1", default-features = false, features = [] }
```

| 选项 | 含义 |
|------|------|
| `default-features = false` | 关闭全部默认特性 |
| `features = ["serde"]` | 只手动开启需要的子集 |

---

### 互斥反例（库作者 — 错误设计）

```toml
# ❌ 糟糕：设计成互斥
[features]
single_thread = []
multi_thread = []
```

上游若同时需要两者 → Cargo 并集同时开启 → 编译失败。

**正确**：所有自有 feature 只**新增**代码，任意组合可编译。

---

## 五、与代码侧配合（预览）

```toml
# Cargo.toml（库作者）
[dependencies]
serde = { version = "1", optional = true }

[features]
default = ["serde"]
serde = ["dep:serde"]
```

```rust
// lib.rs
#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

#[derive(Debug)]
pub struct Record {
    pub data: Vec<u8>,
}

#[cfg(feature = "serde")]
impl Record {
    pub fn to_json(&self) -> Result<String, serde_json::Error> {
        serde_json::to_string(self)
    }
}
```

→ 详 [02 crate 内使用](./02-using-features-in-crate.md) · demo：[Item 26 demo](../../01-ER/Chapter-04-Dependencies/Item-26-feature-creep/demo/)

---

## 六、ER Item 26：Feature Creep（特性泛滥）

**忠告**：不要无限堆砌细碎小 Feature。

| 问题 | 后果 |
|------|------|
| Feature 过多 | 下游难选、配置复杂 |
| 依赖树膨胀 | 编译时间变长 |
| 大量 `cfg(feature)` | 可读性下降 |

**建议**：功能相近的能力**合并**成一个大 Feature，避免碎片化开关。

---

## 七、核心速记

1. **`dependencies` + `features = [...]`** 合法 — 使用者最常用。  
2. **自有 `[features]`** 封装能力；`dep:` / `serde/xxx` 联动第三方。  
3. **两种 feature 别混**：别人的 vs 自己的。  
4. **互斥禁令**只针对库作者设计自有 feature；使用者无此限。  
5. `optional` + `dep:` + `default` — 见 §四。

---

## 速记

## 两种写法

```toml
# 使用者：依赖上直接开
serde = { version = "1", features = ["derive"] }
# 库作者：自有 feature 中转
serde-full = ["dep:serde", "serde/derive"]
```

## 两种 Feature 别混

第三方 `serde/json` · 自己的 `[features]` 段

## Additive 原则（仅库作者设计自有 feature）

只加不删 · 不改公开 API · **禁止互斥**（Cargo 取并集）

## TOML 三板斧

```toml
serde = { optional = true }
[features]
default = ["serde"]
serde = ["dep:serde"]
```

## 下游精简

`default-features = false` + 手动 `features = [...]`

## Feature Creep

少而粗 · 合并相近能力 · 少 `cfg` 污染

## 自测

- [ ] `dependencies` 里 `features = []` 和自有 `[features]` 差在哪？  
- [ ] 互斥禁令约束使用者还是库作者？  
- [ ] `serde-full = ["dep:serde", "serde/derive"]` 各段含义？

