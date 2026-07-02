## ④.4 嵌入式 / HFT 与何时用 asm

### 和嵌入式一样吗？

| | **OS 引导（haribote）** | **嵌入式（MCU）** |
|---|------------------------|-------------------|
| **汇编** | IPL、nasmhead、asmfunc | **startup.S** — 复位、栈、跳 `main` |
| **C** | **`HariMain`** | **`main()`** |
| **比例** | 启动链 **大段 asm** | startup **几十～几百行**，其余 **大量 C** |

**嵌入式常见模式：**

1. **上电** → asm **startup**  
2. **`main()`** → 几乎全 C  
3. 偶尔 **内联 asm** — `WFI`、关中断  

**高端 SDK：** 厂商封装 startup — 你 **只写 C**，底下仍是 asm。

**本仓库：** [00-practice-go-dex](../../../../00-Trading-and-Exchanges/00-practice-go-dex/) 纯 Go；OS/底层才是 **asm + C**。嵌入式支线 [18–23 模块](../../../)。

---

### 什么时候必须写 `.asm`？

| 场景 | 用 |
|------|-----|
| 循环、画图、内存管理 | **`.c`** |
| **`HLT`、CR、切 CPU 模式** | **`.asm`** |
| 512 B 引导、BIOS 中断 | **`.asm`**（[ipl.asm](../code/sec-3.1-ipl-int13-disk-load/ipl.asm)） |

**HFT 对照：** C++ 写逻辑；极少数 **内联 asm**（RDTSC、屏障、CAS）— **高级语言 99%，汇编只包几条指令**。

---

### 自检

- [ ] 说清 **C 为什么不能直接写 `HLT` / 切模式**  
- [ ] 说清 **asmfunc 包装、bootpack.c 调用、链接器合并**  
- [ ] 区分 **ipl / nasmhead / asmfunc / bootpack.c**  
- [ ] 能口述 **16→32 例子里 asm 与 C 的分工**  
- [ ] 看过 [sec-3.4-bootpack-asm-and-c/](../code/sec-3.4-bootpack-asm-and-c/) 与 [sec-3.4-minimal-16-to-32-call-c/](../code/sec-3.4-minimal-16-to-32-call-c/)

---

← [§3.4.3 完整例子](./section-3.4.3-16切32与call-C完整例子.md) · [§3.4 导读](./section-3.4-汇编与-C-的结合.md) · [Day 4 →](../../day-04-c-graphics/)
