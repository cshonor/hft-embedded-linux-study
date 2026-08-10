# §14.8 易错点清单

> **来源：** [Ch14 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

MMU 配置的 5 个常见错误：开 MMU 没恒等映射、MMIO 用 Normal 属性、忘记 ISB、页表没 clean cache、AF=0。

## 核心要点

| # | 易错点 | 后果 | 修复 |
|---|--------|------|------|
| 1 | 开 MMU 没恒等映射 | PC 还是物理地址，MMU 翻译失败 → 死机 | 开 MMU 前建 VA=PA 映射 |
| 2 | MMIO 用了 Normal 属性 | 寄存器被缓存，读写行为未定义 | AttrIndx 指向 Device 属性 |
| 3 | 忘记 ISB | 开 MMU 后流水线中有旧指令，行为不可预测 | SCTLR.M=1 后立即 ISB |
| 4 | 页表不在内存中 | 页表缓存在 D-cache，MMU walker 从内存读旧值 | dc civac + dsb |
| 5 | AF=0 | 第一次访问触发 Access Flag fault | 初始化时设 AF=1 |

### 调试技巧

| 症状 | 可能原因 |
|------|----------|
| 开 MMU 后立即死机 | 没恒等映射 / 忘记 ISB |
| MMIO 读写不生效 | 用了 Normal 属性（被缓存） |
| 随机 page fault | 页表没 clean cache / AF=0 |
| 代码执行了旧版本 | I-Cache 缓存旧指令（需 invalidate） |

## HFT 关联

MMU 配置错误在 HFT 系统中是致命的。MMIO 用 Normal 属性是最隐蔽的 bug——寄存器读写被缓存，看起来"偶尔工作"但行为不可靠。HFT 系统上线前应该验证所有 MMIO 区域的页表属性为 Device-nGnRE。AF=0 导致的 Access Flag fault 在 HFT 中不可接受（引入异常处理开销），所有页表项初始化时必须设 AF=1。

## 自测题

1. **开 MMU 后立即死机，最可能的原因是什么？**

<details>
<summary>答案</summary>

最可能原因：**没有恒等映射**。开 MMU 前 PC 是物理地址，开 MMU 后 CPU 通过 MMU 翻译取指地址，如果没有 VA=PA 的映射，取指 page fault → 死机。第二可能：**忘记 ISB**，流水线中有 MMU 开启前的旧指令。
</details>

2. **MMIO 寄存器映射为 Normal 属性会有什么问题？**

<details>
<summary>答案</summary>

Normal 属性允许**缓存、合并、重排**。MMIO 寄存器读写有副作用（如读中断状态寄存器清除中断），被缓存后：1) 读寄存器返回 cache 中的旧值而非实际寄存器值；2) 两次写寄存器可能被合并为一次；3) 写操作可能不按顺序到达设备。行为完全不可预测。
</details>

3. **页表写在 cacheable 区域，开 MMU 前必须做什么？**

<details>
<summary>答案</summary>

必须 **clean cache**（`dc civac` + `dsb sy`）。页表项可能还在 D-cache 中没有写回内存，MMU walker 从内存读页表 → 读到旧值或零 → 翻译错误。`dc civac` 强制写回 + 作废，`dsb sy` 等待写回完成。
</details>

## 参考与延伸

- [§14.6 开 MMU 流程](06-enable-mmu.md) — 避免初始化错误
- [§14.4 内存属性](04-memory-attributes.md) — MMIO 属性设置
- [Ch17 §17.7 易错点](../../chapter-17-tlb-management/notes/section-0-本章完整概述.md) — TLB 相关错误
