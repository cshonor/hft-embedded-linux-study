# §11.5 异常综合征（ESR）

> **来源：** [Ch11 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

同步异常发生时，ESR_ELx 寄存器包含异常原因的分类编码（EC），FAR_ELx 保存触发数据异常的虚拟地址。通过读 ESR + FAR 可以精确定位异常原因。

## 核心要点

### ESR_ELx 寄存器格式

```asm
mrs x0, ESR_EL1         // 读异常综合征
lsr x1, x0, #26         // EC (Exception Class) 在 bit[31:26]
```

### 常见 EC 值

| EC 值 | 含义 |
|-------|------|
| 0x15 | SVC 系统调用 |
| 0x20 | 在 EL0 的指令中止（页错误取指） |
| 0x24 | 在 EL0 的数据中止（页错误访存） |
| 0x25 | 在 EL1 的数据中止 |
| 0x22 | 在 EL0 的对齐错误 |

### ESR vs FAR

| 寄存器 | 作用 | 何时有效 |
|--------|------|----------|
| ESR_ELx | 异常原因分类（EC + ISS） | 所有同步异常 |
| FAR_ELx | 触发异常的虚拟地址 | 数据中止（访存异常） |

> ISS（Instruction Specific Syndrome）= ESR 的低 25 位，提供更细粒度的异常信息（如 SVC 的立即数、页错误的读写方向等）。

## HFT 关联

在 HFT 裸金属开发中，ESR/FAR 是调试页错误的第一工具。交易系统访问未映射的内存地址会导致同步异常，通过读 ESR 判断是取指错误（0x20）还是数据访问错误（0x24），再读 FAR 获取具体地址，可以快速定位是哪行代码访问了非法地址。在内核态（EL1）发生数据中止（EC=0x25）通常意味着内核 bug，需要立即处理。

## 自测题

1. **ESR_EL1 的 EC 字段在哪些位？怎么提取？**

<details>
<summary>答案</summary>

EC 在 **bit[31:26]**（最高 6 位）。提取：`mrs x0, ESR_EL1; lsr x1, x0, #26`，x1 即 EC 值。
</details>

2. **SVC 系统调用的 EC 值是多少？如何从 ESR 中获取 SVC 指令的立即数？**

<details>
<summary>答案</summary>

SVC 的 EC = **0x15**。SVC 指令的立即数在 ISS（低 25 位，bit[24:0]）中。用 `and x1, x0, #0x1FFFFFF` 提取 ISS，即 SVC 的系统调用号。
</details>

3. **FAR_ELx 在什么情况下有效？什么情况下无效？**

<details>
<summary>答案</summary>

FAR 在**数据中止**（访存异常，如 EC=0x24/0x25）时有效，保存触发异常的虚拟地址。在**指令中止**时也可能有效（保存取指地址）。但在 SVC、未定义指令等非访存异常中，FAR 的值**无效**（可能是上一次异常的残留值），不应使用。
</details>

## 参考与延伸

- [§11.1 异常类型](01-exception-types.md) — 哪些异常是同步的（才有 ESR）
- [§11.4 硬件保存+软件保存](04-hw-sw-save.md) — 读 ESR 是在保存完现场之后
- [§11.7 实验要点](07-lab.md) — 实验 11-4 解析 ESR/FAR
