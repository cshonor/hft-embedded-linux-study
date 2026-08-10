# 6.2 SVC 系统调用

> 来源：§6.2 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

SVC 指令触发同步异常实现系统调用，从 EL0 切换到 EL1。

## 核心要点

```asm
; 用户态发起系统调用
mov x8, #64        ; syscall number (e.g., write=64)
mov x0, #1         ; fd=stdout
ldr x1, =msg       ; buffer
mov x2, #12        ; length
svc #0             ; 触发系统调用异常
```

- SVC 触发同步异常 → CPU 切到 EL1
- 异常向量表中 `低EL→当前EL AArch64, 同步` 表项处理
- x8 传系统调用号，x0-x5 传参数，x0 返回值
- 内核通过 ESR_ELx 的 EC 字段识别 SVC（EC=0x15）

## HFT 关联

系统调用是 HFT 低延迟的大敌：
- 每次 SVC 产生异常切换开销 ~1-5μs（EL0→EL1→EL0）
- HFT 关键路径尽量避免 syscall → 预分配内存、避免 I/O
- io_uring（5.1+）把多次 I/O 系统调用批量提交 → 减少 SVC 次数
- vDSO 把 gettimeofday/clock_gettime 映射到用户态 → 零 syscall 获取时间

## 自测题

1. SVC #0 执行后 CPU 发生了什么？
<details><summary>答案</summary>
1. 硬件保存 PC→ELR_EL1, PSTATE→SPSR_EL1
2. 切换到 EL1 的 SP
3. 跳转到 VBAR_EL1 中"低EL→当前EL AArch64, 同步异常"表项（偏移 0x400）
4. 软件保存 X0-X30，读 ESR_EL1 确认 EC=0x15(SVC)，读 x8 获取调用号
</details>

2. 为什么 HFT 要尽量减少系统调用？
<details><summary>答案</summary>
每次 SVC 产生异常切换：保存/恢复寄存器、EL 切换、内核调度可能介入。开销 ~1-5μs，而用户态操作只需纳秒级。HFT 热路径中一次意外的 syscall 可能导致错过交易窗口。
</details>

3. vDSO 如何避免系统调用获取时间？
<detail><summary>答案</summary>
vDSO 把内核的时间数据（vvar 页）映射到用户空间只读。gettimeofday/clock_gettime 直接在用户态读取 vvar 页的时间值，不需要 SVC。只有时间精度要求极高时才回退到真正的 syscall。
</details>

## 参考与延伸

- 原书 §6.2
- [Ch11 异常处理](../../chapter-11-exception-handling/notes/section-0-本章完整概述.md)
- [Ch21 自定义系统调用](../../chapter-21-os-topics/notes/section-0-本章完整概述.md)
