## 4. 异常处理与调试

---

### 一、特权级带来的新故障

| 异常 | 常见原因 |
|------|----------|
| **#PF (14)** | 访问 **未映射** · **User 访问内核页** · NX 执行数据 |
| **#GP (13)** | **错误段选择子** · **IRET 特权违规** · 非法 MSR |
| **#UD (6)** | Ring3 执行 **特权指令** |

**无调试时：** 三重 fault → **QEMU 重启** — 难以定位。

---

### 二、异常处理程序增强

**在 IDT 异常向量上挂调试 handler：**

```
ExceptionHandler(vector, error_code, frame):
    Print("Exception %d err=%x\n", vector, error_code);
    Print("RAX=%lx RBX=%lx … RIP=%lx RSP=%lx\n", …);
    HaltOrPanic();
```

| 打印内容 | 价值 |
|----------|------|
| **vector / error_code** | 区分 **PF 原因位**（present/write/user） |
| **通用寄存器** | 定位 **哪条 syscall** 传错参 |
| **RIP** | 对应 **应用 .text 偏移** |

→ [Ch7 中断框架](../chapter-07-interrupt-fifo/)

---

### 三、典型踩坑

| 现象 | 排查 |
|------|------|
| **一跑应用就 PF** | 页表 **缺 User** · **栈未映射** |
| **syscall 后 GP** | **sysret** 栈/段不对 · **STAR/LSTAR** 配错 |
| **定时器后内存花屏** | **TSS.RSP0** 无效 |

**本章价值：** 把 **“硬重启”** 变成 **可读的寄存器屏显** — 后续 Ch21+ GUI 应用 **必备**。

---

← [3. TSS](./section-3-TSS与RSP0内核栈.md) · 下一节 [5. syscall](./section-5-syscall机制与SyscallEntry.md)
