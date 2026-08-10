# 6.6 实验要点

> 来源：§6.6 · 精读 · [章总览](section-0-本章完整概述.md)

## 实验列表

| 实验 | 内容 | 平台 |
|------|------|------|
| 6-1 | ADRP+ADD 获取全局变量地址 | QEMU |
| 6-2 | SVC 系统调用（EL0→EL1） | QEMU |
| 6-3 | MRS/MSR 读写系统寄存器 | QEMU |
| 6-4 | LDXR/STXR 原子自增 | QEMU |
| 6-5 | DMB 屏障效果验证 | QEMU |

## 实验重点

1. 用 ADRP+ADD 替换 LDR =伪指令，对比反汇编
2. SVC 触发异常，GDB 跟踪 EL0→EL1 切换
3. MRS CurrentEL 确认当前异常等级
4. LDXR/STXR 实现原子计数器，多核竞争测试
5. DMB 前后的访存顺序对比（用 GDB 观察乱序）

## 自测题

1. 实验中如何验证 ADRP+ADD 获取的地址正确？
<details><summary>答案</summary>
1. 用 GDB 打印 ADRP 后的 x0（页基地址）和 ADD 后的 x0（精确地址）
2. 与符号表中的地址对比：`info address global_var`
3. 用获取的地址 LDR 读取值，与预期对比
</details>

2. 如何在实验中观察 LDXR/STXR 的重试？
<details><summary>答案</summary>
1. 在 STXR 后设断点，检查 w2（返回值）
2. 如果 w2 != 0，说明发生了重试
3. 多核场景下用 GDB attach 到另一个核，在 LDXR 和 STXR 之间写同一地址
4. 观察监视器失效导致的 STXR 失败
</details>

## 参考与延伸

- 原书 §6.6
- [6.1 ADR/ADRP](01-adr-adrp.md)
- [6.4 LDXR/STXR](04-ldxr-stxr-preview.md)
