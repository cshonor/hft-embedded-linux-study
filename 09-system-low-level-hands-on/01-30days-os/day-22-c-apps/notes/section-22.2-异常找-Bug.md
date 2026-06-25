## ② 异常找 Bug · 强制结束程序

#### 异常也是调试器

| 异常 | 典型原因 |
|------|----------|
| **INT 0x0d** | 越界、特权指令、坏段寄存器 |
| **INT 0x0c** | **栈段 fault** — C **数组越界写栈** 等 |

**`inthandler` 增强：** 打印 **`EIP` 等寄存器** → 对照 **`.map` / `.lst`** 定位 **崩溃源行**。

```
崩溃 → 屏上 EIP=0x00001234
     → .map 查符号 / .lst 查行号
```

**HFT：** 生产用 **core + addr2line** — 同一 **「寄存器快照 + 符号表」** 套路。

#### Shift+F1 · 强杀 runaway app

**死循环 app** 不触发异常 → **`Shift+F1`** → OS **截获** → **强制终止** → **Console 收回**。

**用户态 escape hatch** — 类似 **Ctrl+C / kill -9** 的极简版。

→ [Day 17 Shift 状态](../day-17-console/)

---
