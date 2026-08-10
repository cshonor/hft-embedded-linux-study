# 5.7 实验要点

> 来源：§5.7 · 精读 · [章总览](section-0-本章完整概述.md)

## 实验列表

| 实验 | 内容 | 平台 |
|------|------|------|
| 5-1 | 条件跳转与循环 | QEMU |
| 5-2 | CSEL 实现无分支 | QEMU |
| 5-3 | CBZ/TBZ 位测试 | QEMU |
| 5-4 | 条件后缀综合 | QEMU |

## 实验重点

1. GDB 单步对比 CMP+B.LT 和 CSEL 的执行流程
2. 故意用错条件后缀（LT vs LO）观察错误结果
3. 用 CSEL 替换 if-else 分支，对比反汇编
4. TBZ 测试多状态标志位分发

## 自测题

1. 实验中如何验证 CSEL 确实无分支？
<details><summary>答案</summary>
GDB 单步执行：CSEL 只有一步，没有跳转。而 if-else 编译为 B.cond + B，至少两步且有跳转。也可看反汇编确认无 B 指令。
</details>

2. 如果用 LT 比较两个内核地址（0xFFFF...），会发生什么？
<details><summary>答案</summary>
地址高位为 1，有符号比较会把它们当成负数。如果两个地址都接近 0xFFFF...，LT 比较可能得到正确结果（都是"大负数"）；但混合用户地址(0x0000...)和内核地址时，用户地址被当成大正数，比较结果反转，导致 bug。
</details>

## 参考与延伸

- 原书 §5.7
- [5.4 条件后缀](04-condition-suffix.md)
- [4.2 NZCV](../../chapter-04-a64-arithmetic-shift/notes/section-0-本章完整概述.md)
