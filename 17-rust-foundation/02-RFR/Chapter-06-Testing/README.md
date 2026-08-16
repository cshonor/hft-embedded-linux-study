# 第 6 章：测试 (Testing)

> **Rust for Rustaceans** · [全书目录](../RFR-本书目录.md)  
> 从「会写 `#[test]`」到**可维护、可证明、可度量**的测试体系。

## 本章结构（与原书对齐）

**3 个主节** · 连同二级子节共 **10 个部分**（2 个带子的主节标题 + 3 + 4 + Summary）。

| 主节 | 英文 | 子节 / 笔记 |
|------|------|-------------|
| **1** | Rust Testing Mechanisms | [01 测试脚手架](./01-test-harness.md) · [02 `#[cfg(test)]`](./02-cfg-test.md) · [03 文档测试](./03-doctests.md) |
| **2** | Additional Testing Tools | [04 Lint](./04-linting.md) · [05 测试生成](./05-test-generation.md) · [06 测试增强](./06-test-augmentation.md) · [07 性能测试](./07-performance-testing.md) |
| **3** | Summary | [08 小结](./08-summary.md) |

## 与 The Book / ER 对照

| 主题 | 本仓库 |
|------|--------|
| 测试基础 | [11.1 如何编写测试](../../00-Book/11-testing/11.1-如何编写测试.md) |
| cfg test | [11.1.1 cfg test](../../00-Book/11-testing/11.1.1-cfg-test与模拟输入.md) |
| Clippy | [ER Item 29](../../01-ER/Chapter-05-Tooling/Item-29-clippy/README.md) |
| 超越单元测试 | [ER Item 30](../../01-ER/Chapter-05-Tooling/Item-30-beyond-unit-tests/README.md) |
| Miri | [ER Item 16 demo](../../01-ER/Chapter-03-Concepts/Item-16-avoid-unsafe/demo/) |

## 旧版单文件

见 git 中的 `6-测试-Testing-深度解析.md`。
