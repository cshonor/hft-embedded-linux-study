# 5.2 Minimal Dependency Versions（依赖下界）

> 所属：**Versioning** · [← 章索引](./README.md)

← [07 MSRV](./07-msrv.md) · 下一节 [09 Changelog](./09-changelogs.md)

> ER → [Item 25 · 依赖图](../../01-ER/Chapter-04-Dependencies/Item-25-dependency-graph/README.md) · [Item 21 SemVer](../../01-ER/Chapter-04-Dependencies/Item-21-semver/README.md)

---

**核心主题**：库项目须设置**合理最低依赖版本**，避免过度收紧范围引发依赖冲突。

---

## 一、依赖写得过紧的危害

### 错误示例

代码仅用 `serde 1.5.6` 的 API，却写 `serde = "1.7.3"`：

| 后果 | 说明 |
|------|------|
| **压缩解析空间** | 下游若另一 crate 需 `serde 1.6.x` → 冲突、构建失败 |
| **强制无谓升级** | 用户被迫拉高依赖 · MSRV / 编译成本上升 |
| **生态锁死** | 小库互锁高版本 → 依赖地狱 |

### MVS 与 `-Z minimal-versions`

Cargo 默认 **MVS（最大有效版本）** — 在满足约束下选**最高**版本。

**`-Z minimal-versions`** 反转 — 强制选**最低**满足约束的版本，专用于**校验下界是否真实可用**。

---

## 二、标准落地实践（库 crate 规范）

### 1. `Cargo.toml` 书写原则

只写**能通过单元测试、覆盖全部所用 API 的最低兼容版本**：

```toml
# ✅ 只需 1.5.6 的方法
serde = "1.5.6"

# ❌ 无理由抬高
# serde = "1.7.3"
```

| 原则 | 说明 |
|------|------|
| 用到某小版本新 API / 修复 | 才抬高对应下界 |
| 无特殊需求 | 只锁主版本 `tokio = "1"` — 兼容性最强 |

### 2. 探测与校验工具

#### （1）Nightly：`cargo update -Z minimal-versions`

```bash
cargo update -Z minimal-versions
cargo test -Z minimal-versions
```

1. 按约束解析**最小**依赖版本  
2. 全量测试 — 失败说明下界写**低**了，须上调  

> 以当前 Cargo / nightly 文档为准；行为可能演进。

#### （2）`cargo-minimal-versions`

```bash
cargo minimal-versions check --workspace --ignore-private
```

简化 nightly flag · 支持 workspace 批量 · 适合 CI。

### 3. CI 准入（发布型库）

```bash
cargo update -Z minimal-versions && cargo test
```

校验失败 → 禁止合并 / 发布 — 防无意抬高下界。

→ ER [Item 32 CI](../../01-ER/Chapter-05-Tooling/Item-32-ci/README.md)

---

## 三、与 ER Item 25 联动

### 库 vs 二进制

| 类型 | 要求 |
|------|------|
| **库 crate** | **强制**最小下界 — 兼顾所有下游 |
| **二进制应用** | 可固定高版本 / 提交 `Cargo.lock` — 不影响第三方 |

### 版本范围写法

| 场景 | 写法 |
|------|------|
| 通用基础依赖 | `log = "0.4"` — 下界最低 |
| 需特定 API | `serde = "1.5.6"` — 精确最小补丁 |
| **禁止**（库） | `=1.7.3` 精确锁 — 仅二进制临时用 |

### 审计工具

| 工具 | 用途 |
|------|------|
| **`cargo tree`** | 依赖树 · 版本重叠 |
| **`cargo deny`** | 管控版本范围 · 禁过紧约束 |

→ [03 工作区 lock](./03-workspaces.md)

---

## 四、边界场景

| 场景 | 处理 |
|------|------|
| **`0.y.z` 预发布** | Minor 视为 breaking — 下界锁到**最小所需补丁**，不可只写 `0.4` |
| **Feature 可选依赖** | 每个 feature 的 optional dep **单独** minimal-versions 校验 |
| **Workspace 多包** | 每个**可发布** member 独立校验、独立下界 |

→ Feature：[01](./01-defining-including-features.md) · [02](./02-using-features-in-crate.md)

---

## 五、核心速记

1. 版本过高 → 压缩解析空间 → 跨 crate 冲突。  
2. 写**可测试通过的最低**兼容版本。  
3. 校验：`-Z minimal-versions` · `cargo-minimal-versions`。  
4. **仅发布库**强制；二进制无严格限制。  
5. CI 必加最小版本测试。  
6. ER Item 25：合理范围 = 整洁依赖图基础。

---

## 速记

## 痛点

下界过高 → MVS 解析空间缩小 → 跨 crate 冲突

## 规范

写**测试通过的最低**版本 · 无需求只锁主版本 `1`

## 校验

```bash
cargo update -Z minimal-versions
cargo test -Z minimal-versions
# 或
cargo minimal-versions check --workspace --ignore-private
```

## 库 vs bin

库 **强制** · 二进制可锁 lock / 高版本

## 边界

0.y.z 严锁补丁 · feature 可选 dep 单独测 · workspace 每包独立

## CI

minimal-versions + test 失败禁止合并

## 自测

- [ ] MVS 默认选最高还是最低？  
- [ ] 为何库不应写 `serde = "=1.7.3"`？  
- [ ] minimal-versions 失败说明下界太高还是太低？

