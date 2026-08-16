# 5.4 Unreleased Versions（未发布版本）

> 所属：**Versioning** · [← 章索引](./README.md)

← [09 Changelog](./09-changelogs.md) · 下一节 [11 小结](./11-summary.md)

前置 → [09 Changelog](./09-changelogs.md) · ER → [Item 21 SemVer](../../01-ER/Chapter-04-Dependencies/Item-21-semver/README.md)

---

## 一、`[Unreleased]` 区块（Keep a Changelog）

### 核心作用

日常开发改动统一记在 CHANGELOG **顶部 `[Unreleased]`** — 集中累积待发布内容，不必每次开发新建版本区块。

**正式发布时**：整块迁移为 `## [X.Y.Z] - YYYY-MM-DD`，清空 `[Unreleased]` 进入下一轮。

### 标准模板

```markdown
# CHANGELOG.md

## [Unreleased]
### Added
- 新增缓冲区批量写入接口
### Changed
- 默认关闭 `async` feature
### Build
- MSRV 提升至 1.73
### Breaking Changes
- 废弃旧版同步读取 API

## [1.2.0] - 2026-01-01
### Fixes
- 修复内存越界问题
```

### 操作流程

| 步骤 | 动作 |
|------|------|
| 1 | 开发提交 → 改动追加到 `[Unreleased]` 对应分类 |
| 2 | 发版前 → 复制 Unreleased 内容，新建 `[X.Y.Z] - 日期` 区块 |
| 3 | 清空 `[Unreleased]`，保留空分类模板 |

→ 详 [09 Changelog §四](./09-changelogs.md)

---

## 二、SemVer 预发布（alpha / beta / rc）

### 格式

`主.次.补丁-预发布标记.序号`

| 版本 | 含义 |
|------|------|
| **`1.0.0-alpha.1`** | 早期内测，功能未完整 |
| **`1.0.0-beta.2`** | 公开测试，功能完整，仍有潜在 bug |
| **`1.0.0-rc.1`** | 候选发布 — 无严重问题即转 **`1.0.0`** |

### 版本优先级

```text
1.0.0-alpha  <  1.0.0-beta  <  1.0.0-rc  <  1.0.0
```

### 依赖预发布 crate

Cargo **默认**只匹配**正式稳定**版本 — 须**显式**写预发布号：

```toml
# ✅ 明确预发布
demo = "2.0.0-rc.1"

# ❌ "2.0.0" 只匹配正式 2.0.0，忽略 rc/beta
demo = "2.0.0"
```

---

## 三、与 Git 协作的标准发版流程

### 发版 PR 三步（必须同 PR 完成）

| # | 步骤 |
|:-:|------|
| 1 | **`Cargo.toml`** — `[package].version` 改为目标发布版 |
| 2 | **CHANGELOG** — `[Unreleased]` → 新版本区块 + 日期；清空 Unreleased |
| 3 | **合并后打 tag** — 如 `v1.2.0`，与 crate 版本一致 |

```bash
git tag v1.2.0
git push origin v1.2.0
```

### 避坑

| 错误 | 后果 |
|------|------|
| **先打 tag 未改 Cargo.toml / CHANGELOG** | tag 与代码版本不一致 |
| | crates.io 与 git tag 对不上 |
| | CHANGELOG 缺该版本记录 |

→ 发布流程 Book [14.2 发布到 crates.io](../../00-Book/14-cargo-crates/14.2-将crate发布到Crates.io.md)

---

## 四、核心速记

1. **`[Unreleased]`** 累积待发 · 发版迁移为带日期正式版。  
2. 预发布：`-alpha` / `-beta` / `-rc` — 优先级递增。  
3. 依赖预发布 crate **须显式**写预发布版本号。  
4. 发版：**Cargo 版本 → CHANGELOG → PR → tag**。  
5. **禁止** tag 与代码内版本不同步。

---

## 速记

## [Unreleased]

顶部累积 · 发版 → `[X.Y.Z] - 日期` · 清空重来

## 预发布

`alpha` < `beta` < `rc` < 正式

## 依赖预发布

必须写 `2.0.0-rc.1` — `"2.0.0"` 不匹配 rc

## 发版三步

1. bump `Cargo.toml` version  
2. 迁移 CHANGELOG  
3. merge → `git tag vX.Y.Z`

## 禁

先 tag 后改 version / CHANGELOG

## 自测

- [ ] 为何 `"2.0.0"` 拉不到 `2.0.0-rc.1`？  
- [ ] 发版 PR 应同时改哪两个文件？  
- [ ] `[Unreleased]` 发版后如何处理？

