## ② INT 0x40 · 稳定 API 入口

#### 硬编码地址的致命伤

**OS 任一改动** → 函数 **重定位** → **旧 .hrb 全崩**。

#### 中断做 syscall 门

| 事实 | 利用 |
|------|------|
| IDT **0~255**，键鼠等只用少数 | **空闲向量**，如 **`0x40`** |
| CPU **`INT n`** | 固定 **2 字节**，不 embed 地址 |

**注册：** IDT[0x40] → **API 汇编入口**（再 **PUSHAD / 分发 / POPAD / IRET** 或 **IRET/RETF** 链，以原书为准）

```nasm
; app 侧
MOV EDX, 1          ; 功能号（见 §⑤）
INT 0x40            ; 呼叫 OS — 与 OS 版本解耦
```

| 对比 | far-CALL 硬地址 | **INT 0x40** |
|------|-----------------|--------------|
| OS 升级 | app 需重编 | **app 不变**（接口稳定） |
| 代码体积 | CALL 远指针更长 | **INT 更短** |

**现代对照：** x86 **`syscall`/`sysenter`**、Linux **号 + 寄存器** — 本课是 **INT 0x40 教学版**。

→ [Day 5/6 IDT](../day-05-gdt-idt/) · [06-The-Linux-Programming-Interface syscall](../../../../06-The-Linux-Programming-Interface/)

---
