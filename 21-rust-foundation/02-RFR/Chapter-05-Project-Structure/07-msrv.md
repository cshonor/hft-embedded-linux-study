# 5.1 Minimum Supported Rust Version（MSRV）

> 所属：**Versioning** · [← 章索引](./README.md)

← [06 条件编译](./06-conditional-compilation.md) · 下一节 [08 依赖下界](./08-minimal-dependency-versions.md)

> ER → [Item 21 · SemVer](../../01-ER/Chapter-04-Dependencies/Item-21-semver/README.md) · [09 Changelog](./09-changelogs.md)

---

**MSRV** = **Minimum Supported Rust Version** — 项目可正常编译、通过测试的**最低稳定** Rust 编译器版本。

---

## 一、`rust-version` 配置（Rust 1.56+）

```toml
[package]
name = "demo"
version = "2.6.0"
edition = "2021"
rust-version = "1.70"
```

### 约束规则

| # | 规则 |
|:-:|------|
| 1 | 纯版本号（`1.70` / `1.70.0`）— **不能** `^` / `>=` 等 SemVer 运算符 |
| 2 | `rust-version` 须 ≥ 当前 **edition** 要求的最低 Rust |
| 3 | 本地编译器 **低于** MSRV → Cargo **报错**阻止编译 |
| 4 | Cargo **1.85+**（2024 edition 生态）：**MSRV 感知依赖解析** — 为旧工具链自动匹配兼容依赖版本 |

### MSRV 契约

| | |
|---|---|
| **承诺** | ≥ MSRV 的稳定 Rust 可编译、通过测试 |
| **不保证** | < MSRV 编译器 — 报错属**预期** |

---

## 二、MSRV 与 SemVer（生态约定）

### 核心规则

**提升 MSRV = Minor（次版本）变更 — 不算 Major 破坏性变更**

```text
2.6.0 → 2.7.0   ✅ 抬高 MSRV
2.6.0 → 3.0.0   ❌ 不必为此跳 major（除非另有 breaking API）
```

#### 底层逻辑（ER Item 21）

1. Rust 稳定编译器**不破坏**向后语法兼容（在 MSRV 承诺范围内）  
2. 新版 Cargo MSRV 解析器为旧工具链匹配旧依赖  
3. 用户可锁依赖 **` < 2.7.0`**，继续在旧 Rust 上使用  

### 行业策略

| 类型 | 策略 |
|------|------|
| **通用库**（tokio 等） | 滚动 ~6 个月窗口，需求驱动，minor 抬 MSRV |
| **基础设施**（hyper 等） | 保守 ~3 年，极少抬 MSRV |

### 禁止行为

**禁止在 Patch 版本抬 MSRV**

```text
2.6.0 → 2.6.1 抬高 MSRV   ❌ 严重生态破坏
```

无感知 patch 升级导致下游编译崩溃。

---

## 三、标准化落地（工程流程）

### 1. 文档声明

- `Cargo.toml` → **`rust-version`**  
- README 顶部标注 MSRV  

### 2. CI 校验（必做）

CI 用**声明的 MSRV** 跑完整 `cargo test`。

```bash
# cargo-msrv：探测 / 校验真实 MSRV
cargo install cargo-msrv
cargo msrv verify
```

→ ER [Item 32 CI](../../01-ER/Chapter-05-Tooling/Item-32-ci/README.md)

### 3. 版本变更记录

抬高 MSRV 时：

1. **次版本 +1**（`2.6 → 2.7`）  
2. **CHANGELOG** 单独条目：新 MSRV · 升级原因（新语法/API）· 下游影响与兼容方案  

→ [09 Changelog 规范](./09-changelogs.md)

---

## 四、配套工具与工作区

| 项 | 说明 |
|----|------|
| **Workspace** | 根 / 各 member 可独立 `rust-version`；解析取依赖链**最高** MSRV 为实际下限 → [03](./03-workspaces.md) |
| **`cargo-msrv`** | 遍历多版 rustc，找真实最低可编译版本 — 初始化 / 重构后修正声明 |
| **`cfg` 隔离** | 高版本特性条件编译 — 平滑过渡（临时兼容旧 MSRV）→ [06](./06-conditional-compilation.md) |

---

## 五、核心速记

1. **`rust-version`** 声明最低编译器。  
2. 抬 MSRV → **Minor**；**禁止 Patch** 改动。  
3. ER Item 21：MSRV 升级 ≠ SemVer major breaking。  
4. 落地：**TOML + README + CI 固定 MSRV**。  
5. CHANGELOG 记录原因。  
6. Cargo 1.85+ MSRV 感知解析 — 旧工具链可锁依赖上限。

---

## 速记

## 定义

最低稳定 Rust 可编译版本 · `rust-version = "1.70"`

## 规则

纯版本号 · ≥ edition 最低 · 低于 MSRV Cargo 报错

## SemVer 生态

抬 MSRV → **minor**（2.6→2.7）· **禁 patch**（2.6.1）

## 落地三步

TOML · README · CI 固定 MSRV · `cargo msrv verify`

## CHANGELOG

新 MSRV · 原因 · 下游影响

## Workspace

各 member 可独立 · 依赖链取**最高** MSRV

## 自测

- [ ] 为何 2.6.1 抬 MSRV 是生态破坏？  
- [ ] MSRV 升级为何通常不算 major？  
- [ ] `cargo-msrv verify` 测什么？

