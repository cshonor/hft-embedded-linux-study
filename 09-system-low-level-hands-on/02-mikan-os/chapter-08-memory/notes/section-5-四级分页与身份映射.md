## 5. 四级分页与身份映射

> **x86-64 长模式：分页强制开启** — 必须配置 **页表** 才能稳定运行。

---

### 一、四级页表结构

虚拟地址（48 bit 有效，示意）拆分为：

```
[ PML4 index | PDP index | PD index | PT index | page offset ]
     9 bit      9 bit       9 bit      9 bit      12 bit
```

| 层级 | 表名 | 作用 |
|------|------|------|
| **L4** | **PML4** | 根 — **CR3** 指向 |
| **L3** | **PDP** | 下一级指针表 |
| **L2** | **PD** | 可 **2MiB 大页** 或指向 PT |
| **L1** | **PT** | **4KiB 页** 项 |

**遍历：** CR3 → PML4 → PDP → PD → PT → **物理页帧 + offset**。

→ [CSAPP Ch9 多级页表](../../../01-CSAPP-3rd/chapter-09-virtual-memory/) · [02-Hennessy 虚拟内存](../../../02-Computer-Architecture-6th/)

---

### 二、身份映射（Identity Mapping）

**本章策略：** **虚拟地址 = 物理地址**（至少覆盖内核与设备 MMIO 所需范围）

```
VA 0x0010_0000  →  PA 0x0010_0000
VA 0xFFFF_8000… →  PA 0xFFFF_8000…（若书使用高半核映射则按书）
```

| 优点 | 说明 |
|------|------|
| **实现简单** | 早期 C++ 指针 **可直接当物理地址用** |
| **过渡阶段** | Ch19 再引入 **进程独立页表** |

**页表项标志（概念）：** Present · Writable · User/Supervisor · NX（若启用）

---

### 三、启用 — 更新 CR3

```
1. 在 OS 内存中构建 PML4→…→PT 链
2. 填充 identity 映射项
3. mov cr3, pml4_phys_addr
4. （若从 UEFI 页表切换）确保切换瞬间 RIP/RSP 均在映射范围内
```

| 失败症状 | 原因 |
|----------|------|
| **Immediate #PF** | 当前指令地址 **未映射** |
| **三重 fault** | 页表本身地址 **不可访问** |

**与 Ch3 迁移：** 新页表必须在 **切 CR3 前** 对 **新栈/GDT/内核代码** 全部有效。

---

← [4. GDT](./section-4-GDT与分段.md) · 下一节 [6. 位图分配器](./section-6-位图管理器与首次适配.md)
