# 5.5 CBZ / CBNZ / TBZ / TBNZ

> 来源：§5.5 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

CBZ/CBNZ（比较为零）和 TBZ/TBNZ（测试位为零）—— 专用条件跳转指令。

## 核心要点

| 指令 | 作用 | 等价 |
|------|------|------|
| CBZ | x0==0 则跳转 | CMP + B.EQ |
| CBNZ | x0!=0 则跳转 | CMP + B.NE |
| TBZ | x0[bit]==0 则跳转 | TST + B.EQ |
| TBNZ | x0[bit]!=0 则跳转 | TST + B.NE |

```asm
cbz x0, null_ptr       ; if (x0 == 0) goto null_ptr
tbnz x0, #7, negative  ; if (x0 bit[7] != 0) goto negative
```

- CBZ/CBNZ 比 CMP+B.EQ 更紧凑（1 条 vs 2 条）
- TBZ/TBNZ 直接测试特定位，不需要构造掩码
- 这些指令有更大的跳转范围（CBZ ±1MB，TBZ ±32KB）

## HFT 关联

这些指令在空指针检查和标志测试中极其常用：
- `cbz x0, error` 空指针检查 → 1 条指令
- `tbnz x0, #flag_bit, handler` 测试状态标志位 → 1 条指令
- 比 CMP+TST+B 组合少 1 条指令 → 减少代码体积和延迟
- 编译器自动使用这些指令优化 if(ptr==NULL) 和 if(flags & MASK) 模式

## 自测题

1. `CBZ x0, label` 和 `CMP x0, #0; B.EQ label` 哪个更好？
<details><summary>答案</summary>
CBZ 更好：1 条指令 vs 2 条，更紧凑。CBZ 不修改 NZCV 标志（不执行 CMP），避免影响后续条件判断。
</details>

2. 如何测试 x0 的第 15 位是否为 1？
<details><summary>答案</summary>
```asm
tbnz x0, #15, bit_set   ; 如果 bit[15]=1 则跳转
```
TBNZ 直接测试特定位，不需要 AND 构造掩码。
</details>

3. CBZ 和 TBZ 的跳转范围有何区别？
<details><summary>答案</summary>
CBZ/CBNZ 的跳转范围约 ±1MB（19 位偏移×4）。TBZ/TBNZ 的跳转范围约 ±32KB（14 位偏移×4）。TBZ 范围更小因为指令中要编码位号。
</details>

## 参考与延伸

- 原书 §5.5
- [5.1 比较指令](01-compare.md)
- [4.4 位操作](../../chapter-04-a64-arithmetic-shift/notes/section-0-本章完整概述.md)
