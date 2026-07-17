## 3.1–3.2 历史观点与程序编码

> **导读定位（贴合两条线）：** 本章讲 **指令、寄存器、栈、调用约定** — 看懂 `bootX64.efi` 底层、`gcc -S` 输出、MikanOS 汇编引导的 **理论根基**。  
> **实操线：** [MikanOS Ch1 UEFI](../../../08-system-low-level-hands-on/01-mikan-os/chapter-01-hello-world/) · **性能线：** 下一章 [Ch4 流水线/缓存](../../chapter-04-processor-architecture/) → HFT 延迟优化。

---

### 3.1 历史观点

#### 1. ISA 迭代路线

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

#### 2. CISC vs RISC · x86 与 ARM 五大区别

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


### 3.2.1 机器级代码：完整编译链路

```
C 源码 (.c)
    ↓  编译器（gcc/clang）
汇编 (.s)          ← gcc -S 停在这里，人类可读
    ↓  汇编器 (as)
目标文件 (.o)      ← 机器码 + 重定位信息，尚不可直接运行
    ↓  链接器 (ld)
可执行程序         ← Linux ELF · UEFI 则为 PE（BOOTX64.EFI）
```

**优化级别 — 两条线各取所需：**

| 选项 | 用途 | 说明 |
|------|------|------|
| **`-Og`** | **学汇编、对照 CSAPP** | 代码清晰、结构可读 — **读 `-S` 输出必用** |
| **`-O3`** | **HFT 上线、性能压测** | 激进优化 — **只看 Release 热路径汇编** |
| **`objdump -d`** | 调试内核 / 交易程序 | 对 **已有二进制** 反汇编，不依赖源码 |

```bash
gcc -Og -S -fno-verbose-asm hello.c -o hello.s   # 学习：看 C → 汇编（默认 AT&T）
gcc -O3 -S hotpath.c -o hotpath-O3.s             # HFT：看优化后真实形态
objdump -d ./a.out | less                        # Linux 默认 AT&T；HFT 共置机常用
# 不要用 -M intel —— 本路径只练 AT&T，与 CSAPP / perf annotate 一致
```

**MikanOS Ch1 补充：** UEFI 的 `.efi` 同样是 **C → 交叉编译 → .o → 链接成 PE**，只是工具链用 `clang -target x86_64-pc-win32-coff` + `lld-link`，不是宿主机 `gcc` 默认的 ELF 路径 → [MikanOS §2 交叉编译](../../../08-system-low-level-hands-on/01-mikan-os/chapter-01-hello-world/notes/section-2-二进制编辑器与BOOTX64.md)。

---

### 3.2.2 栈帧（内核 / 函数调用底层核心）

**经典函数序言（建立栈帧）：**

```asm
push %rbp          ; 保存调用者帧指针
mov  %rsp, %rbp    ; 当前 rsp 作为本帧基址
sub  $N, %rsp      ; 在栈上开辟 N 字节（局部变量）
```

| 步骤 | 作用 |
|------|------|
| **`push %rbp`** | 链到上一帧，便于回溯栈 |
| **`mov %rsp,%rbp`** | 固定本函数 **帧基址** |
| **`sub $N,%rsp`** | 局部变量、 spill 槽在栈上 |

**现代编译器 & 高性能代码（HFT、UEFI 驱动）：** 常 **省略帧指针**（`-fomit-frame-pointer` / `-O2` 默认），只用 **`%rsp` 相对寻址** — 少几条指令、多一个可用寄存器，**profile 里常见无 `%rbp` 的序言**。

→ 完整调用约定、返回地址、`call`/`ret` 见 [§3.7 过程与栈帧](./section-3.7-过程与栈帧.md)

---

### 3.2.3 汇编语法：HFT / Linux 只练 **AT&T（gas）**

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

---

### 前后章节关联

| 方向 | 内容 |
|------|------|
| **本章（Ch3）** | 指令、寄存器、栈、调用约定 → **bootX64.efi / 汇编引导 / 读反汇编** 的理论根 |
| **下一章（Ch4）** | CPU 流水线、缓存、指令级并行 → **HFT 超低延迟** 的硬件侧核心 |
| **Ch5** | 编译优化与性能 — 配合 `-O3` 汇编、`perf` |
| **MikanOS** | Ch1 UEFI C · 后续分页/中断汇编 — 遇不懂的指令回来查 **§3.2–3.7** |

---

### 学习顺序建议（先 MikanOS 实操，再啃 CSAPP）

```
1. MikanOS Ch1：写 UEFI C、make 出 bootX64.efi — 先跑通
2. 对 hello.c 做 gcc/clang -S、objdump -d — 对照本章 §3.2.1–3.2.3
3. 遇汇编/寄存器/栈看不懂 → 回读 §3.2.2、§3.7
4. 内核基础跑通后 → 深入 Ch4 流水线，服务 HFT 延迟优化
```

**不必死磕顺序：** CSAPP Ch3 可与 MikanOS **交叉读** — 以「当前卡住的汇编/栈问题」为索引回查即可。

---

### 口述巩固 · 自测

1. **HFT 共置机主流 ISA？** — **x86-64**；8086 实模式仅历史启蒙（30 天 OS），不是 MikanOS/服务器主线。
2. **学汇编用 `-O0` 还是 `-Og`/`-O3`？** — **学习用 `-Og`**；**压测/上线看 `-O3`** 汇编。
3. **HFT/Linux 练哪种汇编语法？** — **只练 AT&T**（`movq %rax, %rbx` = 源→目标）；不必专门学 Intel。
4. **为何 Release 里常看不到 `%rbp`？** — **省略帧指针** 换性能；用 `%rsp` 相对寻址。
5. **`.efi` 和本章编译链关系？** — 同样是 **C → .o → 链接**；UEFI 链出 **PE** 而非 ELF。
6. **x86 与 ARM 各举一条硬差异？** — 例：CISC 可变长指令 / 可 mem 运算 vs RISC 定长 Load-Store；或 GPR 约 16 vs 约 31。

---

← [本章导读](../README.md) · 下一节 [3.3–3.4 数据格式](./section-3.3-3.4-数据格式与访问信息.md) · [3.7 栈帧深入](./section-3.7-过程与栈帧.md)
