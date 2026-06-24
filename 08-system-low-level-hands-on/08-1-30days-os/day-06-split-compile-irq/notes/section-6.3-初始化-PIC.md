## ③ 初始化 PIC · 可编程中断控制器

CPU **引脚有限** — 外设中断由 **PIC** 汇总后再通知 CPU。

| 名称 | 角色 |
|------|------|
| **PIC** | Programmable Interrupt Controller — CPU 的 **「中断秘书」** |
| **主 PIC** | 接一批 IRQ |
| **从 PIC** | 级联在 **主 PIC 的 IRQ2** 上，扩展 IRQ 线 |

```
键盘/鼠标/… ──► 从PIC ──► 主PIC ──► CPU INTR
                  (IRQ2 级联)
```

通过 **端口 I/O**（汇编 **`OUT`/`IN`**）写 PIC 寄存器 → **完成初始化**（向量号映射、级联、屏蔽位等 — 以原书为准）。

**HFT：** 现代机器多用 **APIC/IOAPIC**；概念仍是 **设备 → 中断控制器 → 核**，以及 **IRQ affinity 绑核**。

---
