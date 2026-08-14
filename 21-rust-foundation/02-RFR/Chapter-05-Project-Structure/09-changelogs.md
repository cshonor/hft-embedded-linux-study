# 5.3 Changelogs（变更日志）

> 所属：**Versioning** · [← 章索引](./README.md)

← [08 依赖下界](./08-minimal-dependency-versions.md) · 下一节 [10 未发布版本](./10-unreleased-versions.md)

> ER → [Item 21 · SemVer](../../01-ER/Chapter-04-Dependencies/Item-21-semver/README.md) · MSRV → [07 MSRV](./07-msrv.md)

---

## 一、核心原则：不依赖 Git log

Git 提交面向开发者 — 碎片化、技术化；使用者无法快速判断升级影响。

须**独立**维护标准化 **CHANGELOG**，遵循 [Keep a Changelog](https://keepachangelog.com/) — 每条版本对用户友好、可检索。

### 核心价值

| # | 价值 |
|:-:|------|
| 1 | 升级前判断：是否有 breaking · 是否改业务代码 |
| 2 | 区分：安全修复 · 功能新增 · 兼容调整 · MSRV |
| 3 | 严格对齐 **SemVer** — 版本与日志一一对应 |
| 4 | 审计、漏洞追溯、下游适配有据可查 |

---

## 二、必须重点突出的 5 类内容

| # | 类型 | 说明 |
|:-:|------|------|
| 1 | **Breaking changes** | API 删除、签名变更、字段调整、行为变更、trait 收紧 — **置顶加粗** + 迁移方案 |
| 2 | **MSRV 提升** | 新最低 Rust · 原因（新语法/API）· 工具链要求 → [07](./07-msrv.md) |
| 3 | **Feature 默认改动** | default / 聚合 feature 变更 — 影响编译体积与依赖树 → [01](./01-defining-including-features.md) |
| 4 | **安全修复** | 内存安全、竞争、越界等 — 标注等级 · 建议立即升级 |
| 5 | **常规** | 新功能 · 性能 · Bug · 文档 |

---

## 三、与 SemVer 对齐（ER Item 21）

| 版本 | 含义 |
|------|------|
| **Major `X.0.0`** | 任意 **Breaking change** — 需大规模适配 |
| **Minor `X.Y.0`** | 向后兼容新增；**生态：抬 MSRV 仅 Minor** — 不升 Major |
| **Patch `X.Y.Z`** | Bug / 安全 / 文档 / 性能 — **无 API 变更、无 MSRV 上调** |

---

## 四、Keep a Changelog 书写规范

1. 固定 **`CHANGELOG.md`** 于仓库根  
2. **倒序** — 新版本置顶  
3. 每版本分区块，例如：  
   - `### Breaking Changes`  
   - `### Features`  
   - `### Fixes / Security`  
   - `### Build / MSRV`  
4. 破坏性变更附**迁移示例**  
5. 待发布 → **`## [Unreleased]`** — 发版时提取为版本号 → [10 未发布版本](./10-unreleased-versions.md)  
6. 可链 PR / commit 溯源  

### 极简模板

```markdown
# Changelog
All notable changes to this project will be documented in this file.

## [Unreleased]
### Breaking Changes
- Remove deprecated `old_api()` function

### Build
- Bump MSRV to 1.72

## [2.1.0] - 2026-06-18
### Features
- Add json serialization behind `serde` feature

## [2.0.0] - 2026-05-01
### Breaking Changes
- Rewrite buffer internal API, all read/write methods signature changed
```

---

## 五、避坑要点

| # | 避坑 |
|:-:|------|
| 1 | 站在**库使用者**视角 — 非 commit 消息堆砌 |
| 2 | **MSRV 不可进 Patch** — 下游无感知编译失败 |
| 3 | **安全修复**单独高亮 |
| 4 | 勿漏 **Feature 默认**变更 |
| 5 | 发布前同步 **README MSRV** 与 CHANGELOG |

---

## 六、核心速记

1. 独立 **CHANGELOG** — Keep a Changelog，不靠 Git log。  
2. 四大高亮：**Breaking · MSRV · Feature 默认 · 安全**。  
3. **SemVer**：Major 破坏 · Minor 新增/MSRV · Patch 修复。  
4. ER Item 21 配套。  
5. **`CHANGELOG.md` + `[Unreleased]`** 预存待发。

---

## 速记

## 原则

独立 **CHANGELOG.md** · Keep a Changelog · 不靠 Git log

## 五类必写

Breaking · MSRV · Feature 默认 · 安全 · 常规

## SemVer

| 版本 | 内容 |
|------|------|
| Major | Breaking |
| Minor | 新功能 / **抬 MSRV** |
| Patch | 修复 / 安全 — **禁 MSRV** |

## 结构

倒序 · 分区块 · `[Unreleased]` 待发 · 迁移示例

## 避坑

用户视角 · MSRV 不进 patch · 安全高亮 · README 同步

## 自测

- [ ] MSRV 应写在 Minor 还是 Patch？  
- [ ] `[Unreleased]` 何时改成版本号？  
- [ ] default feature 变更为何要写 CHANGELOG？

