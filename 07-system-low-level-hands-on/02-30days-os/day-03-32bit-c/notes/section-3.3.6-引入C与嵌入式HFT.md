## ③.6 引入 C 语言 · 嵌入式 / HFT 要写 asm 吗？

### 引入 C 语言

| 文件 / 符号 | 作用 |
|-------------|------|
| **`bootpack.c`** | OS 本体主要逻辑（C） |
| **`HariMain`** | C 程序 **入口**（类似 `main`） |

**意义：** 复杂功能 **不必再堆汇编** — 为 Day 4+ 图形、内存管理、多任务 **铺路**。

C 写不了的 **`HLT`、切模式** 等仍靠 [§3.4 汇编与 C](./section-3.4-汇编与-C-的结合.md) 里的 **`asmfunc.asm`** 等。

→ [01-CSAPP](../../../../01-CSAPP-3rd/) · C 与机器码 · [03-Linux-Kernel-Development](../../../../03-Linux-Kernel-Development/) 内核 mostly C

---

### 嵌入式 / HFT 也要写汇编吗？

| 领域 | C/C++ | 汇编 | 说明 |
|------|-------|------|------|
| **嵌入式** | **主** | **少量** startup.S、中断向量 | SDK 好则几乎只写 C |
| **HFT** | **主** | **极少** 内联 asm（RDTSC、屏障、CAS） | 业务 **不要** 堆汇编 |
| **OS 引导（01）** | **HariMain** | **多** IPL、nasmhead、asmfunc | **比嵌入式更依赖 asm** |

**结论：** 底层 **逃不开 asm 概念**；**深入写汇编** 主要是 **OS 引导/内核启动**。嵌入式 **几百行 startup**，HFT **几条指令级优化** 即可。

**学习主次（HFT / 嵌入式）：** **跳过 haribote 系统敲码**；C + CSAPP 达标后 **直接 MikanOS** → [HFT-AND-EMBEDDED-PRIORITY.md](../../../HFT-AND-EMBEDDED-PRIORITY.md)

---

← [§3.3.5 MikanOS](./section-3.3.5-MikanOS与UEFI对照.md) · [§3.4 汇编与 C →](./section-3.4-汇编与-C-的结合.md) · [§3.3 导读](./section-3.3-32-位模式前期准备与导入-C-语言.md)
