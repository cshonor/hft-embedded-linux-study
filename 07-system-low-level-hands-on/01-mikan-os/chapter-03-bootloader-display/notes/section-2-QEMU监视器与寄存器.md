## 2. QEMU 监视器与寄存器

> 底层开发中，**改一行打印** 有时会让 Bug **消失或变样** — 需要 **CPU 状态级** 调试手段。

---

### 一、QEMU 监视器（Monitor）

| 能力 | 说明 |
|------|------|
| **查看寄存器** | 通用寄存器、**RIP**、**RFLAGS** 等 |
| **查看内存** | 指定物理/虚拟地址 dump |
| **控制执行** | 暂停、单步（配合 gdb 更强） |

**进入方式：** QEMU 运行时快捷键（常见 **`Ctrl+Alt+2`** 切到 monitor 控制台，视配置而定）— 详见官方仓库 `run_qemu.sh` 与 [SETUP.md](../../SETUP.md)。

**典型用途：**

- 内核跳转后 **RIP 是否在预期入口**
- 帧缓冲地址写入是否正确
- 「Heisenbug」— 加日志改变时序 → 用 monitor **看不改时序的状态**

---

### 二、关键寄存器（x86-64）

| 寄存器 | 作用 |
|--------|------|
| **RAX–RDI 等** | **通用寄存器** — 算术、地址、参数传递（SysV ABI） |
| **RIP** | **Instruction Pointer** — 下一条要执行的指令地址 |
| **RFLAGS** | **标志寄存器** — 零标志 ZF、进位 CF、中断 IF 等 |

```
RIP 指向哪里 → CPU 正在执行哪段代码
RFLAGS → 上一条指令是否为零、是否开中断…
```

→ [CSAPP Ch3 机器级程序](../../../01-CSAPP-3rd/chapter-03-machine-level-programs/) · [02 附录 A 指令集](../../../02-Computer-Architecture-6th/appendix-A-指令集原理.md)

---

### 三、与本章实验的关系

| 场景 | 用 monitor 验证 |
|------|-----------------|
| **Loader 跳 kernel 后黑屏** | RIP 是否 = ELF **Entry Point** |
| **屏幕花屏** | 帧缓冲基址是否落在 **GOP 报告的区域** |
| **随机崩溃** | 栈指针 RSP、页分配是否越界 |

**建议：** 开发循环 = **QEMU monitor**（快速看状态）+ 必要时 **gdb stub**（源码级）。

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 内核与 ELF](./section-3-第一个内核与ELF加载.md)
