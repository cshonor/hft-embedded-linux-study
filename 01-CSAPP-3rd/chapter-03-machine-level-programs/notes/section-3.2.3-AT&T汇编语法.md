## 3.2.3 汇编语法：HFT / Linux 只练 **AT&T（gas）**

> [章导读](../README.md) · 上节 [§3.2.2 栈帧](./section-3.2.2-栈帧.md) · 下节 [§3.3 数据格式](./section-3.3-数据格式.md)

---

> **本仓库 CSAPP + HFT 路径：只学 AT&T。**  
> 共置机几乎全是 **Linux**；`gcc -S`、`objdump -d`、`perf annotate`、内核、CSAPP 书均为 AT&T。  
> **不必专门学 Intel 语法**（操作数顺序相反，混着练容易搞反源/目标）。

| AT&T 习惯 | 例 |
|-----------|-----|
| 寄存器带 `%` | `%rax` `%rbp` |
| **源在左，目标在右** | `movq %rax, %rbx` — rax → rbx |
| 立即数带 `$` | `movl $10, %eax` |
| 宽度后缀 | `movb` / `movw` / `movl` / `movq` |
| 内存 | `8(%rbp)` · `(%rax)` |

```asm
pushq   %rbp
movq    %rsp, %rbp
subq    $32, %rsp
call    foo
```

**可选备注（非学习目标）：** Intel SDM、部分 NASM/UEFI 手写 asm、Windows 文档用 **Intel 语法**（`mov rbx, rax` = 目标在左）。若偶尔对照手册，记住「顺序相反」即可，**HFT 日常仍只读 AT&T**。

### 前后章节关联

| 方向 | 内容 |
|------|------|
| **本章（Ch3）** | 指令、寄存器、栈、调用约定 → **bootX64.efi / 汇编引导 / 读反汇编** 的理论根 |
| **下一章（Ch4）** | CPU 流水线、缓存、指令级并行 → **HFT 超低延迟** 的硬件侧核心 |
| **Ch5** | 编译优化与性能 — 配合 `-O3` 汇编、`perf` |
| **MikanOS** | Ch1 UEFI C · 后续分页/中断汇编 — 遇不懂的指令回来查 **§3.2–3.7** |

---

### 口述巩固 · 自测

1. **HFT/Linux 练哪种汇编语法？** — **只练 AT&T**（`movq %rax, %rbx` = 源→目标）

---

← [本章导读](../README.md) · [§3.2.2 ←](./section-3.2.2-栈帧.md) · [§3.3 →](./section-3.3-数据格式.md)
