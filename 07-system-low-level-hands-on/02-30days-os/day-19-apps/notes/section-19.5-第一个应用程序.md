## ⑤ 第一个应用程序 · `hlt.hrb`

#### 极简 app

**`hlt.hrb`** — 仅 **3 字节** 级：**HLT + 死循环**（原书示意）。

#### OS 如何运行外部程序

| 步骤 | 说明 |
|------|------|
| **`file_loadfile`** | 从磁盘 **读 .hrb 进内存** |
| **新 GDT 段** | 为 app 分配 **独立段描述符**（如 **1003 号**）— **与内核段隔离** |
| **`farjmp`** | 跳到 **app 段:offset** 执行 |

```
Console: run hlt.hrb
    → file_loadfile → 内存 buffer
    → 注册 GDT 段（用户/app 代码段）
    → farjmp → app 在 Ring 保护下跑（以原书设定为准）
    → HLT…
```

**转折点：** 从 **内置命令** → **加载外部二进制** — **`.hrb` = 纸娃娃的 exe**；Day 20+ **API/syscall** 在此基础上扩展。

→ [Day 5/6 GDT Ring](../day-05-gdt-idt/) · [Day 15 far-JMP](../day-15-multitask1/) · [06-The-Linux-Programming-Interface execve 概念](../../../../06-The-Linux-Programming-Interface/)

---
