# 2.4 Performance Testing（性能测试）

> 所属：**Additional Testing Tools** · [← 章索引](./README.md)

## 常见陷阱

| 陷阱 | 对策 |
|------|------|
| **方差** | `criterion` 多次采样 + 统计区间 |
| **DCE 测没了** | `std::hint::black_box` 抑制对被测结果的优化 |
| **I/O 主导** | 热循环去掉 `println!`、RNG 等无关开销 |

## `black_box` 示例

→ [ER Item 30 demo](../../01-ER/Chapter-05-Tooling/Item-30-beyond-unit-tests/demo/)

## 与第 7 章优化

基准数字需结合 [06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17 ch07](../../06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/part02_src_to_machine/chapter07_ir_optimize/README.md) 理解编译器做了什么。

ER → [Item 20 避免过度优化](../../01-ER/Chapter-03-Concepts/Item-20-avoid-over-optimize/README.md)
