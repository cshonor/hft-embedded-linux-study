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

#### 2. CISC vs RISC

| | **x86（CISC）** | **ARM（RISC）** |
|---|-----------------|-----------------|
| 指令 | **长度不固定**，寻址模式极多 | 定长指令为主 |
| 本书 / MikanOS | **全书汇编、分页、中断** 均 x86 CISC 逻辑 | 对比心里有数即可 |
| **HFT 现实** | 超低延迟机房 **清一色 Intel/AMD x86-64** | ARM 多见于普通云；**低延迟交易栈极少选 ARM** |

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
gcc -Og -S -fno-verbose-asm hello.c -o hello.s   # 学习：看 C → 汇编
gcc -O3 -S hotpath.c -o hotpath-O3.s             # HFT：看优化后真实形态
objdump -d ./a.out | less                        # 现成 ELF 反汇编
objdump -d -M intel hello.o                      # 同一 .o 可强制 Intel 语法显示
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

### 3.2.3 两大汇编语法（极易踩坑）

写 bootX64.efi 相关代码、查 Intel 手册、读 MikanOS 汇编 — **必须区分**：

| | **AT&T** | **Intel** |
|---|----------|-----------|
| **谁在用** | **GCC 默认**、`objdump -d`、`perf annotate`、**Linux 内核**、**MikanOS 用 gcc/clang 看的 .s** | **Intel 官方手册**、**IDA 逆向**、**NASM 默认**（MikanOS [day01/asm](https://github.com/uchan-nos/mikanos-build/tree/master/day01/asm)）、部分 **Windows/EFI 文档** |
| **mov 格式** | `movq **源**, **目标**` | `mov **目标**, **源**` — **顺序完全相反** |
| **寄存器** | `%rax` `%rbp` | `rax` `rbp` |
| **字节后缀** | `movb/w/l/q` = 1/2/4/8 字节 | 常写 `mov rax, rbx`（64 位默认）或 `byte ptr` 等 |

#### 简易对照 · 同一操作

| 操作 | AT&T | Intel |
|------|------|-------|
| rax → rbx | `movq %rax, %rbx` | `mov rbx, rax` |
| 压栈 | `pushq %rbp` | `push rbp` |
| 调用 | `call foo` | `call foo` |
| 序言三件套 | `pushq %rbp` · `movq %rsp,%rbp` · `subq $16,%rsp` | `push rbp` · `mov rbp,rsp` · `sub rsp,16` |

#### MikanOS 语境示例

**① 对 `hello.c` 的 `EfiMain` 做 `gcc -S`（AT&T）：**

```bash
clang -target x86_64-pc-win32-coff -S -Og hello.c -o hello.s
# 或 Linux 上 gcc -Og -S foo.c
```

可能看到类似：

```asm
pushq   %rbp
movq    %rsp, %rbp
subq    $32, %rsp
callq   OutputString      ; AT&T：call 仍无前缀差异
```

**② MikanOS 官方 day01 手写汇编（NASM · Intel）：**

```asm
; hello.asm — Intel 语法，与 AT&T 操作数顺序相反
mov     rax, rcx          ; Intel：目标 rax ← 源 rcx
push    rbp
call    SomeUefiRoutine
```

**实操提醒：**

1. **MikanOS + gcc/clang/objdump/perf** → 默认 **AT&T**；`objdump -d -M intel` 可临时切换显示。
2. **查 Intel SDM（分页、MSR、中断）** → 手册全是 **Intel**；把示例抄进 AT&T 环境时 **别反了源/目标**。
3. **NASM 写引导**（day01/asm）→ **Intel**；与 CSAPP 书里 AT&T 并排对照时，先认语法再认指令。

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
3. **`movq %rax,%rbx` 在 Intel 里怎么写？** — `mov rbx, rax`（**目标在左**）。
4. **为何 Release 里常看不到 `%rbp`？** — **省略帧指针** 换性能；用 `%rsp` 相对寻址。
5. **`.efi` 和本章编译链关系？** — 同样是 **C → .o → 链接**；UEFI 链出 **PE** 而非 ELF。

---

← [本章导读](../README.md) · 下一节 [3.3–3.4 数据格式](./section-3.3-3.4-数据格式与访问信息.md) · [3.7 栈帧深入](./section-3.7-过程与栈帧.md)
