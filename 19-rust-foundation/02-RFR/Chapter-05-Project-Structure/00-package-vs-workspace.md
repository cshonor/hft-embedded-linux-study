# 0. Package vs Workspace（单包 lib+bin 与多包工作区）

> 第 5 章基础辨析 · [← 章索引](./README.md) · 下一节 [01 Feature](./01-defining-including-features.md)

前置 → Book [1.3 Hello Cargo](../../00-Book/01-getting-started/1.3-Hello-Cargo.md) · [7.3.1 跨 Package 路径](../../00-Book/07-packages-modules/7.3.1-跨Package路径与Workspace依赖.md)

---

## 一、Package vs Crate（先掰清楚）

| 概念 | 含义 |
|------|------|
| **Package（包）** | 整个 Cargo 项目 — **一个 `Cargo.toml`** |
| **Crate（编译单元）** | 编译最小单位 — 一个 Package 可含**多个** crate |

### 一个 Package 里可以有哪些 crate？

| 类型 | 典型入口 | 产物 |
|------|----------|------|
| **二进制 crate** | `src/main.rs` | **`.exe`** |
| **库 crate** | `src/lib.rs` | **`.rlib`** 等 — 供依赖 |
| **更多** | `[[bin]]`、`[lib]`、examples、tests | 多个 bin/lib |

### 关键结论

```text
cargo new xxx  →  1 个 Package，默认 1 个 bin crate（main）
加 src/lib.rs  →  同一 Package 内 lib + bin = 2 个 crate
```

> **一个 Package ≠ 只能一个 crate** — 只是新手默认只有一个 bin。

| 误区 | 正解 |
|------|------|
| 文件夹 + `mod.rs` = 独立 crate | 单 Package 内只是 **mod** |
| lib 收拢 mod = Workspace | 仍是 **单 Package** |

---

## 二、单 Package：lib + bin

`main.rs` 可直接 `use myproj::xxx;` — **无需**在 TOML 为同包 lib 写依赖。

**实践**：lib 收拢业务 `mod`；main 只做入口 — **不是 Workspace**。

```text
myproj/
├── Cargo.toml
└── src/
    ├── lib.rs          # 库 crate：pub mod db; pub mod utils;
    ├── main.rs         # bin crate
    ├── db/mod.rs       # ← 同一 lib 内的 mod，非独立 crate
    └── utils/mod.rs
```

改 `utils` → 常重编**整个 lib** → 趋向**巨石单 lib crate**。

---

## 三、巨石单 Crate（Monolithic）的利弊

**定义**：几乎所有代码塞在**一个 lib 或一个 bin crate** 里（`mod` 再分子目录仍属同一 crate）。

### 优点

- 结构简单 — 无 workspace、多子 crate 版本/循环依赖烦恼  
- **几千行以内**小工具、Demo 完全够用  

### 五大缺陷（大项目痛点）

| # | 问题 | 说明 |
|:-:|------|------|
| 1 | **编译慢** | 改小函数也常触发**整个 crate**重编；多 crate 只重编变更包 |
| 2 | **边界弱** | 仅靠 `mod`/`pub` — 易乱调用；多 crate **不 pub 则完全不可见** |
| 3 | **耦合重** | 外人想用一小段工具须依赖**整个巨石** + 全部传递依赖 |
| 4 | **测试/feature/版本臃肿** | feature creep；整体一个版本号，无法组件独立发版 |
| 5 | **协作冲突** | 多人改同一巨大 crate → git 冲突多 |

→ Feature 膨胀：[01 Feature](./01-defining-including-features.md) · ER [Item 26](../../01-ER/Chapter-04-Dependencies/Item-26-feature-creep/README.md)

---

## 四、Workspace：多个独立 Package

顶层 **`[workspace]`** + 子目录各 **独立 `Cargo.toml`**。

```text
my_workspace/
├── Cargo.toml               # members 列表
├── Cargo.lock
├── crates/core-utils/       # Package · 独立版本/依赖
├── crates/database/
└── apps/server-app/         # bin 入口 Package
```

```toml
[workspace]
members = ["crates/core-utils", "crates/database", "apps/server-app"]

# apps/server-app/Cargo.toml
[dependencies]
core-utils = { path = "../../crates/core-utils" }
```

→ 详 [03 工作区](./03-workspaces.md)

---

## 五、架构对比

| | **单 Package / 巨石 lib** | **Workspace 多 Package** |
|---|---------------------------|--------------------------|
| Package | **1** | **多个** |
| `db/` 文件夹 | lib 内 **mod** | 常 **独立 Package** |
| 编译 | 改模块 → 重编整个 lib | 只重编**变更子包** |
| 隔离 | `pub` 自觉 | **crate** 级强制 |
| 适合 | 小项目 · 几千行 | 上万行 · 团队 · 分层 |

---

## 六、exe 从哪来？（常见疑问）

1. **无论巨石还是 workspace**，最终运行的 **exe 都来自某一个二进制 crate**。  
2. **exe = 运行入口** — 可链接 workspace 内多个已编译的**库 crate**。  
3. **巨石**：逻辑全在一个 crate 里编进 exe。  
4. **模块化**：exe 薄薄一层；业务/工具/底层各为独立 lib crate，**分别编译后链接**成一个 exe。

```text
单巨石：     [ one big lib + main ] → exe
Workspace：  [ core ] [ db ] [ biz ] + thin main → link → exe
```

---

## 七、选用建议

| 规模 | 建议 |
|------|------|
| 几千行以内 · 工具 · Demo | **单 Package**（lib + main）即可 |
| 上万行 · 长期维护 · 团队协作 | **Workspace** 拆多个小 crate |

---

## 八、一句话速记

- **Package** = 一个 `Cargo.toml`；可有 **多个 crate**（bin/lib/…）。  
- **巨石单 crate** 简单但大项目编译慢、边界差、耦合高。  
- **exe** 只来自 **bin crate**；内部组织不影响「只有一个 exe」，影响的是**编译效率与架构管控**。  
- **mod.rs** = crate 内模块；**子目录 Cargo.toml** = 独立 Package。

---

## 速记

## Package vs Crate

1 Cargo.toml = 1 **Package** · 可含多 **crate**（bin/lib/[[bin]]/tests）

`cargo new` → 1 Package · 1 bin · 加 lib.rs → +1 lib crate

## 巨石单 crate

全塞一个 lib/bin · 简单 · 大项目：编译慢 · 边界弱 · 耦合 · feature 臃肿

## exe

永远来自 **bin crate** · 可链接多个 lib crate · 巨石 vs workspace 都有 1 个 exe

## mod vs Package

`mod.rs` = crate 内 · 子目录 `Cargo.toml` = 独立 Package

## 选型

≤几千行单 Package · 上万行/团队 → Workspace

## 自测

- [ ] 改 utils/mod 重编整个 lib 还是只重编 utils？  
- [ ] workspace 下 exe 链接几个 crate？  
- [ ] 为何外人不能只依赖你巨石里一个小工具函数？

