## ① 多任务原理 · TSS（任务状态段）

#### 单核「同时」的真相

| 表象 | 实质 |
|------|------|
| 多个程序 **同时跑** | CPU **极快切换**（原书 ~**0.01–0.03 s**/次） |
| 人眼/交互感 | **时间片轮转** — 每个任务 **跑一小段 → 让出** |

**HFT：** 单核 **绑多线程** 仍是 **并发非并行**；**多核** 才是真并行 — 但 **切换/缓存冷** 成本同源。

#### 切换必须保存什么？

**全套 CPU 寄存器**（EIP、ESP、通用寄存器、段寄存器…）→ 切走 **写入内存**，切回 **读回**。

#### TSS + TR + far-JMP

| 组件 | 作用 |
|------|------|
| **TSS** | Task Status Segment — **存任务寄存器快照** 的特殊段 |
| **TR** | Task Register — CPU 里 **当前 TSS 在哪** |
| **far-JMP 到 TSS 选择子** | CPU **硬件自动** save/restore 寄存器 — **无缝切换** |

```
任务 A 运行中
    far-JMP → TSS_B
        CPU:  A 的寄存器 → TSS_A
              TSS_B → 寄存器
        任务 B 运行
```

→ [01-CSAPP Ch8 上下文切换](../../../../01-CSAPP-3rd/chapter-08-exceptional-control-flow/) · [03-Linux-Kernel-Development 进程切换](../../../../03-Linux-Kernel-Development/) · x86 TSS 在现代 OS 中 **部分字段仍用**，调度模型更复杂

---
