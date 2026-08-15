# 第 1 章 · Welcome（Introduction）

> **Crafting Interpreters** · [Part I · Welcome](../README.md) · [本书目录](../../本书目录.md)  
> 在线：[craftinginterpreters.com](https://craftinginterpreters.com/introduction.html) · [中文在线](https://craftinginterpreters.com/introduction.html)

## 状态

- [x] 已读（笔记整理）

---

## 一句话

第一章不写代码，只做三件事：为什么要学、书怎么读、两趟实现（jlox → clox）预告。读完应能建立信心，并知道 Part I～III 各干什么。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §1.1 | 为什么要学这些（Why Learn This Stuff?） | [01-why-learn-this-stuff.md](./01-why-learn-this-stuff.md) |
| §1.2 | 本书是如何组织的（How the Book is Organized） | [02-how-the-book-is-organized.md](./02-how-the-book-is-organized.md) |
| §1.3 | 第一个解释器（The First Interpreter） | [03-the-first-interpreter.md](./03-the-first-interpreter.md) |
| — | 速记 · 自测 · 进度 |

---

## 逻辑脉络


---

## 速记

## 本章速记

```text
§1.1  DSL 实用 + 通用语言/DSL 对照 + jlox/clox/LLVM 预告 + 没有魔法
§1.2  三 Part · 一章一特性 · Code/Asides/Challenges/Design notes
§1.3  jlox（Java 语义）→ clox（C 性能与底层）
```

---

---

## 读后下一步

| 章 | 目录 | 内容 |
|:--:|------|------|
| **2** | [chapter02 · A Map of the Territory](../../chapter02_map-of-the-territory/) | **编译之山**：扫描、解析、分析、IR、代码生成 |
| **3** | [chapter03 · The Lox Language](../../chapter03_the-lox-language/) | Lox 完整规格（写 jlox 前必读） |
| **4+** | Part II | 从 Scanning 动手 **jlox** |

---

---

## 自测 / Challenges（可选）

- [ ] 举一个你项目里的 **DSL**（配置、模板、宏展开……）。
- [ ] 用自己的话解释：为何先 **jlox** 再 **clox**？
- [ ] 读 Design Note *What's in a Name?*，各举 1 个「好命名 / 差命名」的语言特性。
- [ ] 在 [`02-RFR/学习路径与章节对照.md`](../../../../../02-RFR/学习路径与章节对照.md) 标出：读 CI 时与 RFR 哪几章并行。

---

---

## 阅读进度

- [x] §1.1～§1.3 结构梳理（本章笔记）
- [ ] Design Note *What's in a Name?*
- [ ] 本章 Challenges（若有）

