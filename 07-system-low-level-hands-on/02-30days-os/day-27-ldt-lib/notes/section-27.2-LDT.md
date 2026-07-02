## ② LDT · 局部段记录表

#### 缺口 · crack7.hrb

Day 21 **GDT** 隔离 **OS vs 单 app**；Day 25 **动态段号** 隔离 **并发 app 数据**。

**crack7** 证明：**恶意 app 仍可读写其他 app 的段** — GDT 条目 **全局可见**。

#### LDT vs GDT

| | **GDT** | **LDT** |
|--|---------|---------|
| 范围 | **全局** — OS + 所有任务可见描述符 | **每任务私有** |
| 用途 | 内核段、TSS、公共描述符 | **该 app 专属 CS/DS** |
| CPU | **LGDT** | 任务切换时 **LLDT / TSS 中 LDT** |

**每个 TASK 自己的 LDT** 注册 **本 app 代码/数据段** → CPU **无法加载别 task 的 LDT 项** → **硬件级 app↔app 隔离**。

```
app A 的 LDT:  仅 A 的 CS/DS
app B 的 LDT:  仅 B 的 CS/DS
A 想 MOV DS, B的段 → #GP
```

**多任务安全闭环** — 配合 Day 21 **0x0d**、Day 25 **sel 映射**。

→ [Day 5/6 GDT](../day-05-gdt-idt/) · [03-Linux-Kernel-Development 内存保护](../../../../03-Linux-Kernel-Development/)

---
