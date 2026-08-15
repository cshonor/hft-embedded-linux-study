# 第 2 章 · A Map of the Territory（领域地图） · 本章定位

← [本章目录](./README.md) · 下一节：[01-the-parts-of-a-language.md](./01-the-parts-of-a-language.md)

---

第二章**仍不写实现代码**，而是给出**高层架构图**：从人类可读的**源代码**到**机器执行**，中间要经过哪些阶段。作者用 **「攀登山峰再下山」** 比喻整条流水线（与封面「编译之山」一致）。

| 小节（原文结构） | 主题 |
|------------------|------|
| **§2.1** The Parts of a Language | 流水线各站：Scanning → Parsing → IR → Code Gen → VM → Runtime |
| **§2.2** Shortcuts and Alternate Routes | 单遍编译、树遍历解释、转译——不必走完整「上山又下山」 |

读完本章 = 拥有**语言实现全局观**；Part II 起将逐一攻克山上的节点。

---
