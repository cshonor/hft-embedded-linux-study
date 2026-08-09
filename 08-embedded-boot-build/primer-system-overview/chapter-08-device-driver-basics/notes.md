# Embedded Linux Primer 第 08 章 — Device Driver Basics

> 对应目录：`chapter-08-device-driver-basics/`  
> 书：*Embedded Linux Primer*, 2nd ed — Christopher Hallinan  
> 大纲：[../OUTLINE.md](../OUTLINE.md)

**优先级**：选读入门；**主课在 [12](../../../09-device-drivers-dt/)**  
**FAQ：** [8.0](./8.0-new-hw-dts-vs-driver.md)–[8.6](./8.6-gpio-header-vs-dts.md)  
**C 语言：** 标准 C 为根 — [struct 驱动向](../../../01-c-language/01-Primer-K-and-R-C/ch06-structures/6.0-struct-for-drivers.md)；高频 GNU 见 [速查表](../../../01-c-language/04-Kernel-Prep-Embedded-C-Self-Cultivation/ch06-gnu-c-extensions/DRIVER-GNU-C-CHEATSHEET.md) · [学多少](../../../01-c-language/04-Kernel-Prep-Embedded-C-Self-Cultivation/ch06-gnu-c-extensions/6.0-driver-how-much-gnu-c.md)  
**前置：** [Ch7 · DT ≠ UEFI](../chapter-07-bootloaders/7.0-device-tree-vs-uefi.md) · [BSP 模板](../chapter-03-processor-basics/3.2-bsp-is-template-not-product.md)  
**后置：** [12 Madieu + LDD3](../../../09-device-drivers-dt/) · [MELP](../../build-toolchain-yocto/)

---

## 章节要点

（待填 · 见 OUTLINE 小节表：模块、insmod、设备号、fops）

> **先建立心智：** 加硬件 ≠ 必写驱动；但 **DTS 几乎必动** — [8.0](./8.0-new-hw-dts-vs-driver.md)。  
> **绑定机制：** `compatible` → `probe` → `of_*` — [8.1](./8.1-dts-driver-relationship.md)。  
> **为何是内核代码：** 权限/中断/隔离 — [8.2](./8.2-why-drivers-in-kernel.md)。  
> **不只初始化：** 读写 + IRQ + 异常 + 释放 — [8.3](./8.3-driver-lifecycle-and-irq.md)。  
> **为何常零编码：** 主线/标准协议/BSP 已有驱动 — [8.4](./8.4-why-most-drivers-are-shared.md)。  
> **最简外设：** GPIO — [8.5](./8.5-gpio-basics.md)；**排针≠全是 GPIO** — [8.6](./8.6-gpio-header-vs-dts.md)。

---

## 参考

- Hallinan, *Embedded Linux Primer*, 2nd ed, Chapter 8  
- [8.0](./8.0-new-hw-dts-vs-driver.md)–[8.6](./8.6-gpio-header-vs-dts.md)  
- [09-device-drivers-dt](../../../09-device-drivers-dt/)
