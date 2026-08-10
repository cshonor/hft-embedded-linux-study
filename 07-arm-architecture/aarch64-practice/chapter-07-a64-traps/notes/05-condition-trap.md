# 7.5 条件执行陷阱

> 来源：§7.5 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

从 AArch32 迁移到 AArch64 时，条件执行机制的差异导致的陷阱。

## 核心要点

AArch32 vs AArch64 条件执行：

| 特性 | AArch32 (ARMv7) | AArch64 (ARMv8) |
|------|-----------------|------------------|
| 条件后缀 | 几乎所有指令都可加后缀 | 只有少数（B.cond/CSEL/CCMP） |
| IT 块 | IT 指令让下 4 条指令条件执行 | **无 IT 块** |
| 预测执行 | 条件指令都预测执行 | 用 CSEL 替代 |

```asm
; AArch32 风格（AArch64 不支持）
moveq r0, #1     ; ❌ AArch64 没有 MOVcond
addgt r0, r0, #1 ; ❌ AArch64 没有 ADDcond

; AArch64 等价写法
csel x0, x1, x2, eq   ; 条件选择
b.eq label             ; 条件跳转
```

## HFT 关联

从 32 位迁移到 64 位时的常见问题：
- ARMv7 的条件执行可以消除分支 → AArch64 需用 CSEL 替代
- 迁移老代码时不能直接翻译 `moveq` → 需改写为 CSEL 或分支
- AArch64 的 CSEL 虽然不如条件执行通用，但覆盖了大部分场景
- 性能上 CSEL 仍然无分支，延迟可预测

## 自测题

1. AArch64 为什么取消了 IT 块？
<detail><summary>答案</summary>
IT 块让 1-4 条指令条件执行，增加解码复杂度（需跟踪 IT 状态），且对乱序执行不友好。AArch64 简化指令集，用 CSEL/CSET/CCMP 等专用条件指令替代，更适合超标量乱序流水线。
</details>

2. AArch32 的 `moveq r0, #1` 在 AArch64 怎么写？
<details><summary>答案</summary>
```asm
; 方法1：CSEL（无分支）
mov x1, #1
mov x2, #0    ; 或保留原值
csel x0, x1, x2, eq

; 方法2：CSET（如果只需要 0/1）
cset x0, eq   ; eq → x0=1, ne → x0=0
```
</details>

3. CSEL 和条件分支哪个更适合 HFT？
<detail><summary>答案</summary>
CSEL 更适合。CSEL 始终 1 cycle，无分支预测失败风险。条件分支预测正确时 ~0-1 cycle，但预测失败时 ~20 cycle（流水线冲刷）。HFT 追求延迟可预测性，CSEL 更优。但 CSEL 只能选择值，不能跳转到不同代码块。
</details>

## 参考与延伸

- 原书 §7.5
- [5.2 CSEL](../../chapter-05-a64-compare-branch/notes/section-0-本章完整概述.md)
- ARM ARM §C1.3
