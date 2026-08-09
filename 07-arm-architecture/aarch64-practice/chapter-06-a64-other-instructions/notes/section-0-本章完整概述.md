# Ch6 完整总结 · 其他重要指令

> **《ARM64体系结构编程与实践》** · 奔跑吧Linux社区 · **精读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md) · [Pi5 适配](../../PI5-ADAPT.md)

---

## 本章定位

覆盖前几章没讲的关键指令：**ADRP**（地址计算）、**SVC**（系统调用）、**MRS/MSR**（系统寄存器）、**LDXR/STXR**（独占访问）、**DMB/DSB/ISB**（屏障预览）。屏障和原子细节分别在 Ch18-20 展开。

实验优先 **QEMU** `-cpu cortex-a76`。

---

## 6.1 ADR / ADRP ⭐内核重定位关键

| 指令 | 行为 | 范围 |
|------|------|------|
| `ADR Xd, label` | `Xd = PC + offset` | ±1MB |
| `ADRP Xd, label` | `Xd = (PC & ~0xFFF) + (offset << 12)` | ±4GB（页对齐） |

```asm
; ADRP 取页基址，再 LDR/ADD 取页内偏移
adrp x0, sym
add  x0, x0, #:lo12:sym   ; 拼低 12 位得到完整地址

; 或者用 LDR 伪指令一步到位（编译器自动拆）
ldr  x0, =sym              ; 伪指令，放 litpool
```

### ADRP + ADD 两步法（内核高频）

```asm
; 读取全局变量 my_var 的地址
adrp x0, my_var            ; x0 = my_var 所在 4KB 页的基址
ldr  x1, [x0, #:lo12:my_var] ; 加页内偏移并读值
```

> **为什么不用 MOV？** MOV 只能加载 16 位立即数 + 移位，64 位地址放不下。  
> **为什么 ADRP 而非 ADR？** ADRP 范围 ±4GB，ADR 只有 ±1MB。  
> **内核重定位**：KASLR 下代码位置不固定，必须用 PC 相对寻址（ADRP），不能硬编码地址。

---

## 6.2 SVC —— 系统调用

```asm
; 用户态触发 SVC 异常 → 陷入 EL1（内核态）
mov x8, #64       ; syscall number (write=64 on AArch64)
svc #0            ; 触发同步异常
```

- `SVC #imm` → 产生同步异常，硬件切换到 EL1，跳到异常向量表
- `imm` 不被硬件使用（软件可读 ESR_EL1 的 IL/ISS 域获取），通常写 `#0`
- x8 = 系统调用号；x0-x5 = 参数；x0 = 返回值（AArch64 Linux ABI）

> 类比 x86 的 `syscall` / `int 0x80`。详见 Ch11 异常处理。

---

## 6.3 MRS / MSR —— 系统寄存器读写 ⭐

通用寄存器（X0-X30）和系统寄存器是**不同的地址空间**，必须用 MRS/MSR 桥接。

| 指令 | 行为 |
|------|------|
| `MRS Xd, sysreg` | 读系统寄存器 → Xd（如 `mrs x0, CurrentEL`） |
| `MSR sysreg, Xn` | 写 Xn → 系统寄存器（如 `msr DAIFSet, #0xf`） |

```asm
; 读当前异常等级
mrs x0, CurrentEL
lsr x0, x0, #2     ; EL 在 bit[3:2]
; x0 = 0/1/2/3

; 读 PSTATE
mrs x0, NZCV       ; 读 NZCV 标志到 x0

; 关中断（设 DAIF.I）
msr DAIFSet, #0xf  ; 屏蔽所有（D+A+I+F）
; 或手动：
mrs x0, DAIF
orr x0, x0, #0x80  ; 设 I 位
msr DAIF, x0
```

> **内核中大量使用 MRS/MSR**：读 TTBR、TCR、SCTLR、VBAR、ESR、FAR 等。  
> **MSR 写 SCTLR 需要屏障**：写完系统寄存器后通常跟 `isb`。

---

## 6.4 LDXR / STXR —— 独占访问（预览）⭐

| 指令 | 行为 |
|------|------|
| `LDXR Wd/Xd, [Xn]` | 独占加载（标记监视器） |
| `STXR Ws, Wd/Xd, [Xn]` | 独占存储；Ws=0 成功，Ws=1 失败 |
| `LDAR Wd/Xd, [Xn]` | Load-Acquire（自带 acquire 语义） |
| `STLR Wd/Xd, [Xn]` | Store-Release（自带 release 语义） |

```asm
; 原子加法（CAS 循环模式）
retry:
    ldxr x1, [x0]       ; 独占读
    add  x1, x1, #1     ; 修改
    stxr w2, x1, [x0]   ; 独占写
    cbnz w2, retry      ; 失败则重试
```

> 独占监视器（Exclusive Monitor）跟踪 LDXR 后是否有其他核/异常改写了同一缓存行。  
> 细节在 Ch20 原子操作展开。Ch7 有 LDXR 导致死机的案例分析。

---

## 6.5 DMB / DSB / ISB —— 内存屏障（预览）

| 指令 | 行为 |
|------|------|
| `DMB` | Data Memory Barrier：保证屏障前的访存完成后，屏障后的访存才可见 |
| `DSB` | Data Synchronization Barrier：比 DMB 更强，等所有访存完成才继续执行 |
| `ISB` | Instruction Synchronization Barrier：冲刷流水线，保证后续指令重新取指 |

```asm
; 写完系统寄存器后用 ISB
msr SCTLR_EL1, x0
isb                ; 确保后续指令在新的系统配置下执行

; DMA 场景：写描述符 → DSB → 启动 DMA
str x1, [x0, #DESC_OFFSET]
dsb sy             ; 确保描述符写入对 DMA 可见
str x2, [x0, #CTRL_OFFSET]  ; 启动 DMA
```

> DMB vs DSB：DMB 只约束访存顺序，CPU 可继续执行非访存指令；DSB 完全停住。  
> 细节在 Ch18-19 展开。

---

## 6.6 实验要点

| 实验 | 内容 | 平台 |
|------|------|------|
| 6-1 | 测试 ADRP 和 LDR 伪指令 | QEMU |
| 6-2 | ADRP 和 LDR 伪指令的陷阱 | QEMU |
| 6-3 | LDXR 和 STXR 指令的使用1 | QEMU |
| 6-4 | LDXR 和 STXR 指令的使用2 | QEMU |

---

## 6.7 易错点清单

1. **ADRP 地址是页对齐的** → 必须加 `:lo12:` 偏移才能得到精确地址。
2. **SVC 的 imm 不被硬件使用** → 不要以为是系统调用号，系统调用号在 X8。
3. **MRS/MSR 操作系统寄存器需要权限** → EL0 不能随意读写 EL1+ 寄存器。
4. **LDXR/STXR 之间不能有太多指令** → 否则监视器可能被清除，导致永远失败。
5. **MSR 后忘了 ISB** → 可能导致后续指令在旧配置下执行，行为未定义。

---

## 书中思考题（自测）

1. ADRP 算出来的地址有什么特点？为什么要配合 ADD 或 LDR 的 `:lo12:`？
2. SVC 触发什么类型的异常？系统调用号存在哪个寄存器？
3. MRS 和 MSR 分别做什么？能用 MOV 操作系统寄存器吗？
4. LDXR/STXR 如何实现原子操作？如果 STXR 返回 1 代表什么？
5. DMB、DSB、ISB 的区别？

**参考答案：**

1. **4KB 页对齐**（低 12 位为 0）；配合 `:lo12:` 拼出完整地址。  
2. **同步异常**；系统调用号在 **X8**（AArch64 Linux）。  
3. MRS 读系统寄存器→通用寄存器；MSR 写通用寄存器→系统寄存器。**不能用 MOV**。  
4. LDXR 标记监视器，STXR 检查是否被干扰；返回 **1 = 失败**（需重试）。  
5. DMB 约束访存顺序；DSB 停 CPU 等访存完成；ISB 冲刷流水线。

---

上一章 [Ch5 比较与跳转](../../chapter-05-a64-compare-branch/) · 下一章 [Ch7 工程陷阱](../../chapter-07-a64-traps/) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md)
