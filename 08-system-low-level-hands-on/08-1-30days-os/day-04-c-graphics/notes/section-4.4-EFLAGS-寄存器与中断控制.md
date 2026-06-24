## ④ EFLAGS 寄存器与中断控制

设调色板要 **连续写多个端口** — 若中途被 **中断打断**，颜色可能错乱。

| 指令 | 作用 |
|------|------|
| **`CLI`** | **Clear Interrupt flag** — **屏蔽可屏蔽中断** |
| **`STI`** | **Set Interrupt flag** — **恢复中断** |

 **`EFLAGS`**（32 位）含 **IF（中断标志）** 等位 — 控制 CPU 是否响应部分中断。

| 栈操作 | 作用 |
|--------|------|
| **`PUSHFD`** | 把 **EFLAGS** 压栈保存 |
| **`POPFD`** | 从栈恢复 **EFLAGS** |

**模式：** `PUSHFD` → `CLI` → 设调色板 → `POPFD`（或 `STI`）— **原子性更好的临界区**。

→ 后续 Day **IDT / 自己的中断处理**；Linux 里对应 **local_irq_save/restore** 一类思路。

---
