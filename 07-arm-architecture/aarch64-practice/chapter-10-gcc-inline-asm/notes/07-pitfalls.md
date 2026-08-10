# 10.7 易错点清单

> 来源：§10.7 · 精读 · [章总览](section-0-本章完整概述.md)

## 4 大易错点

1. **忘记 volatile** → 有副作用的汇编被优化掉
2. **忘记 "memory" clobber** → 内存访问被重排
3. **约束选错** → 编译器生成错误代码
4. **early clobber 遗漏** → 输入输出寄存器冲突

## 自测题

1. 以下代码有什么问题？
```c
u64 read_time(void) {
    u64 t;
    asm("mrs %0, cntvct_el0" : "=r"(t));
    return t;
}
```
<details><summary>答案</summary>
缺少 `volatile`。读取时间戳有副作用，编译器可能优化掉。应加 `asm volatile(...)`。
</details>

2. 以下代码为什么可能产生错误结果？
```c
asm volatile(
    "mov %0, %1\n"
    "add %0, %0, #1"
    : "=r"(result) : "r"(input)
);
```
<details><summary>答案</summary>
缺少 `&`（early clobber）。mov 在 add 前写入 %0，编译器可能让 %0 和 %1 共用寄存器。应改为 `"=&r"(result)`。
</details>

3. 内联汇编中忘记了 `"memory"` clobber 会怎样？
<details><summary>答案</summary>
编译器可能把汇编前后的内存访问重排到另一侧，导致数据不一致。有内存副作用的汇编必须加 `"memory"`。
</details>

## 参考与延伸

- 原书 §10.7
- [10.2 约束字符](02-constraints.md)
