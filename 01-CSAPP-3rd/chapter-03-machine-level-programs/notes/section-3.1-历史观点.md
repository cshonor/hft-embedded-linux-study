## 3.1 历史观点

> **导读定位：** 本章讲 **指令、寄存器、栈、调用约定** — 看懂 `bootX64.efi`、`gcc -S`、MikanOS 汇编的理论根基。  
> **实操线：** [MikanOS Ch1 UEFI](../../../08-system-low-level-hands-on/01-mikan-os/chapter-01-hello-world/) · **性能线：** [Ch4](../../chapter-04-processor-architecture/) → HFT 延迟优化。

> [章导读](../README.md) · 下节 [§3.2.1 编译链路](./section-3.2.1-机器级代码与编译链路.md)

---

### 1. ISA 迭代路线

```
8086（16 位实模式，BIOS 软盘 IPL 那套）
    ↓
IA-32（32 位保护模式，《30 天自制 OS》中后期）
    ↓
x86-64 / AMD64（64 位长模式 — MikanOS · bootX64.efi · Linux 全是这套）
```

| 阶段 | 典型场景 | 与你学习路径 |
|------|----------|--------------|
| **8086 / 实模式** | 软盘 512B IPL @ `0x7C00` | [02 30days-os Day1](../../../08-system-low-level-hands-on/02-30days-os/day-01-boot-asm/) 启蒙 |
| **IA-32** | 保护模式、32 位内核 | 30 天 OS 中后期 |
| **x86-64 长模式** | **UEFI · bootX64.efi · 服务器 OS** | **MikanOS 主线 · HFT 共置机** |

**结论：** 写 UEFI 引导、自制内核、HFT 服务器 — **核心 ISA 都是 x86-64**，不是 8086 实模式。

→ MikanOS 启动链对照 [§1.四 BOOTX64 vs IPL](../../../08-system-low-level-hands-on/01-mikan-os/chapter-01-hello-world/notes/section-1-本章定位.md#四核心区分bootx64efi--软盘启动两条线)

---

### 2. CISC vs RISC · x86 与 ARM 五大区别

两种 **完全不同的 CPU 架构**：指令编码、寄存器、寻址、ABI、汇编 **互不兼容**（同一套 C 源码要分别编译）。

| | **x86 / x86-64（CISC）** | **ARM / AArch64（RISC）** |
|---|-------------------------|---------------------------|
| **① 指令集** | 复杂指令；长度 **不固定**（常约 1–15B）；可 **内存直接参与运算** | 指令 **功能单一**；AArch64 指令宽 **固定 4B**；**Load/Store**：算术前必须先 `ldr` 进寄存器 |
| **② 寄存器** | 通用约 **16** 个；可单独摸 8/16/32/64 位片段 | 通用约 **31** 个（`x0`–`x30`）+ `sp`；`wN` = 低 32 位 |
| **③ 寻址** | 基址+偏移、比例变址、绝对址等 **很丰富** | 模式 **更少**；复杂地址常要额外算 |
| **④ 场景** | PC / 服务器 / **HFT 共置机**（Intel/AMD） | 手机、板子、嵌入式、Apple Silicon、部分云 ARM |
| **⑤ ABI/汇编** | System V；**Linux gas AT&T**（`%rdi`…） | AAPCS64：`x0`–`x7` — **二进制不能互通** |

**直观对比（内存加法）：**

```asm
; x86-64 允许内存作源操作数
addq (%rbx), %rax

; AArch64 必须分步（load → 寄存器运算 → 可选 store）
ldr x1, [x0]
add x0, x0, x1
```

**栈指针口语：** x86 的 `%rsp` 是 ISA 里固定的栈指针角色；AArch64 的 `sp` 也有硬件约束（对齐等），但日常仍说「软件约定用它当栈顶」——与「随便拿个 `x9` 当栈」不是一回事。

| 线 | 怎么用这张表 |
|----|----------------|
| 本书 / MikanOS | **全书汇编、分页、中断** 走 **x86-64 CISC** |
| HFT | 超低延迟机房仍 **几乎全是 x86-64**；ARM 多见于手机/普通云 |
| 嵌入式 | → [19-ARM64](../../../19-ARM64-Architecture/)（奔跑吧 AArch64 · Smith ARM32） |

**极简背诵：** 两边 64 位 GPR 都是 **一次最多 8 字节**；但 **CISC ≠ RISC**，机器码与汇编 **不能混跑**。

---

### 口述巩固 · 自测

1. **HFT 共置机主流 ISA？** — **x86-64**  
2. **x86 与 ARM 各举一条硬差异？** — CISC 可变长 / mem 运算 vs RISC 定长 Load-Store；或 GPR 约 16 vs 约 31

---

← [本章导读](../README.md) · [§3.2.1 →](./section-3.2.1-机器级代码与编译链路.md)
