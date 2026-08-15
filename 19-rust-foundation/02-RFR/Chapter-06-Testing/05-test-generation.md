# 2.2 Test Generation（测试生成）

> 所属：**Additional Testing Tools** · [← 章索引](./README.md)

## 模糊测试 (Fuzzing)

对解析器等喂随机 / 变异输入直至 panic。

- **`cargo-fuzz`** + libFuzzer 等后端。
- 适合输入空间巨大、手写用例难覆盖的 API。

## 基于属性的测试 (PBT)

- **`proptest`** 等生成大量输入。
- 对照**朴素参考实现**或**不变量 (invariant)**，发现边界 bug。

## 与单元测试分工

- 单元测试 — 已知关键路径。
- Fuzz / PBT — 探索未知输入空间。

ER → [Item 30 超越单元测试](../../01-ER/Chapter-05-Tooling/Item-30-beyond-unit-tests/README.md)
