# Embedded Linux Primer 第 08 章 — Device Driver Basics

> 对应目录：`chapter-08-device-driver-basics/`  
> 书：*Embedded Linux Primer*, 2nd ed — Christopher Hallinan  
> 大纲：[../OUTLINE.md](../OUTLINE.md)

**优先级**：选读入门；**主课在 [12](../../../12-device-drivers-dt/)**  
**FAQ：** [新增外设：DTS 必改 / 驱动分三档](./8.0-new-hw-dts-vs-driver.md) · [DT↔驱动完整关系](./8.1-dts-driver-relationship.md)  
**前置：** [Ch7 · DT ≠ UEFI](../chapter-07-bootloaders/7.0-device-tree-vs-uefi.md) · [BSP 模板](../chapter-03-processor-basics/3.2-bsp-is-template-not-product.md)  
**后置：** [12 Madieu + LDD3](../../../12-device-drivers-dt/) · [MELP](../../build-toolchain-yocto/)

---

## 章节要点

（待填 · 见 OUTLINE 小节表：模块、insmod、设备号、fops）

> **先建立心智：** 加硬件 ≠ 必写驱动；但 **DTS 几乎必动** — [8.0](./8.0-new-hw-dts-vs-driver.md)。  
> **绑定机制：** `compatible` → `probe` → `of_*` — [8.1](./8.1-dts-driver-relationship.md)。

---

## 参考

- Hallinan, *Embedded Linux Primer*, 2nd ed, Chapter 8  
- [8.0-new-hw-dts-vs-driver.md](./8.0-new-hw-dts-vs-driver.md)  
- [8.1-dts-driver-relationship.md](./8.1-dts-driver-relationship.md)  
- [12-device-drivers-dt](../../../12-device-drivers-dt/)
