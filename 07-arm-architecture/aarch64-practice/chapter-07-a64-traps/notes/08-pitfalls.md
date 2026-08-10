# 7.8 易错点清单

> 来源：§7.8 · 精读 · [章总览](section-0-本章完整概述.md)

## 5 大易错点

1. **MOV 大立即数**超出 16 位 → 汇编报错
2. **字符串加载**小端序混淆 → 字符顺序反转
3. **LDXR 在 Device 内存**上执行 → 死机
4. **SP 不 16 字节对齐** → SP 对齐异常
5. **AArch32 条件执行**搬到 AArch64 → 不支持

## 自测题

1. 以下代码哪里有错？
```asm
mov x0, #0x12345678
str x0, [sp]
```
<details><summary>答案</summary>
`mov x0, #0x12345678` 不合法——0x12345678 超出 16 位立即数范围，不能单条 MOV 加载。应改为 `movz x0, #0x5678; movk x0, #0x1234, lsl #16` 或 `ldr x0, =0x12345678`。
</details>

2. 在 QEMU 上 LDXR/STXR 测试通过，上真实 Pi 硬件却死机，可能原因？
<detail><summary>答案</summary>
1. 在 Device/MMIO 内存上用了 LDXR（QEMU 不检查，真实硬件会异常）
2. 地址未对齐（QEMU 宽松，硬件严格）
3. 独占监视器超时（QEMU 不模拟超时，硬件有超时限制）
4. QEMU 没有完整实现独占监视器竞争 → 真实硬件 STXR 失败处理不当
</details>

3. 从 ARMv7 代码迁移 `itt eq; mov r0, #1; mov r1, #2` 到 AArch64 怎么写？
<details><summary>答案</summary>
AArch64 没有 IT 块，需要用 CSEL 或分支：
```asm
// 方法1：分支
b.ne skip
mov x0, #1
mov x1, #2
skip:

// 方法2：CSEL（如果只需要选值）
mov x2, #1
mov x3, #0
csel x0, x2, x3, eq   ; eq → x0=1, ne → x0=0
```
</details>

## 参考与延伸

- 原书 §7.8
- [7.1 MOV 陷阱](01-mov-trap.md)
- [7.3 LDXR 死机](03-ldxr-crash.md)
