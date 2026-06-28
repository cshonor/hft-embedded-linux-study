## 4. MSI 中断配置

---

### 一、Legacy IRQ vs MSI

| Legacy（8259 PIC / 共享 IRQ 线） | **MSI（Message Signaled Interrupt）** |
|-----------------------------------|----------------------------------------|
| 物理 **IRQ 线** 电平/边沿触发 | **内存写** 触发逻辑中断 |
| 多设备 **共享** 一线 · 需查询谁中断 | 可 **每设备独立** Message |
| 老 PCI 常见 | **现代 PCI/PCIe** 标配 — **xHC** |

**MSI 本质：** 设备向 **Message Address** 写入 **Message Data** → 芯片组/APIC 转为 **CPU 向量中断**。

---

### 二、配置步骤（概念）

```
1. 在 IDT 中为 MSI 向量注册 ISR
2. 从 PCI 配置空间 / MSI Capability 读取：
      Message Address（通常 LAPIC 窗口）
      Message Data（含向量号等）
3. 向 xHC 写入：启用 MSI · 绑定向量
4. 设备有事件 → DMA/寄存器更新 → MSI 写 → CPU 进 ISR
```

| 与 Ch6 关系 | 说明 |
|-------------|------|
| **Ch6** | xHC 初始化 · **轮询** Event Ring |
| **Ch7** | 同一 xHC · 改 **MSI 通知** 替代空转 |

---

### 三、为何 xHC 适合 MSI

- **PCIe 设备** — 引脚 IRQ 少 · **MSI/MSI-X** 更灵活
- **高事件率** USB — 中断驱动 + FIFO 比轮询 **省 CPU**

**调试提示：** MSI 地址/向量配错 → **永不进 ISR** — 用 QEMU monitor 查 **IDT 项** 与 PCI MSI 寄存器。

→ [Ch6 xHC 初始化](../chapter-06-mouse-pci/notes/section-5-BAR0与xHC初始化.md)

---

← [3. IDT](./section-3-IDT与lidt.md) · 下一节 [5. FIFO](./section-5-FIFO与ArrayQueue.md)
