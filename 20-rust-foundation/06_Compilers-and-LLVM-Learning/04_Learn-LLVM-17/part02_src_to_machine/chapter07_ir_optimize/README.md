# Chapter 07 — IR 优化流水线

| 项目 | 说明 |
|------|------|
| **学习策略** | **精读（重点）** |
| **对应书** | 《Learn LLVM 17》第 7 章 |

## 本目录用途

- 理解 **Pass**、常量折叠、DCE、与**指令重排**相关现象。  
- **与狗熊书内存序结合**：哪些优化会被 `fence` / `atomic` **禁止**或改变形状。

## 目录约定

| 子目录 | 说明 |
|--------|------|
| `notes/` | Pass 名称、书中案例、与 `atomic` 章节的交叉索引 |
| `code/` | 可选：最小可优化 vs 不可优化对比说明 |

## IR 归档

**成对**保存 debug vs release 或 `-C opt-level=0` vs `3` 到 `ir_samples/optimize_compare/`，便于 diff。
