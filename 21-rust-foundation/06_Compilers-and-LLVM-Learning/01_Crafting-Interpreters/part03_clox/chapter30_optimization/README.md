# 第 30 章 · Optimization（优化）

> **Crafting Interpreters** · [Part III · clox](../README.md) · [本书目录](../../本书目录.md)  
> 在线：[craftinginterpreters.com](https://craftinginterpreters.com/optimization.html) · [中文在线](https://craftinginterpreters.com/optimization.html)

## 状态

- [x] 已读（笔记整理）

---

## 一句话

全书 Lagniappe（额外赠礼） · 最后一章含新代码。用 Benchmark + Profiler 找热点，实施两项 底层微优化 —— 不改 Lox 语义，只压榨 VM 实现。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §30.1～ | §30.2 哈希表探测优化（Hash Table Probe Optimization） | [01-hash-table-probe-optimization.md](./01-hash-table-probe-optimization.md) |
| §30.3 | NaN 装箱（NaN Boxing） | [02-nan-boxing.md](./02-nan-boxing.md) |
| ·4 | 优化方法论（全书收束） | [03-optimization.md](./03-optimization.md) |
| ·5 | 全书 30 章能力地图 | [04-optimization-30.md](./04-optimization-30.md) |
| — | 速记 · 自测 · 进度 |

---

## 逻辑脉络


---

## 速记

## 本章速记

```text
§30.1–2  cap=2^n → index = hash & (cap-1) · ~2× on hash-heavy
§30.3     NaN box · 64-bit Value · ptr/bool/nil in NaN space
方法      benchmark → profile → peephole（ch28 invoke 同理）
```

---

---

## 读后延伸

| 方向 | 链接 |
|------|------|
| **LLVM 优化** | [04_Learn-LLVM-17](../../../04_Learn-LLVM-17/) |
| **编译器 SSA** | [02 编译器工程](../../../02_Compiler-Principles/) |
| **附录** | [Appendix I Lox 语法](../../backmatter/appendix01_lox-grammar/) |
| **Wrappers** | [ch29 原书 REPL/API](https://craftinginterpreters.com/wrappers-and-api.html) |

---

---

## 自测

1. 为何 `& (capacity-1)` 要求 capacity 是 2 的幂？
2. NaN boxing 后 `IS_NUMBER` 如何判断一个 `uint64_t`？
3. 若不做 NaN box，栈 256 深时多浪费多少字节（相对 8B slot）？

---

---

## 阅读进度

- [x] §30.1～§30.3 结构梳理（本章笔记）
- [x] **Crafting Interpreters 正文 30 章笔记链路贯通**
- [ ] 跑书中 benchmark · 对比 mask / NaN 前后
- [ ] 本章 Challenges · 全书 Challenges 复盘

