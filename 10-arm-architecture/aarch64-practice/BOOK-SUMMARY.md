# 《ARM64体系结构编程与实践》全书详细总结

> **编著：** 奔跑吧Linux社区 · **出版：** 人民邮电出版社（2022-04）· 异步图书 · 安谋科技教育计划推荐教材  
> **本仓库：** [aarch64-practice/](./) · 裁剪见 [OUTLINE.md](./OUTLINE.md) · 名词见 [AARCH64-NAMING.md](./AARCH64-NAMING.md)  
> **平台：** 原书 **Pi4B**（A72）+ QEMU；**本仓库实机 Pi5**（A76）— [PI5-ADAPT.md](./PI5-ADAPT.md)  
> **代码：** [runninglinuxkernel/arm64_programming_practice](https://github.com/runninglinuxkernel/arm64_programming_practice)  
> **不是** Hohl/Hinds 的 ARM32/Thumb（Cortex-M）→ [../arm32-asm/](../arm32-asm/)

---

## 一、图书基础信息

| 项 | 内容 |
|----|------|
| 定位 | 嵌入式 / 底层开发的 **AArch64 实操教材**（不讲 AArch32） |
| 实验 | Ubuntu · aarch64-gcc · QEMU（**推荐 `-cpu cortex-a76`**）· 原书写 Pi4B；**上板见 [PI5-ADAPT](./PI5-ADAPT.md)** |
| 资源 | GitHub 源码、Pi/QEMU VMware 镜像、调试教程；开篇 **20 道自测题**（高频面试） |
| 汇编风格 | GNU as **小写**；官方手册大写 |
| 名词 | 本书 = **AArch64 / A64**；口语 ARM64 见 [AARCH64-NAMING](./AARCH64-NAMING.md) |

**一句话：** 用国产实操案例打通「指令 → 汇编/链接 → 异常/GIC → MMU/Cache → 屏障/原子 → 小型 OS」，填补英文 ARM 手册与工程落地之间的缺口。

---

## 二、全书框架（23 章 · 九大模块）

```
硬件基础 (1–2)
  → A64 指令 (3–7)
  → 汇编/链接/内联 (8–10)
  → 异常/中断/GIC (11–13)
  → MMU (14) · Cache/一致性/TLB (15–17)
  → 屏障/原子 (18–20)
  → 简易 OS (21)
  → NEON / SVE (22–23)
```

每章：前置思考题 + 后置实操。标签与文件夹对齐 [OUTLINE](./OUTLINE.md)。

---

### 模块 1 · ARM64 硬件基础（Ch 1–2）· 精读

#### 第 1 章 · ARM64 体系基础知识

[chapter-01-arm64-fundamentals/](./chapter-01-arm64-fundamentals/) · **详记** → [section-0-本章完整概述.md](./chapter-01-arm64-fundamentals/notes/section-0-本章完整概述.md)

- ARM 脉络：v1→ARMv9；A/R/M 产品线；v8 双状态（**AArch64 / AArch32**）；v9 增 SVE2、CCA  
- **EL0–EL3**；X0–X30 / W / XZR；PSTATE（NZCV、DAIF、PAN/UAO）；SP、ELR、SPSR、系统寄存器  
- **Cortex-A72**（Pi4）：乱序超标量；I/D/L2；内置 GIC/MMU；Pi5=A76(v9) 概念兼容  

#### 第 2 章 · 实验环境（原书 Pi4B · 本仓库 Pi5 适配）

[chapter-02-raspberry-pi-lab/](./chapter-02-raspberry-pi-lab/) · **详记** → [Pi5 适配与实验路线](./chapter-02-raspberry-pi-lab/notes/section-0-Pi5适配与实验路线.md)

- 原书：BCM2711 · BenOS · 串口/JTAG · QEMU+GDB  
- **适配：** 架构实验 **QEMU 优先**；PL011/GIC 基址与 `config.txt` 勿照抄 4B；工具链不变  
- 两套用途：QEMU=裸机架构；Pi5=适配后外设 + 64-bit OS（TLPI/驱动课）

---

### 模块 2 · A64 指令集（Ch 3–7）· 全书实操核心 · 精读

#### 第 3 章 · 加载 / 存储（Load-Store）

[chapter-03-a64-load-store/](./chapter-03-a64-load-store/)

- 指令 **32 位定长**；访存必经寄存器  
- 寻址：基址、偏移、前/后变基、PC 相对；**LDR 真指令** vs **`ldr x0,=val` 伪指令**  
- LDRB/LDRSB、LDUR、LDP/STP（无 PUSH/POP）；LDXR/STXR、LDAR/STLR、LDTR  
- MOV 仅 16 位立即数（可移位）；大常量用 LDR 伪指令  
- 实验：寻址、memcpy、栈操作  

#### 第 4 章 · 算术、移位、位操作

[chapter-04-a64-arithmetic-shift/](./chapter-04-a64-arithmetic-shift/)

- ADD/ADDS/ADC、SUB/SUBS、CMP≡SUBS XZR；NZCV + 条件后缀  
- LSL/LSR/ASR/ROR；AND/ORR/EOR/BIC；UBFX/SBFX/BFI  

#### 第 5 章 · 比较与跳转

[chapter-05-a64-compare-branch/](./chapter-05-a64-compare-branch/)

- CSEL/CSET/CSINC；CMN≡ADDS  
- B / B.cond / BL（LR=X30）/ BR/BLR / RET / ERET；嵌套 BL 须栈存 X29/X30  
- CBZ/TBZ  

#### 第 6 章 · 杂项关键指令

[chapter-06-a64-other-instructions/](./chapter-06-a64-other-instructions/)

- ADR / **ADRP**（`#lo12` 拼全地址；内核重定位关键）  
- SVC、MRS/MSR、DMB/DSB/ISB  

#### 第 7 章 · 工程陷阱

[chapter-07-a64-traps/](./chapter-07-a64-traps/)

- 字符串加载、大数 MOV、独占死机、启动汇编踩坑；串口输出 / Linux 启动汇编大作业  

---

### 模块 3 · 汇编、链接、内联（Ch 8–10）· 精读

| 章 | 目录 | 要点 |
|----|------|------|
| **8** GNU as | [ch08](./chapter-08-gnu-assembler/) | 伪指令、段、宏；C↔汇编互调 |
| **9** LD / 链接脚本 | [ch09](./chapter-09-linker-scripts/) | VMA/LMA、位置计数器；BenOS/U-Boot/内核重定位 |
| **10** GCC 内联 | [ch10](./chapter-10-gcc-inline-asm/) | 约束、goto 模板；memset、系统寄存器、内核工具函数 |

---

### 模块 4 · 异常与中断（Ch 11–13）· 精读 · OS/飞控核心

#### 第 11 章 · 异常处理

[chapter-11-exception-handling/](./chapter-11-exception-handling/)

- 同步 vs 异步（IRQ/FIQ）；VBAR_ELx 向量表  
- 硬件保存 PSTATE/ELR/SPSR，软件保存通用寄存器；EL2→EL1 实验  

#### 第 12 章 · 中断基础

[chapter-12-interrupt-handling/](./chapter-12-interrupt-handling/)

- Pi 传统中断、通用定时器；现场保存/恢复；定时器驱动案例  

#### 第 13 章 · GIC-v2 / GIC-400

[chapter-13-gic-v2/](./chapter-13-gic-v2/)

- GIC 分层、路由、硬件中断号；定时器中断实验  

---

### 模块 5 · MMU / Cache / TLB（Ch 14–17）· 重难点

#### 第 14 章 · 页表与 MMU · 精读

[chapter-14-memory-management/](./chapter-14-memory-management/)

- 4 级长描述符（4KB、48 位 VA）；TTBR0/1、TCR、SCTLR  
- Normal vs Device；恒等映射；BenOS 建页表开 MMU + 故障调试  

#### 第 15–17 章 · Cache / 一致性 / TLB · 选读（可对照 Hennessy）

| 章 | 目录 | 要点 |
|----|------|------|
| **15** Cache | [ch15](./chapter-15-cache-basics/) | 直接/全相联/组相联；PIPT/VIPT；重名·同名别名；PoU/PoC |
| **16** MESI | [ch16](./chapter-16-cache-coherency/) | 四状态推演、伪共享；DMA 清/失效；自修改代码 |
| **17** TLB | [ch17](./chapter-17-tlb-management/) | ASID、刷新指令、内核维护、BBM |

---

### 模块 6 · 屏障与原子（Ch 18–20）· 精读 · 多核同步

| 章 | 目录 | 要点 |
|----|------|------|
| **18–19** 屏障 | [ch18](./chapter-18-memory-barriers/) · [ch19](./chapter-19-barrier-usage/) | 弱序模型；DMB/DSB/ISB；acquire/release；DMA/锁/TLB；内核源码 |
| **20** 原子 | [ch20](./chapter-20-atomic-operations/) | LDXR/STXR 独占监视器；CAS；WFE 低功耗自旋锁 |

---

### 模块 7 · 简易 OS（Ch 21）· 精读

[chapter-21-os-topics/](./chapter-21-os-topics/)

- AArch64 C 陷阱、调用约定、栈帧（main→func1→func2）  
- PCB、0 号进程、`do_fork`、上下文切换、简易抢占；SVC（malloc/clone 等自定义 syscall）  

---

### 模块 8 · 浮点与向量（Ch 22–23）· 选读 / 跳过

| 章 | 目录 | 要点 | 标签 |
|----|------|------|------|
| **22** NEON | [ch22](./chapter-22-fp-neon/) | FP 寄存器、LD1–LD4；RGB↔BGR、矩阵乘 | 选读 |
| **23** SVE/SVE2 | [ch23](./chapter-23-sve-optimization/) | 可变长向量、谓词；strcmp/图像/矩阵；QEMU SVE | 跳过（首遍） |

---

## 三、图书核心特色

1. **强实践：** 串口 → MMU → 带调度小型 OS，知识点可复现  
2. **面试向：** 章首思考题 + 开篇 20 题 ≈ 大厂 ARM 底层高频点  
3. **踩坑库：** MOV 范围、缓存别名、重定位崩溃、JTAG 等工程案例  
4. **双平台：** 原书 Pi4B + QEMU；本仓库 **QEMU（A76）+ Pi5 实物**（外设后迁）  
5. **权威索引：** ARM v8.6/v9 手册、BCM2711（原书）/ **BCM2712（Pi5）**、GNU 工具链

---

## 四、适用人群与本仓库用法

| 读者 | 价值 |
|------|------|
| 嵌入式底层 / 内核 / bootloader / 芯片验证 | 完整 AArch64 工程链路 |
| 本仓库飞控 / Pi 驱动线 | **精读** 1–14、18–21；15–17 对照 Hennessy；22–23 按需 |

**与 arm32-asm：** Hohl/Hinds 练 Load/Store·栈·调用约定思维；**板上 Pi5 / 本书实验 = 只认 A64**，语法不可直接搬。

**阅读顺序建议：** 见 [OUTLINE 推荐路径](./OUTLINE.md) — 有汇编基础可直入本书 Ch1。

---

## 五、一句话收束

> 本书以 **QEMU + BenOS（原书兼写 Pi4B）** 把 **AArch64/A64** 从指令写到简易 OS；口语叫 ARM64，规范名是 AArch64。  
> **本仓库实机 = Pi5**：架构实验跟 QEMU；板上外设按 [PI5-ADAPT](./PI5-ADAPT.md) 改基址后再迁。
