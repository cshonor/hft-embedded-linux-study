# Ch 20 补丁、开发和社区 · Patches, Hacking, and the Community

> **Linux Kernel Development 3rd** · Robert Love · **跳过**

> 本章定位：**LKML**、**Linux 编码风格**、**MAINTAINERS**、bug 报告、**git format-patch** 提交流程 — 从读者到贡献者的收官页。

---

## 本节结构

| 节 | 主题 | 带走什么 |
|----|------|----------|
| **① 社区** | LKML | 订阅/读归档 |
| **② 编码风格** | Linus style | Tab 8 · K&R · 80 列 |
| **③ 指挥链** | MAINTAINERS | 子系统维护者 |
| **④ Bug 报告** | oops · 复现 | 抄送维护者+LKML |
| **⑤ 补丁** | diff · git | inline · `[PATCH]` |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 社区 | [notes/section-20.1-社区.md](./notes/section-20.1-社区.md) |
| Linux 编码风格 | [notes/section-20.2-Linux-编码风格.md](./notes/section-20.2-Linux-编码风格.md) |
| 指挥链 | [notes/section-20.3-指挥链.md](./notes/section-20.3-指挥链.md) |
| 提交错误报告 | [notes/section-20.4-提交错误报告.md](./notes/section-20.4-提交错误报告.md) |
| 补丁 | [notes/section-20.5-补丁.md](./notes/section-20.5-补丁.md) |
| 从读到贡献（全书闭环） | [notes/section-20.6-从读到贡献全书闭环.md](./notes/section-20.6-从读到贡献全书闭环.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 主论坛？ | **LKML**（及子系统列表） |
| 缩进？ | **Tab（逻辑 8 列）** |
| 找谁审？ | **`MAINTAINERS`** |
| Bug 报告？ | 症状 + Oops + 复现 + 硬件 |
| 怎么出补丁？ | **`git format-patch`** 优于裸 diff |
| 邮件主题？ | **`[PATCH] ...`** · 大改 **拆分** |

---

## 本章学习目标 · 自检

- [ ] 知道 **LKML / lore** 是观察开发的主渠道
- [ ] 能说出 **Tab 8、K&R、80 列** 三条
- [ ] 会在 **`MAINTAINERS`** 里找维护者
- [ ] Bug 报告含 **Oops 全文 + 复现步骤**
- [ ] 会用 **`git format-patch`** 生成可邮件提交的补丁
- [ ] 理解 **`[PATCH v2]`** 与 review 迭代

---

## 相关章节

- 上一章：[../chapter-19-portability/](../chapter-19-portability/)
- 全书导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
